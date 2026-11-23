#pragma once

#include "MachMemory.hpp"
#include <string>
#include <mutex>
#include <deque>
#include <thread>
#include <atomic>

namespace MemoryCheats {

enum class Feature : int {
    AKM_1HIT = 0,
    DESERT_EAGLE = 1,
    G18C = 2,
    M4A1_1HIT = 3,
    ONE_HIT_ALL = 4,
    NO_RECOIL = 5,
    AUTO_SCOPE = 6,
    HIGH_SCOPE = 7,
    FAST_SHOT = 8,
    FAST_RELOAD = 9,
    FAST_HEAL = 10,
    SPEED_HACK = 11,
    WALL_HACK = 12,
    NO_FALL_DAMAGE = 13,
    FLY_HACK = 14,
    UNDER_GROUND = 15,
    NO_GRASS = 16,
    FAST_PARACHUTE = 17,
    CAMERA_WIDE = 18,
    CLEAR_FOG = 19,
    FEATURE_COUNT = 20
};

struct FeatureControl {
    bool active = false;
    bool busy = false;
    bool restore_pending = false;
    size_t matches = 0;
    size_t modified = 0;
    std::string message = "OFF";
    MachMemory::PatchState patch_state;
    std::mutex mtx;
};

struct FeatureRequest {
    Feature feature;
    bool enable;
    float param;
};

class FeatureWorker {
private:
    std::thread m_thread;
    std::mutex m_queue_mutex;
    std::deque<FeatureRequest> m_queue;
    std::atomic<bool> m_running{false};

    void WorkerLoop();

public:
    FeatureWorker();
    ~FeatureWorker();

    void Submit(const FeatureRequest& req);
};

// Global control interfaces
void Initialize();
void Shutdown();
void RequestFeature(Feature feat, bool enable, float param = 0.0f);
FeatureControl& GetControl(Feature feat);
std::string GetFeatureStatus(Feature feat);
const char* GetFeatureLabel(Feature feat);
const char* GetFeatureScope(Feature feat);
float GetCurrentSpeedSetting();
void SetDiagnosticsDirectory(const std::string& dir);
void SaveDiagnostics();
void Tick();

// Feature implementations
void AKM1HitON();
void DesertEagleON();
void G18CON();
void M4A11HitON();
void OneHitAllGunON();
void NoRecoilON();
void AutoScopeON();
void HighScopeON();
void FastShotON();
void FastReloadON();
void FastHealON();
void SpeedHackON(float multiplier);
void WallHackON();
void NoFallDamageON();
void NoFallDamageOFF();
void FlyHackON();
void FlyHackOFF();
void UnderGroundON();
void NoGrassON();
void FastParachuteON();
void CameraWideON();
void ClearFogON();

} // namespace MemoryCheats
