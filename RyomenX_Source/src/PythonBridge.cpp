#include "PythonBridge.hpp"
#include "Config.hpp"
#include "MemoryCheats.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <algorithm>

#if __has_include("EmbeddedPayload.hpp")
#include "EmbeddedPayload.hpp"
#define HAS_EMBEDDED_PAYLOAD 1
#else
#define HAS_EMBEDDED_PAYLOAD 0
#endif

#if defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <mach/machine.h>
#include <sys/stat.h>
#endif

namespace RyomenX {

// Verified RoS arm64 embedded Python offsets from client binary & RyomenX analysis
// Relative to __TEXT base (0x100000000 unslid, i.e., reinterpret_cast<uintptr_t>(_dyld_get_image_header(0)) + offset)
static const uintptr_t ROS_OFF_PY_ISINITIALIZED   = 0x1d05a24;
static const uintptr_t ROS_OFF_PYGILSTATE_ENSURE  = 0x1d05158;
static const uintptr_t ROS_OFF_PYGILSTATE_RELEASE = 0x1d051dc;
static const uintptr_t ROS_OFF_PYIMPORT_ADDMODULE = 0x1cf8fd4;
static const uintptr_t ROS_OFF_PYMODULE_GETDICT   = 0x1c79884;
static const uintptr_t ROS_OFF_PYRUN_STRINGFLAGS  = 0x1d0689c;
static const uintptr_t ROS_OFF_PYERR_CLEAR        = 0x1cf17f0;

static long long NowMillis() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

static std::string EscapePySingleQuoted(const std::string& input) {
    std::string out;
    out.reserve(input.size() + 16);
    char buf[8];
    for (unsigned char c : input) {
        if (c == '\\' || c == '\'' || c < 0x20) {
            std::snprintf(buf, sizeof(buf), "\\x%02x", c);
            out.append(buf);
        } else {
            out.push_back(static_cast<char>(c));
        }
    }
    return out;
}

static std::string IndentPythonFourSpaces(const std::string& code) {
    std::string out;
    out.reserve(code.size() + (code.size() / 16) + 64);
    size_t pos = 0;
    while (pos < code.size()) {
        size_t nl = code.find('\n', pos);
        size_t end = (nl == std::string::npos) ? code.size() : nl;
        // Strip trailing CR if present
        size_t line_end = end;
        if (line_end > pos && code[line_end - 1] == '\r') {
            line_end--;
        }
        out.append("    ");
        out.append(code, pos, line_end - pos);
        out.push_back('\n');
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    return out;
}

void PythonBridge::SetRuntimeStage(const std::string& stage) {
    {
        std::lock_guard<std::mutex> lock(m_status_mutex);
        m_runtime_stage = stage;
    }
    if (!m_cache_dir.empty()) {
        std::string p = m_cache_dir + "/runtime-stage.txt";
        std::ofstream f(p);
        if (f.is_open()) {
            f << stage << "\n";
        }
    }
}

std::string PythonBridge::GetRuntimeStage() {
    std::lock_guard<std::mutex> lock(m_status_mutex);
    return m_runtime_stage;
}

bool PythonBridge::Initialize(const std::string& cache_dir) {
    m_cache_dir = cache_dir;
    SetRuntimeStage("Resolving Python runtime");

    // Retry symbol resolution with backoff: Python may not be loaded yet at inject time
    bool symbols_ok = false;
    for (int attempt = 0; attempt < 15; ++attempt) {
        if (ResolvePythonSymbols()) {
            symbols_ok = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    if (!symbols_ok) {
        SetRuntimeStage("Runtime resolution failed");
        std::cerr << "[PythonBridge] Failed to resolve Python runtime symbols\n";
        m_initialized = true;
        return false;
    }

    m_initialized = true;

    // Wait until Python is actually initialized by the game engine
    SetRuntimeStage("Waiting for game Python");
    for (int attempt = 0; attempt < 30; ++attempt) {
        if (p_Py_IsInitialized && p_Py_IsInitialized()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    // Authoritative payload: embedded script (always current)
    std::string payload_code;
#if HAS_EMBEDDED_PAYLOAD
    payload_code = RyomenX::EMBEDDED_PAYLOAD;
    std::string script_path = m_cache_dir + "/neox_battle_payload.py";
    std::ofstream out(script_path);
    if (out.is_open()) {
        out << payload_code;
    }
#else
    std::string script_path = m_cache_dir + "/neox_battle_payload.py";
    std::ifstream file(script_path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        payload_code = buffer.str();
    }
#endif

    // Optional debug override if neox_override.py exists in cache dir
    std::string override_path = m_cache_dir + "/neox_override.py";
    std::ifstream ov_file(override_path);
    if (ov_file.is_open()) {
        std::stringstream buffer;
        buffer << ov_file.rdbuf();
        std::string ov_code = buffer.str();
        if (!ov_code.empty()) payload_code = ov_code;
    }

    if (payload_code.empty()) {
        SetRuntimeStage("No payload available");
        return false;
    }

    // Write initial config files BEFORE running Python payload
    WriteConfigSync(m_display_w, m_display_h, m_scale_x, m_scale_y);

    m_payload_active = ExecutePayload(payload_code);

    // Background watcher: ensure Python payload executes once runtime is ready
    if (!m_payload_active) {
        std::thread([this, payload_code]() {
            for (int i = 0; i < 60; ++i) {
                if (m_payload_active) break;
                std::this_thread::sleep_for(std::chrono::seconds(2));
                if (!m_payload_active && !payload_code.empty()) {
                    m_payload_active = ExecutePayload(payload_code);
                }
            }
        }).detach();
    }

    return m_payload_active;
}

bool PythonBridge::ResolvePythonSymbols() {
#if defined(__APPLE__)
    // 1. Try dlsym(RTLD_DEFAULT, ...) across all loaded modules
    p_Py_IsInitialized = reinterpret_cast<Py_IsInitialized_t>(dlsym(RTLD_DEFAULT, "Py_IsInitialized"));
    p_PyGILState_Ensure = reinterpret_cast<PyGILState_Ensure_t>(dlsym(RTLD_DEFAULT, "PyGILState_Ensure"));
    p_PyGILState_Release = reinterpret_cast<PyGILState_Release_t>(dlsym(RTLD_DEFAULT, "PyGILState_Release"));
    p_PyImport_AddModule = reinterpret_cast<PyImport_AddModule_t>(dlsym(RTLD_DEFAULT, "PyImport_AddModule"));
    p_PyModule_GetDict = reinterpret_cast<PyModule_GetDict_t>(dlsym(RTLD_DEFAULT, "PyModule_GetDict"));
    p_PyRun_StringFlags = reinterpret_cast<PyRun_StringFlags_t>(dlsym(RTLD_DEFAULT, "PyRun_StringFlags"));
    p_PyErr_Clear = reinterpret_cast<PyErr_Clear_t>(dlsym(RTLD_DEFAULT, "PyErr_Clear"));

    if (p_Py_IsInitialized && p_PyGILState_Ensure && p_PyGILState_Release &&
        p_PyImport_AddModule && p_PyModule_GetDict && p_PyRun_StringFlags) {
        return true;
    }

    // 2. Try multiple handles: main executable, then known NeoX Python dylib names
    void* handles[] = {
        dlopen(NULL, RTLD_NOW),
        dlopen("@rpath/libpython2.7.dylib", RTLD_NOW | RTLD_NOLOAD),
        dlopen("@rpath/Python.framework/Python", RTLD_NOW | RTLD_NOLOAD),
        dlopen("libpython2.7.dylib", RTLD_NOW | RTLD_NOLOAD),
        dlopen("Python", RTLD_NOW | RTLD_NOLOAD),
        dlopen(NULL, RTLD_LAZY)
    };

    for (void* handle : handles) {
        if (!handle) continue;

        if (!p_Py_IsInitialized) p_Py_IsInitialized = reinterpret_cast<Py_IsInitialized_t>(dlsym(handle, "Py_IsInitialized"));
        if (!p_PyGILState_Ensure) p_PyGILState_Ensure = reinterpret_cast<PyGILState_Ensure_t>(dlsym(handle, "PyGILState_Ensure"));
        if (!p_PyGILState_Release) p_PyGILState_Release = reinterpret_cast<PyGILState_Release_t>(dlsym(handle, "PyGILState_Release"));
        if (!p_PyImport_AddModule) p_PyImport_AddModule = reinterpret_cast<PyImport_AddModule_t>(dlsym(handle, "PyImport_AddModule"));
        if (!p_PyModule_GetDict) p_PyModule_GetDict = reinterpret_cast<PyModule_GetDict_t>(dlsym(handle, "PyModule_GetDict"));
        if (!p_PyRun_StringFlags) p_PyRun_StringFlags = reinterpret_cast<PyRun_StringFlags_t>(dlsym(handle, "PyRun_StringFlags"));
        if (!p_PyErr_Clear) p_PyErr_Clear = reinterpret_cast<PyErr_Clear_t>(dlsym(handle, "PyErr_Clear"));

        if (p_Py_IsInitialized && p_PyGILState_Ensure && p_PyGILState_Release &&
            p_PyImport_AddModule && p_PyModule_GetDict && p_PyRun_StringFlags) {
            return true;
        }
    }

    // 3. Fallback: If stripped, resolve directly via RoS client Mach-O base (header == slide + 0x100000000)
    const struct mach_header* hdr = _dyld_get_image_header(0);
    if (hdr && hdr->magic == MH_MAGIC_64 && hdr->cputype == CPU_TYPE_ARM64) {
        uintptr_t base = reinterpret_cast<uintptr_t>(hdr);
        const uint64_t* py_init_words = reinterpret_cast<const uint64_t*>(base + ROS_OFF_PY_ISINITIALIZED);
        const uint64_t* py_run_words  = reinterpret_cast<const uint64_t*>(base + ROS_OFF_PYRUN_STRINGFLAGS);

        // Verify prologue bytes match client binary before binding pointers
        bool sig_ok = (py_init_words[0] == 0x39674100900133e8ULL && py_init_words[1] == 0xd10203ffd65f03c0ULL) &&
                      (py_run_words[0]  == 0xa90157f6a9bc5ff8ULL && py_run_words[1]  == 0xa9037bfda9024ff4ULL);

        if (sig_ok) {
            p_Py_IsInitialized  = reinterpret_cast<Py_IsInitialized_t>(base + ROS_OFF_PY_ISINITIALIZED);
            p_PyGILState_Ensure = reinterpret_cast<PyGILState_Ensure_t>(base + ROS_OFF_PYGILSTATE_ENSURE);
            p_PyGILState_Release= reinterpret_cast<PyGILState_Release_t>(base + ROS_OFF_PYGILSTATE_RELEASE);
            p_PyImport_AddModule= reinterpret_cast<PyImport_AddModule_t>(base + ROS_OFF_PYIMPORT_ADDMODULE);
            p_PyModule_GetDict  = reinterpret_cast<PyModule_GetDict_t>(base + ROS_OFF_PYMODULE_GETDICT);
            p_PyRun_StringFlags = reinterpret_cast<PyRun_StringFlags_t>(base + ROS_OFF_PYRUN_STRINGFLAGS);
            p_PyErr_Clear       = reinterpret_cast<PyErr_Clear_t>(base + ROS_OFF_PYERR_CLEAR);
            return true;
        } else {
            SetRuntimeStage("Binary mismatch");
        }
    }

    return false;
#else
    return true;
#endif
}

bool PythonBridge::ExecutePayload(const std::string& python_code) {
    if (!m_initialized || python_code.empty()) return false;

#if defined(__APPLE__)
    if (!p_Py_IsInitialized || !p_Py_IsInitialized()) {
        SetRuntimeStage("Interpreter not ready");
        return false;
    }

    SetRuntimeStage("Waiting for Python GIL");
    void* gstate = p_PyGILState_Ensure ? p_PyGILState_Ensure() : nullptr;
    void* main_module = p_PyImport_AddModule ? p_PyImport_AddModule("__main__") : nullptr;
    void* global_dict = (main_module && p_PyModule_GetDict) ? p_PyModule_GetDict(main_module) : nullptr;

    if (!global_dict || !p_PyRun_StringFlags) {
        if (gstate && p_PyGILState_Release) p_PyGILState_Release(gstate);
        SetRuntimeStage("Failed getting __main__");
        return false;
    }

    SetRuntimeStage("Installing payload");

    std::string escaped_dir = EscapePySingleQuoted(m_cache_dir);
    std::string indented_body = IndentPythonFourSpaces(python_code);

    // Wrap payload identical to original RyomenX wrapper with proper 4-space block indentation
    std::string wrapped = "_RYOMENX_DIR = '" + escaped_dir + "'\n"
                          "import traceback\n"
                          "try:\n"
                          + indented_body +
                          "except Exception:\n"
                          "    open(_RYOMENX_DIR + '/payload-error.txt', 'w').write(traceback.format_exc())\n"
                          "    raise\n";

    // Py_file_input = 257 (0x101)
    void* result = p_PyRun_StringFlags(wrapped.c_str(), 257, global_dict, global_dict, nullptr);
    if (!result) {
        if (p_PyErr_Clear) p_PyErr_Clear();
        SetRuntimeStage("Payload failed (see payload-error.txt)");
    } else {
        SetRuntimeStage("ACTIVE (BigWorld)");
    }

    if (gstate && p_PyGILState_Release) {
        p_PyGILState_Release(gstate);
    }
    m_payload_active = (result != nullptr);
    return m_payload_active;
#else
    SetRuntimeStage("ACTIVE (Sim)");
    m_payload_active = true;
    return true;
#endif
}

void PythonBridge::WriteConfigSync(float display_w, float display_h, float scale_x, float scale_y) {
    if (m_cache_dir.empty()) return;

    if (display_w > 0.0f) m_display_w = display_w;
    if (display_h > 0.0f) m_display_h = display_h;
    if (scale_x > 0.0f) m_scale_x = scale_x;
    if (scale_y > 0.0f) m_scale_y = scale_y;

    long long now_ms = NowMillis();
    if (m_last_cfg_sync_ms != 0 && (now_ms - m_last_cfg_sync_ms) < 100) {
        return;
    }
    m_last_cfg_sync_ms = now_ms;

    const auto& s = Settings::Get();
    float fbw = m_display_w * m_scale_x;
    float fbh = m_display_h * m_scale_y;

    auto feat_on = [](MemoryCheats::Feature feat) -> int {
        return MemoryCheats::GetControl(feat).active ? 1 : 0;
    };

    float smooth_val = s.instant_aim ? 1.0f : s.aim_smoothness;
    float power_val  = s.instant_aim ? 1.0f : std::max(0.1f, std::min(1.0f, s.aim_power));

    int speed_on  = feat_on(MemoryCheats::Feature::SPEED_HACK);
    float speed_v = std::max(1.1f, std::min(30.0f, MemoryCheats::GetCurrentSpeedSetting()));
    int wall_on   = feat_on(MemoryCheats::Feature::WALL_HACK);
    int nofall_on = feat_on(MemoryCheats::Feature::NO_FALL_DAMAGE);
    int fly_on    = feat_on(MemoryCheats::Feature::FLY_HACK);
    int ug_on     = feat_on(MemoryCheats::Feature::UNDER_GROUND);
    int nograss_on= feat_on(MemoryCheats::Feature::NO_GRASS);
    int para_on   = feat_on(MemoryCheats::Feature::FAST_PARACHUTE);
    int cam_on    = feat_on(MemoryCheats::Feature::CAMERA_WIDE);
    int fog_on    = feat_on(MemoryCheats::Feature::CLEAR_FOG);
    int norecoil_on = feat_on(MemoryCheats::Feature::NO_RECOIL);
    int ipad_or_cam = (s.ipad_view || cam_on) ? 1 : 0;

    // 1. Atomic write aim_cfg.txt matching all 40 fields c[0..39] expected by neox_battle_payload.py
    std::string aim_cfg_path = m_cache_dir + "/aim_cfg.txt";
    std::string aim_tmp_path = aim_cfg_path + ".new";
    {
        std::ofstream aim_f(aim_tmp_path);
        if (aim_f.is_open()) {
            aim_f << (s.master_aim ? 1 : 0) << " "         // c[0]: en
                  << static_cast<int>(s.aim_trigger) << " " // c[1]: mode
                  << s.fov_radius << " "                    // c[2]: fov (points)
                  << smooth_val << " "                      // c[3]: smooth
                  << power_val << " "                       // c[4]: power
                  << (s.visible_check ? 1 : 0) << " "       // c[5]: vischk
                  << (s.ignore_knocked ? 1 : 0) << " "      // c[6]: ignkn
                  << static_cast<int>(s.target_bone) << " " // c[7]: bone
                  << m_display_w << " "                     // c[8]: W
                  << m_display_h << " "                     // c[9]: H
                  << s.aim_distance << " "                  // c[10]: maxd (aim)
                  << s.silent_distance << " "               // c[11]: maxd (silent)
                  << 0 << " "                               // c[12]
                  << (s.silent_aim ? 1 : 0) << " "          // c[13]: silent
                  << (s.silent_360 ? 1 : 0) << " "          // c[14]: silent360
                  << 1 << " "                               // c[15]
                  << fbw << " " << fbh << " "               // c[16..17]: fbw, fbh
                  << 0 << " " << 0 << " "                   // c[18..19]: vpx, vpy
                  << fbw << " " << fbh << " "               // c[20..21]: vpw, vph
                  << 0 << " " << 200 << " " << 1 << " "     // c[22..24]: autofire, range, flag
                  << (s.filter_bots ? 1 : 0) << " "         // c[25]: hide_bots
                  << (s.show_skeleton ? 1 : 0) << " "       // c[26]: draw_skeleton
                  << ipad_or_cam << " "                     // c[27]: ipad_on
                  << s.ipad_fov << " "                      // c[28]: ipad_fov
                  << speed_on << " "                        // c[29]: sp_on
                  << speed_v << " "                         // c[30]: sp_val
                  << wall_on << " "                         // c[31]: wh_on
                  << nofall_on << " "                       // c[32]: nf_on
                  << fly_on << " "                          // c[33]: fly_on
                  << ug_on << " "                           // c[34]: ug_on
                  << nograss_on << " "                      // c[35]: ng_on
                  << para_on << " "                         // c[36]: para_on
                  << cam_on << " "                          // c[37]: cam_on
                  << fog_on << " "                          // c[38]: fog_on
                  << norecoil_on << "\n";                   // c[39]: nr_on
        }
    }
    std::rename(aim_tmp_path.c_str(), aim_cfg_path.c_str());

    // 2. Atomic write vip_cfg.txt
    std::string vip_cfg_path = m_cache_dir + "/vip_cfg.txt";
    std::string vip_tmp_path = vip_cfg_path + ".new";
    {
        std::ofstream vip_f(vip_tmp_path);
        if (vip_f.is_open()) {
            vip_f << 0 << " " << 0 << " " << 0 << " "
                  << (s.ipad_view ? 1 : 0) << " "
                  << s.ipad_fov << " "
                  << norecoil_on << " "
                  << speed_on << " " << speed_v << "\n";
        }
    }
    std::rename(vip_tmp_path.c_str(), vip_cfg_path.c_str());
}

void PythonBridge::UpdateFromEspFile(float display_w, float display_h, float scale_x, float scale_y) {
    if (display_w > 0.0f) m_display_w = display_w;
    if (display_h > 0.0f) m_display_h = display_h;
    if (scale_x > 0.0f) m_scale_x = scale_x;
    if (scale_y > 0.0f) m_scale_y = scale_y;

    if (m_cache_dir.empty()) return;
    std::string esp_file = m_cache_dir + "/esp.txt";

#if defined(__APPLE__)
    struct stat st {};
    if (stat(esp_file.c_str(), &st) != 0) {
        return;
    }
    if (st.st_ino == m_last_ino &&
        st.st_mtimespec.tv_sec == m_last_mtime_sec &&
        st.st_mtimespec.tv_nsec == m_last_mtime_nsec) {
        return;
    }
    m_last_ino = static_cast<uint64_t>(st.st_ino);
    m_last_mtime_sec = st.st_mtimespec.tv_sec;
    m_last_mtime_nsec = st.st_mtimespec.tv_nsec;
#endif

    std::ifstream f(esp_file);
    if (!f.is_open()) return;

    std::string line;
    if (!std::getline(f, line)) return;

    // Header: entity_count, local_x, local_y, local_z, vx, vy, vz, dx, dy, dz, mode, bs, trig
    std::stringstream ss(line);
    int total_lines = 0;
    ss >> total_lines;

    if (total_lines <= 0) {
        std::lock_guard<std::mutex> lock(m_entity_mutex);
        m_entities.clear();
        m_entity_count = 0;
        m_bot_count = 0;
        m_player_count = 0;
        return;
    }

    ss >> m_local_pos.x >> m_local_pos.y >> m_local_pos.z;
    ss >> m_local_view_pos.x >> m_local_view_pos.y >> m_local_view_pos.z;
    ss >> m_local_view_dir.x >> m_local_view_dir.y >> m_local_view_dir.z;
    int esp_mode = 0;
    if (ss >> esp_mode) {
        m_esp_mode = esp_mode;
    }

    auto convert_screen_pt = [&](float& x, float& y) -> bool {
        if (!std::isfinite(x) || !std::isfinite(y)) return false;
        if (std::fabs(x) <= 2.0f && std::fabs(y) <= 2.0f) {
            x *= m_display_w;
            y *= m_display_h;
        } else {
            x /= m_scale_x;
            y /= m_scale_y;
        }
        return std::isfinite(x) && std::isfinite(y);
    };

    std::vector<PlayerEntity> entities;
    entities.reserve(std::min(total_lines, 256));

    int bot_count = 0;
    int player_count = 0;

    while (std::getline(f, line)) {
        if (line.empty() || line == "0") continue;

        PlayerEntity e;
        std::stringstream ls(line);

        // Format: fs.x, fs.y, hs.x, hs.y, on, p.x, p.y, p.z, vx, vy, vz,
        //         hp, mhp, enemy, vis, dist, kn, bot, team, sh, msh, vid, eid, nm|bones|weapon
        int on_val = 0;
        if (!(ls >> e.feet_screen.x >> e.feet_screen.y
                 >> e.head_screen.x >> e.head_screen.y
                 >> on_val
                 >> e.world_pos.x >> e.world_pos.y >> e.world_pos.z
                 >> e.view_pos.x >> e.view_pos.y >> e.view_pos.z
                 >> e.hp >> e.max_hp
                 >> e.is_enemy >> e.is_visible >> e.distance
                 >> e.is_knocked >> e.is_bot >> e.team_id
                 >> e.shield >> e.max_shield >> e.in_vehicle >> e.entity_id)) {
            continue;
        }

        if (!std::isfinite(e.distance) || e.distance < 0.0f) continue;
        // Convert RoS decimeter world units to meters (matches RyomenX parser)
        e.distance /= 10.0f;

        if (!convert_screen_pt(e.feet_screen.x, e.feet_screen.y) ||
            !convert_screen_pt(e.head_screen.x, e.head_screen.y)) {
            on_val = 0;
        }
        e.is_on_screen = (on_val == 1);

        // Read FULL remainder containing spaces in bone_text: "name|b0x y;b1x y...|weapon"
        std::string extra;
        std::getline(ls >> std::ws, extra);
        size_t pipe1 = extra.find('|');
        size_t pipe2 = extra.rfind('|');

        if (pipe1 != std::string::npos && pipe2 != std::string::npos && pipe2 > pipe1) {
            e.name = extra.substr(0, pipe1);
            std::string bone_str = extra.substr(pipe1 + 1, pipe2 - pipe1 - 1);
            e.weapon_kind = std::atoi(extra.substr(pipe2 + 1).c_str());

            std::stringstream bss(bone_str);
            std::string bone_item;
            int b_idx = 0;
            while (std::getline(bss, bone_item, ';') && b_idx < BONE_COUNT) {
                std::stringstream bis(bone_item);
                int b_valid = 0;
                float bx = 0.0f, by = 0.0f;
                if ((bis >> b_valid >> bx >> by) && b_valid == 1 && convert_screen_pt(bx, by)) {
                    e.bones[b_idx].valid = true;
                    e.bones[b_idx].x = bx;
                    e.bones[b_idx].y = by;
                } else {
                    e.bones[b_idx].valid = false;
                }
                b_idx++;
            }
        } else {
            e.name = e.is_bot ? "Bot" : "Player";
        }

        if (e.is_bot) bot_count++;
        else player_count++;

        entities.push_back(std::move(e));
    }

    {
        std::lock_guard<std::mutex> lock(m_entity_mutex);
        m_entities = std::move(entities);
        m_entity_count = static_cast<int>(m_entities.size());
        m_bot_count = bot_count;
        m_player_count = player_count;
    }
}

std::vector<PlayerEntity> PythonBridge::GetEntities() {
    std::lock_guard<std::mutex> lock(m_entity_mutex);
    return m_entities;
}

} // namespace RyomenX
