#include "PythonBridge.hpp"
#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

#if __has_include("EmbeddedPayload.hpp")
#include "EmbeddedPayload.hpp"
#define HAS_EMBEDDED_PAYLOAD 1
#else
#define HAS_EMBEDDED_PAYLOAD 0
#endif

#if defined(__APPLE__)
#include <dlfcn.h>
#include <mach-o/dyld.h>
#endif

namespace RyomenX {

// Verified RoS 1.610637.617289 arm64 embedded Python offsets from RyomenX binary analysis
// In the client binary (base 0x100000000):
static const uintptr_t ROS_OFF_PY_ISINITIALIZED  = 0x1d05a24;
static const uintptr_t ROS_OFF_PYGILSTATE_ENSURE = 0x1d05158;
static const uintptr_t ROS_OFF_PYGILSTATE_RELEASE= 0x1d051dc;
static const uintptr_t ROS_OFF_PYIMPORT_ADDMODULE= 0x1cf8fd4;
static const uintptr_t ROS_OFF_PYMODULE_GETDICT  = 0x1c79884;
static const uintptr_t ROS_OFF_PYRUN_STRINGFLAGS = 0x1d0689c;
static const uintptr_t ROS_OFF_PYERR_CLEAR       = 0x1cf17f0;

bool PythonBridge::Initialize(const std::string& cache_dir) {
    m_cache_dir = cache_dir;

    // Retry symbol resolution with backoff: Python may not be loaded yet at inject time
    bool symbols_ok = false;
    for (int attempt = 0; attempt < 12; ++attempt) {
        if (ResolvePythonSymbols()) {
            symbols_ok = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    if (!symbols_ok) {
        std::cerr << "[PythonBridge] Failed to resolve Python runtime symbols after retries\n";
        m_initialized = true;
        return false;
    }

    m_initialized = true;

    // Wait until Python is actually initialized by the game engine
    for (int attempt = 0; attempt < 20; ++attempt) {
        if (p_Py_IsInitialized && p_Py_IsInitialized()) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
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

    // Allow user override if neox_override.py exists in cache dir
    std::string override_path = m_cache_dir + "/neox_override.py";
    std::ifstream ov_file(override_path);
    if (ov_file.is_open()) {
        std::stringstream buffer;
        buffer << ov_file.rdbuf();
        std::string ov_code = buffer.str();
        if (!ov_code.empty()) payload_code = ov_code;
    }

    if (!payload_code.empty()) {
        m_payload_active = ExecutePayload(payload_code);
    }

    // Background watcher: ensure Python payload executes once runtime is ready
    std::thread([this, payload_code]() {
        for (int i = 0; i < 30; ++i) {
            if (m_payload_active) break;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            if (!m_payload_active && !payload_code.empty()) {
                m_payload_active = ExecutePayload(payload_code);
            }
        }
    }).detach();

    return true;
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

    // 3. Fallback: If stripped, resolve directly via RoS client Mach-O ASLR slide (RyomenX original mechanism)
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    if (slide != 0 || _dyld_get_image_header(0) != nullptr) {
        p_Py_IsInitialized = reinterpret_cast<Py_IsInitialized_t>(slide + ROS_OFF_PY_ISINITIALIZED);
        p_PyGILState_Ensure = reinterpret_cast<PyGILState_Ensure_t>(slide + ROS_OFF_PYGILSTATE_ENSURE);
        p_PyGILState_Release = reinterpret_cast<PyGILState_Release_t>(slide + ROS_OFF_PYGILSTATE_RELEASE);
        p_PyImport_AddModule = reinterpret_cast<PyImport_AddModule_t>(slide + ROS_OFF_PYIMPORT_ADDMODULE);
        p_PyModule_GetDict = reinterpret_cast<PyModule_GetDict_t>(slide + ROS_OFF_PYMODULE_GETDICT);
        p_PyRun_StringFlags = reinterpret_cast<PyRun_StringFlags_t>(slide + ROS_OFF_PYRUN_STRINGFLAGS);
        p_PyErr_Clear = reinterpret_cast<PyErr_Clear_t>(slide + ROS_OFF_PYERR_CLEAR);
        return true;
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
        return false;
    }

    void* gstate = p_PyGILState_Ensure ? p_PyGILState_Ensure() : nullptr;
    void* main_module = p_PyImport_AddModule ? p_PyImport_AddModule("__main__") : nullptr;
    void* global_dict = (main_module && p_PyModule_GetDict) ? p_PyModule_GetDict(main_module) : nullptr;

    if (!global_dict || !p_PyRun_StringFlags) {
        if (gstate && p_PyGILState_Release) p_PyGILState_Release(gstate);
        return false;
    }

    // Wrap payload identical to RyomenX production wrapper with traceback capture
    std::string wrapped = "_RYOMENX_DIR = '" + m_cache_dir + "'\n"
                          "import traceback\n"
                          "try:\n"
                          + python_code + "\n"
                          "except Exception:\n"
                          "    open(_RYOMENX_DIR + '/payload-error.txt', 'w').write(traceback.format_exc())\n"
                          "    raise\n";

    // Py_file_input = 257 (0x101)
    void* result = p_PyRun_StringFlags(wrapped.c_str(), 257, global_dict, global_dict, nullptr);
    if (!result && p_PyErr_Clear) {
        p_PyErr_Clear();
    }

    if (gstate && p_PyGILState_Release) {
        p_PyGILState_Release(gstate);
    }
    m_payload_active = (result != nullptr);
    return m_payload_active;
#else
    m_payload_active = true;
    return true;
#endif
}

void PythonBridge::WriteConfigSync() {
    if (m_cache_dir.empty()) return;
    const auto& s = Settings::Get();

    // 1. Write aim_cfg.txt for Python aimbot loop
    std::string aim_cfg_path = m_cache_dir + "/aim_cfg.txt";
    std::ofstream aim_f(aim_cfg_path);
    if (aim_f.is_open()) {
        aim_f << (s.master_aim ? 1 : 0) << " "
              << static_cast<int>(s.aim_trigger) << " "
              << (s.silent_aim ? 1 : 0) << " "
              << (s.silent_360 ? 1 : 0) << " "
              << (s.visible_check ? 1 : 0) << " "
              << (s.ignore_knocked ? 1 : 0) << " "
              << static_cast<int>(s.target_bone) << " "
              << s.fov_radius << " "
              << s.aim_distance << " "
              << s.silent_distance << " "
              << s.aim_power << " "
              << s.aim_smoothness << " "
              << (s.filter_bots ? 1 : 0) << " "
              << (s.show_skeleton ? 1 : 0) << "\n";
    }

    // 2. Write vip_cfg.txt for iPad FOV and miscellaneous toggles
    std::string vip_cfg_path = m_cache_dir + "/vip_cfg.txt";
    std::ofstream vip_f(vip_cfg_path);
    if (vip_f.is_open()) {
        vip_f << (s.ipad_view ? 1 : 0) << " "
              << s.ipad_fov << "\n";
    }
}

void PythonBridge::UpdateFromEspFile() {
    std::string esp_file = m_cache_dir + "/esp.txt";
    std::ifstream f(esp_file);
    if (!f.is_open()) return;

    std::string line;
    if (!std::getline(f, line)) return;

    // Header: entity_count, local_x, local_y, local_z, vx, vy, vz, dx, dy, dz, mode, bs, trig
    std::stringstream ss(line);
    int total_lines = 0;
    ss >> total_lines;
    ss >> m_local_pos.x >> m_local_pos.y >> m_local_pos.z;
    ss >> m_local_view_pos.x >> m_local_view_pos.y >> m_local_view_pos.z;
    ss >> m_local_view_dir.x >> m_local_view_dir.y >> m_local_view_dir.z;

    std::vector<PlayerEntity> entities;
    entities.reserve(total_lines);

    int bot_count = 0;
    int player_count = 0;

    while (std::getline(f, line)) {
        if (line.empty() || line == "0") continue;

        PlayerEntity e;
        std::stringstream ls(line);

        // Format: fs.x, fs.y, hs.x, hs.y, on, p.x, p.y, p.z, vx, vy, vz,
        //         hp, mhp, enemy, vis, dist, kn, bot, team, sh, msh, vid, eid, nm|bones|weapon
        int on_val = 0;
        ls >> e.feet_screen.x >> e.feet_screen.y
           >> e.head_screen.x >> e.head_screen.y
           >> on_val
           >> e.world_pos.x >> e.world_pos.y >> e.world_pos.z
           >> e.view_pos.x >> e.view_pos.y >> e.view_pos.z
           >> e.hp >> e.max_hp
           >> e.is_enemy >> e.is_visible >> e.distance
           >> e.is_knocked >> e.is_bot >> e.team_id
           >> e.shield >> e.max_shield >> e.in_vehicle >> e.entity_id;

        e.is_on_screen = (on_val == 1);

        // Remainder contains: "name|bone1;bone2...|weapon"
        std::string extra;
        ls >> extra;
        size_t pipe1 = extra.find('|');
        size_t pipe2 = extra.rfind('|');

        if (pipe1 != std::string::npos && pipe2 != std::string::npos && pipe2 > pipe1) {
            e.name = extra.substr(0, pipe1);
            std::string bone_str = extra.substr(pipe1 + 1, pipe2 - pipe1 - 1);
            e.weapon_kind = std::atoi(extra.substr(pipe2 + 1).c_str());

            // Parse bone string: "valid x y;valid x y;..."
            std::stringstream bss(bone_str);
            std::string bone_item;
            int b_idx = 0;
            while (std::getline(bss, bone_item, ';') && b_idx < BONE_COUNT) {
                std::stringstream bis(bone_item);
                int b_valid = 0;
                bis >> b_valid >> e.bones[b_idx].x >> e.bones[b_idx].y;
                e.bones[b_idx].valid = (b_valid == 1);
                b_idx++;
            }
        } else {
            e.name = e.is_bot ? "Bot" : "Player";
        }

        if (e.is_bot) bot_count++;
        else player_count++;

        entities.push_back(e);
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
