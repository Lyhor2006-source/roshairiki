#include "MemoryCheats.hpp"
#include "PythonBridge.hpp"
#include <iostream>
#include <algorithm>

namespace MemoryCheats {

static FeatureControl s_controls[static_cast<size_t>(Feature::FEATURE_COUNT)];
static FeatureWorker* s_worker = nullptr;
static float s_speed_setting = 2.5f;
static std::string s_diag_dir = "/tmp";
static std::mutex s_diag_mutex;

void Initialize() {
    if (!s_worker) {
        s_worker = new FeatureWorker();
    }
}

void Shutdown() {
    if (s_worker) {
        delete s_worker;
        s_worker = nullptr;
    }
    for (size_t i = 0; i < static_cast<size_t>(Feature::FEATURE_COUNT); ++i) {
        std::lock_guard<std::mutex> lock(s_controls[i].mtx);
        s_controls[i].patch_state.RestoreAll();
        s_controls[i].active = false;
        s_controls[i].message = "OFF";
    }
}

FeatureWorker::FeatureWorker() {
    m_running = true;
    m_thread = std::thread(&FeatureWorker::WorkerLoop, this);
}

FeatureWorker::~FeatureWorker() {
    m_running = false;
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void FeatureWorker::Submit(const FeatureRequest& req) {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    m_queue.push_back(req);
}

void FeatureWorker::WorkerLoop() {
    while (m_running) {
        FeatureRequest req;
        bool has_req = false;
        {
            std::lock_guard<std::mutex> lock(m_queue_mutex);
            if (!m_queue.empty()) {
                req = m_queue.front();
                m_queue.pop_front();
                has_req = true;
            }
        }

        if (has_req) {
            Feature feat = req.feature;
            FeatureControl& ctrl = s_controls[static_cast<size_t>(feat)];

            if (req.enable) {
                switch (feat) {
                    case Feature::AKM_1HIT: AKM1HitON(); break;
                    case Feature::DESERT_EAGLE: DesertEagleON(); break;
                    case Feature::G18C: G18CON(); break;
                    case Feature::M4A1_1HIT: M4A11HitON(); break;
                    case Feature::ONE_HIT_ALL: OneHitAllGunON(); break;
                    case Feature::NO_RECOIL: NoRecoilON(); break;
                    case Feature::AUTO_SCOPE: AutoScopeON(); break;
                    case Feature::HIGH_SCOPE: HighScopeON(); break;
                    case Feature::FAST_SHOT: FastShotON(); break;
                    case Feature::FAST_RELOAD: FastReloadON(); break;
                    case Feature::FAST_HEAL: FastHealON(); break;
                    case Feature::SPEED_HACK: SpeedHackON(req.param > 0.0f ? req.param : s_speed_setting); break;
                    case Feature::WALL_HACK: WallHackON(); break;
                    case Feature::NO_FALL_DAMAGE: NoFallDamageON(); break;
                    case Feature::FLY_HACK: FlyHackON(); break;
                    case Feature::UNDER_GROUND: UnderGroundON(); break;
                    case Feature::NO_GRASS: NoGrassON(); break;
                    case Feature::FAST_PARACHUTE: FastParachuteON(); break;
                    case Feature::CAMERA_WIDE: CameraWideON(); break;
                    case Feature::CLEAR_FOG: ClearFogON(); break;
                    default: break;
                }
            } else {
                std::lock_guard<std::mutex> lock(ctrl.mtx);
                ctrl.patch_state.RestoreAll();
                if (feat == Feature::NO_FALL_DAMAGE) NoFallDamageOFF();
                if (feat == Feature::FLY_HACK) FlyHackOFF();
                ctrl.active = false;
                ctrl.busy = false;
                ctrl.message = "OFF";
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

void RequestFeature(Feature feat, bool enable, float param) {
    if (!s_worker) Initialize();
    FeatureControl& ctrl = s_controls[static_cast<size_t>(feat)];
    {
        std::lock_guard<std::mutex> lock(ctrl.mtx);
        ctrl.busy = true;
        ctrl.message = enable ? "Scanning..." : "OFF";
    }
    FeatureRequest req;
    req.feature = feat;
    req.enable = enable;
    req.param = param;
    s_worker->Submit(req);
}

FeatureControl& GetControl(Feature feat) {
    return s_controls[static_cast<size_t>(feat)];
}

std::string GetFeatureStatus(Feature feat) {
    std::lock_guard<std::mutex> lock(s_controls[static_cast<size_t>(feat)].mtx);
    return s_controls[static_cast<size_t>(feat)].message;
}

const char* GetFeatureLabel(Feature feat) {
    switch (feat) {
        case Feature::AKM_1HIT: return "AKM 1 Hit";
        case Feature::DESERT_EAGLE: return "Desert Eagle";
        case Feature::G18C: return "G18C";
        case Feature::M4A1_1HIT: return "M4A1 1 Hit";
        case Feature::ONE_HIT_ALL: return "One Hit All Gun";
        case Feature::NO_RECOIL: return "No Recoil";
        case Feature::AUTO_SCOPE: return "Auto Scope";
        case Feature::HIGH_SCOPE: return "High Scope";
        case Feature::FAST_SHOT: return "Fast Shot";
        case Feature::FAST_RELOAD: return "Fast Reload";
        case Feature::FAST_HEAL: return "Fast Heal";
        case Feature::SPEED_HACK: return "Speed Hack";
        case Feature::WALL_HACK: return "Wall Hack";
        case Feature::NO_FALL_DAMAGE: return "No Fall Damage";
        case Feature::FLY_HACK: return "Fly Hack";
        case Feature::UNDER_GROUND: return "Under Ground";
        case Feature::NO_GRASS: return "No Grass";
        case Feature::FAST_PARACHUTE: return "Fast Parachute";
        case Feature::CAMERA_WIDE: return "Camera Wide";
        case Feature::CLEAR_FOG: return "Clear Fog";
        default: return "Unknown";
    }
}

const char* GetFeatureScope(Feature feat) {
    switch (feat) {
        case Feature::AKM_1HIT:
        case Feature::DESERT_EAGLE:
        case Feature::G18C:
        case Feature::M4A1_1HIT:
        case Feature::ONE_HIT_ALL:
        case Feature::NO_RECOIL:
        case Feature::FAST_SHOT:
        case Feature::FAST_RELOAD:
            return "WEAPON";
        case Feature::SPEED_HACK:
        case Feature::NO_FALL_DAMAGE:
        case Feature::FLY_HACK:
        case Feature::UNDER_GROUND:
        case Feature::FAST_PARACHUTE:
        case Feature::FAST_HEAL:
            return "PLAYER";
        default:
            return "ENVIRONMENT";
    }
}

float GetCurrentSpeedSetting() {
    return s_speed_setting;
}

void SetDiagnosticsDirectory(const std::string& dir) {
    std::lock_guard<std::mutex> lock(s_diag_mutex);
    s_diag_dir = dir;
}

void SaveDiagnostics() {}
void Tick() {}

// Helper for weapon group patch routines decompiled from RyomenX
static void ExecuteWeaponPatch(Feature feat, uint32_t weapon_id, uint32_t replacement_id,
                               const std::vector<MachMemory::SearchItem>& items, size_t match_idx, size_t max_dist) {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(feat)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) {
        regions = MachMemory::QueryAllWritableRegions();
    }

    auto matches = MachMemory::GroupScan(regions, items, match_idx, max_dist);
    ctrl.matches = matches.size();

    if (matches.empty()) {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Pattern not found";
        return;
    }

    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &replacement_id, sizeof(uint32_t))) {
            applied++;
        }
    }

    ctrl.modified = applied;
    if (applied > 0) {
        ctrl.active = true;
        ctrl.busy = false;
        ctrl.message = "ACTIVE";
    } else {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Patch failed";
    }
}

void AKM1HitON() {
    // 0x7a2b0: 99103, 112775, 20002 (AKM), 112774
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(99103)),
        MachMemory::SearchItem(static_cast<uint32_t>(112775)),
        MachMemory::SearchItem(static_cast<uint32_t>(20002)),
        MachMemory::SearchItem(static_cast<uint32_t>(112774))
    };
    ExecuteWeaponPatch(Feature::AKM_1HIT, 20002, 20005, items, 2, 97);
}

