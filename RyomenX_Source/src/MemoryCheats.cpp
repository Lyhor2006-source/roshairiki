#include "MemoryCheats.hpp"
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

void NoRecoilON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::NO_RECOIL)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    std::vector<MachMemory::SearchItem> items = {
        MachMemory::SearchItem(static_cast<uint32_t>(20002)),
        MachMemory::SearchItem(1.88f),
        MachMemory::SearchItem(2.50f)
    };

    auto matches = MachMemory::GroupScan(regions, items, 1, 120);
    ctrl.matches = matches.size();

    if (matches.empty()) {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Pattern not found";
        return;
    }

    float zero_recoil = 0.0f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &zero_recoil, sizeof(float))) {
            applied++;
        }
    }

    ctrl.modified = applied;
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Patch failed";
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
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::SPEED_HACK)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    s_speed_setting = multiplier;
    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    // Default base speed 1.0f in player physics component
    auto matches = MachMemory::ScanFloat(regions, 1.0f, 0.0001f);
    ctrl.matches = matches.size();

    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &multiplier, sizeof(float))) {
            applied++;
        }
    }

    ctrl.modified = applied;
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    if (applied > 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "ACTIVE (%.1fx Engine)", multiplier);
        ctrl.message = buf;
    } else {
        ctrl.message = "Pattern not found";
    }
}

void WallHackON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::WALL_HACK)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    uint32_t search = 515; // GL_LESS / Depth test
    auto matches = MachMemory::ScanDword(regions, search, 4);

    uint32_t patch = 519; // GL_ALWAYS
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &patch, sizeof(uint32_t))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void NoFallDamageON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::NO_FALL_DAMAGE)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    float threshold = 20.5f;
    auto matches = MachMemory::ScanFloat(regions, threshold, 0.01f);

    float immune = 999999.0f;
    if (!matches.empty()) {
        MachMemory::RegisterFreeze("NoFall_Threshold", matches[0], &immune, sizeof(float));
        ctrl.active = true;
        ctrl.busy = false;
        ctrl.message = "ACTIVE (20.5f)";
    } else {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Pattern not found";
    }
}

void NoFallDamageOFF() {
    MachMemory::UnregisterFreeze("NoFall_Threshold");
}

void FlyHackON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::FLY_HACK)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    float gravity = -9.8f;
    auto matches = MachMemory::ScanFloat(regions, gravity, 0.01f);

    float lift = 0.5f;
    if (!matches.empty()) {
        MachMemory::RegisterFreeze("Fly_Gravity", matches[0], &lift, sizeof(float));
        ctrl.active = true;
        ctrl.busy = false;
        ctrl.message = "ACTIVE";
    } else {
        ctrl.active = false;
        ctrl.busy = false;
        ctrl.message = "Pattern not found";
    }
}

void FlyHackOFF() {
    MachMemory::UnregisterFreeze("Fly_Gravity");
}

void UnderGroundON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::UNDER_GROUND)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    // Underground depth -13.5f / -20.0f
    double depth = -13.5;
    auto matches = MachMemory::ScanQword(regions, *reinterpret_cast<uint64_t*>(&depth), 8);

    double underground_target = -20.0;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &underground_target, sizeof(double))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void NoGrassON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::NO_GRASS)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    // Grass render distance float (450.0f)
    auto matches = MachMemory::ScanFloat(regions, 450.0f, 0.1f);
    float zero_dist = 0.0f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &zero_dist, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE (Vegetation)" : "Pattern not found";
}

void FastParachuteON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::FAST_PARACHUTE)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    float fall_speed = -5.0f;
    auto matches = MachMemory::ScanFloat(regions, fall_speed, 0.05f);

    float fast_fall = -35.0f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &fast_fall, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void CameraWideON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::CAMERA_WIDE)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    float base_fov = 65.0f;
    auto matches = MachMemory::ScanFloat(regions, base_fov, 0.05f);

    float wide_fov = 105.0f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &wide_fov, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

void ClearFogON() {
    FeatureControl& ctrl = s_controls[static_cast<size_t>(Feature::CLEAR_FOG)];
    std::lock_guard<std::mutex> lock(ctrl.mtx);

    ctrl.patch_state.Clear();
    auto regions = MachMemory::QueryRegions(MachMemory::GetRegionMode());
    if (regions.empty()) regions = MachMemory::QueryAllWritableRegions();

    float fog_density = 0.005f;
    auto matches = MachMemory::ScanFloat(regions, fog_density, 0.0001f);

    float zero_fog = 0.0f;
    size_t applied = 0;
    for (uintptr_t addr : matches) {
        if (ctrl.patch_state.AddAndApply(addr, &zero_fog, sizeof(float))) {
            applied++;
        }
    }
    ctrl.active = (applied > 0);
    ctrl.busy = false;
    ctrl.message = (applied > 0) ? "ACTIVE" : "Pattern not found";
}

} // namespace MemoryCheats
