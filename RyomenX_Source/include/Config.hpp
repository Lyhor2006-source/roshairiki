#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace RyomenX {

enum class BoxStyle : int {
    Box2D = 0,
    Corner = 1,
    Box3D = 2
};

enum class LineOrigin : int {
    Top = 0,
    Crosshair = 1,
    Bottom = 2,
    FloatingBar = 3
};

enum class TargetBone : int {
    Head = 0,
    Chest = 1,
    Pelvis = 2
};

enum class AimTrigger : int {
    Always = 0,
    Scoped = 1,
    Firing = 2,
    ScopedAndFiring = 3
};

struct ColorRGBA {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;

    uint32_t ToU32() const {
        uint32_t R = static_cast<uint32_t>(r * 255.0f) & 0xFF;
        uint32_t G = static_cast<uint32_t>(g * 255.0f) & 0xFF;
        uint32_t B = static_cast<uint32_t>(b * 255.0f) & 0xFF;
        uint32_t A = static_cast<uint32_t>(a * 255.0f) & 0xFF;
        return (A << 24) | (B << 16) | (G << 8) | R;
    }
};

struct Settings {
    // Menu & State
    bool menu_open = false;
    bool show_floating_bar = true;
    float menu_scale = 1.0f;
    float bar_pos_x = 100.0f;
    float bar_pos_y = 100.0f;
    int current_tab = 0;

    // Visuals (ESP)
    bool master_esp = true;
    int esp_style = 0; // 0: CrownPulse (Modern), 1: VideoTrace (Streamlined)
    BoxStyle box_style = BoxStyle::Corner;
    bool show_box = true;
    bool show_skeleton = true;
    bool show_health = true;
    bool show_name = true;
    bool show_team = true;
    bool show_player_id = false;
    bool show_lines = true;
    LineOrigin line_origin = LineOrigin::Bottom;
    bool show_distance = true;
    bool show_arrows = true;
    bool show_hud_count = true;
    bool filter_bots = false;
    float esp_range = 500.0f;

    // Advanced & Colors
    ColorRGBA enemy_color = {1.0f, 0.2f, 0.2f, 1.0f};
    ColorRGBA bot_color = {0.2f, 0.7f, 1.0f, 1.0f};
    ColorRGBA skeleton_color = {1.0f, 1.0f, 1.0f, 1.0f};
    ColorRGBA line_color = {1.0f, 0.9f, 0.0f, 0.8f};

    // iPad View / Camera FOV
    bool ipad_view = false;
    float ipad_fov = 95.0f;

    // Aimbot
    bool master_aim = false;
    AimTrigger aim_trigger = AimTrigger::Always;
    bool silent_aim = false;
    bool silent_360 = false;
    bool instant_aim = false;
    bool visible_check = true;
    bool ignore_knocked = true;
    bool target_bots = true;
    bool show_fov_circle = true;
    TargetBone target_bone = TargetBone::Head;
    float fov_radius = 120.0f;
    float aim_distance = 300.0f;
    float aim_power = 1.5f;
    float silent_distance = 150.0f;
    float aim_smoothness = 3.0f;

    // HUD / Overlay Settings
    bool screen_crosshair = false;
    bool show_watermark = true;

    // Memory Region
    int selected_region_mode = 0; // 0: OTHER, 1: ANONYMOUS, 2: C_ALLOC, 3: CODE_APP

    // Singleton
    static Settings& Get() {
        static Settings instance;
        return instance;
    }

    void LoadDefaults();
    void LoadFromPreferences();
    void SaveToPreferences();
};

} // namespace RyomenX