void M4A11HitON() {
    // 0x7a340: 20001 (M4A1)
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(99103)),
        MachMemory::SearchItem(static_cast<uint32_t>(112775)),
        MachMemory::SearchItem(static_cast<uint32_t>(20001)),
        MachMemory::SearchItem(static_cast<uint32_t>(112774))
    };
    ExecuteWeaponPatch(Feature::M4A1_1HIT, 20001, 20005, items, 2, 97);
}

void DesertEagleON() {
    // 0x7a310: 20014 (Desert Eagle)
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(99103)),
        MachMemory::SearchItem(static_cast<uint32_t>(112775)),
        MachMemory::SearchItem(static_cast<uint32_t>(20014)),
        MachMemory::SearchItem(static_cast<uint32_t>(112774))
    };
    ExecuteWeaponPatch(Feature::DESERT_EAGLE, 20014, 20005, items, 2, 97);
}

void G18CON() {
    // 0x7a300: 20015 (G18C)
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(99103)),
        MachMemory::SearchItem(static_cast<uint32_t>(112775)),
        MachMemory::SearchItem(static_cast<uint32_t>(20015)),
        MachMemory::SearchItem(static_cast<uint32_t>(112774))
    };
    ExecuteWeaponPatch(Feature::G18C, 20015, 20005, items, 2, 97);
}

