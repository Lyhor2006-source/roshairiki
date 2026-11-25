#pragma once

#include "Entity.hpp"
#include <vector>
#include <string>
#include <mutex>
#include <atomic>

namespace RyomenX {

class PythonBridge {
private:
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_payload_active{false};
    std::string m_cache_dir;
    std::mutex m_entity_mutex;
    std::mutex m_status_mutex;
    std::string m_runtime_stage = "Pending";
    std::vector<PlayerEntity> m_entities;
    OverlayMath::Vec3 m_local_pos;
    OverlayMath::Vec3 m_local_view_pos;
    OverlayMath::Vec3 m_local_view_dir;
    int m_entity_count = 0;
    int m_bot_count = 0;
    int m_player_count = 0;
    int m_esp_mode = 0;
    float m_display_w = 844.0f;
    float m_display_h = 390.0f;
    float m_scale_x = 2.0f;
    float m_scale_y = 2.0f;
    uint64_t m_last_ino = 0;
    long m_last_mtime_sec = 0;
    long m_last_mtime_nsec = 0;
    long long m_last_cfg_sync_ms = 0;

    void SetRuntimeStage(const std::string& stage);

    // Python 2.7 C-API dynamically resolved function pointers
    typedef int (*Py_IsInitialized_t)();
    typedef void* (*PyGILState_Ensure_t)();
    typedef void (*PyGILState_Release_t)(void*);
    typedef void* (*PyImport_AddModule_t)(const char*);
    typedef void* (*PyModule_GetDict_t)(void*);
    typedef void* (*PyRun_StringFlags_t)(const char*, int, void*, void*, void*);
    typedef void (*PyErr_Clear_t)();

    Py_IsInitialized_t p_Py_IsInitialized = nullptr;
    PyGILState_Ensure_t p_PyGILState_Ensure = nullptr;
    PyGILState_Release_t p_PyGILState_Release = nullptr;
    PyImport_AddModule_t p_PyImport_AddModule = nullptr;
    PyModule_GetDict_t p_PyModule_GetDict = nullptr;
    PyRun_StringFlags_t p_PyRun_StringFlags = nullptr;
    PyErr_Clear_t p_PyErr_Clear = nullptr;

public:
    static PythonBridge& Get() {
        static PythonBridge instance;
        return instance;
    }

    bool Initialize(const std::string& cache_dir);
    bool ResolvePythonSymbols();
    bool ExecutePayload(const std::string& python_code);
    void UpdateFromEspFile(float display_w = 0.0f, float display_h = 0.0f, float scale_x = 0.0f, float scale_y = 0.0f);
    void WriteConfigSync(float display_w = 0.0f, float display_h = 0.0f, float scale_x = 0.0f, float scale_y = 0.0f);

    // Data accessors for UI & Rendering
    std::vector<PlayerEntity> GetEntities();
    int GetEntityCount() const { return m_entity_count; }
    int GetPlayerCount() const { return m_player_count; }
    int GetBotCount() const { return m_bot_count; }
    int GetEspMode() const { return m_esp_mode; }
    std::string GetRuntimeStage();
    OverlayMath::Vec3 GetLocalPosition() const { return m_local_pos; }
    bool IsPayloadActive() const { return m_payload_active; }
};

} // namespace RyomenX
