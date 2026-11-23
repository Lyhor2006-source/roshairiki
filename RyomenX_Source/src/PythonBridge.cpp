#include "PythonBridge.hpp"
#include "Config.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

#if defined(__APPLE__)
#include <dlfcn.h>
#endif

namespace RyomenX {

bool PythonBridge::Initialize(const std::string& cache_dir) {
    m_cache_dir = cache_dir;

    if (!ResolvePythonSymbols()) {
        std::cerr << "[PythonBridge] Failed to resolve Python runtime symbols\n";
        return false;
    }

    m_initialized = true;

    // Load payload script from bundled file or default location
    std::string script_path = m_cache_dir + "/neox_battle_payload.py";
    std::ifstream file(script_path);
    if (file.is_open()) {
        std::stringstream buffer;
        buffer << file.rdbuf();
        ExecutePayload(buffer.str());
    }

    return true;
}

bool PythonBridge::ResolvePythonSymbols() {
#if defined(__APPLE__)
    void* handle = dlopen(NULL, RTLD_NOW);
    if (!handle) return false;

    p_Py_IsInitialized = reinterpret_cast<Py_IsInitialized_t>(dlsym(handle, "Py_IsInitialized"));
    p_PyGILState_Ensure = reinterpret_cast<PyGILState_Ensure_t>(dlsym(handle, "PyGILState_Ensure"));
    p_PyGILState_Release = reinterpret_cast<PyGILState_Release_t>(dlsym(handle, "PyGILState_Release"));
    p_PyImport_AddModule = reinterpret_cast<PyImport_AddModule_t>(dlsym(handle, "PyImport_AddModule"));
    p_PyModule_GetDict = reinterpret_cast<PyModule_GetDict_t>(dlsym(handle, "PyModule_GetDict"));
    p_PyRun_StringFlags = reinterpret_cast<PyRun_StringFlags_t>(dlsym(handle, "PyRun_StringFlags"));
    p_PyErr_Clear = reinterpret_cast<PyErr_Clear_t>(dlsym(handle, "PyErr_Clear"));

    return (p_Py_IsInitialized && p_PyGILState_Ensure && p_PyGILState_Release &&
            p_PyImport_AddModule && p_PyModule_GetDict && p_PyRun_StringFlags);
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

    void* gstate = p_PyGILState_Ensure();
    void* main_module = p_PyImport_AddModule("__main__");
    void* global_dict = p_PyModule_GetDict(main_module);

    std::string setup_script = "_RYOMENX_DIR = '" + m_cache_dir + "'\n" + python_code;

    // Py_file_input = 257
    void* result = p_PyRun_StringFlags(setup_script.c_str(), 257, global_dict, global_dict, nullptr);
    if (!result && p_PyErr_Clear) {
        p_PyErr_Clear();
    }

    p_PyGILState_Release(gstate);
    m_payload_active = (result != nullptr);
    return m_payload_active;
#else
    m_payload_active = true;
    return true;
#endif
}

void PythonBridge::WriteConfigSync() {
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