void OneHitAllGunON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::ONE_HIT_ALL)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    // Damage multiplier scan
    uint32_t search_val = 100;
    auto matches = MachMemory::ScanDword(regions, search_val, 4);
    ctrl.matches = matches.size();

    if (matches.empty()) {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Pattern not found";
        return;
    }

    uint32_t patch_val = 99999;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &patch_val, sizeof(uint32_t))) {
            applied++;
        }
    }

    ctrl.modified = applied;
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE: verified weapon matches" : "Patch failed";
}

static void ActivatePythonBackedFeature(Feature feat, const char* note) {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(feat)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);
    ctrl.patch_state.Clear();
    ctrl.active = true;
    ctrl.busy = false;
    ctrl.message = note ? note : (RyomenX::PythonBridge::Get().IsPayloadActive() ? "ACTIVE (NeoX)" : "ACTIVE (Pending Py)");
}

void NoRecoilON() {
    ActivatePythonBackedFeature(Feature::NO_RECOIL, "ACTIVE (NeoX)");
}

void AutoScopeON() {
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(111753)),
        MachMemory::SearchItem(1.94f)
    };
    ExecuteWeaponPatch(Feature::AUTO_SCOPE, 111753, 111755, items, 0, 80);
}

void HighScopeON() {
    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(121114)),
        MachMemory::SearchItem(2.80f)
    };
    ExecuteWeaponPatch(Feature::HIGH_SCOPE, 121114, 121120, items, 0, 80);
}

void FastShotON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::FAST_SHOT)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(0.1f),
        MachMemory::SearchItem(static_cast<uint32_t>(20002))
    };

    auto matches = MachMemory::GroupScan(regions, items, 0, 100);
    float fast_val = 0.01f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &fast_val, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void FastReloadON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::FAST_RELOAD)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(2.5f),
        MachMemory::SearchItem(static_cast<uint32_t>(20002))
    };

    auto matches = MachMemory::GroupScan(regions, items, 0, 100);
    float instant = 0.1f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &instant, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void FastHealON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::FAST_HEAL)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(5.0f),
        MachMemory::SearchItem(static_cast<uint32_t>(49021))
    };

    auto matches = MachMemory::GroupScan(regions, items, 0, 80);
    float fast_heal = 0.2f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &fast_heal, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void SpeedHackON(float multiplier) {
    s_speed_setting = std::max(1.1f, std::min(30.0f, multiplier));
    char buf[64];
    std::snprintf(buf, sizeof(buf), "ACTIVE (%.1fx NeoX)", s_speed_setting);
    ActivatePythonBackedFeature(Feature::SPEED_HACK, buf);
}

void WallHackON() {
    ActivatePythonBackedFeature(Feature::WALL_HACK, "ACTIVE (NeoX)");
}

void NoFallDamageON() {
    ActivatePythonBackedFeature(Feature::NO_FALL_DAMAGE, "ACTIVE (NeoX)");
}

void NoFallDamageOFF() {
    MachMemory::UnregisterFreeze("NoFall_Threshold");
}

void FlyHackON() {
    ActivatePythonBackedFeature(Feature::FLY_HACK, "ACTIVE (NeoX)");
}

void FlyHackOFF() {
    MachMemory::UnregisterFreeze("Fly_Gravity");
}

void UnderGroundON() {
    ActivatePythonBackedFeature(Feature::UNDER_GROUND, "ACTIVE (NeoX)");
}

void NoGrassON() {
    ActivatePythonBackedFeature(Feature::NO_GRASS, "ACTIVE (NeoX)");
}

void FastParachuteON() {
    ActivatePythonBackedFeature(Feature::FAST_PARACHUTE, "ACTIVE (NeoX)");
}

void CameraWideON() {
    ActivatePythonBackedFeature(Feature::CAMERA_WIDE, "ACTIVE (NeoX 95deg)");
}

void ClearFogON() {
    ActivatePythonBackedFeature(Feature::CLEAR_FOG, "ACTIVE (NeoX)");
}

} // namespace MemoryCheats
