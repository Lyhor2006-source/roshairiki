#include "Menu.h"
#include "Config.hpp"
#include "MemoryCheats.hpp"
#include "PythonBridge.hpp"
#include "StealthBypass.hpp"

#include <cstdio>
#include <cmath>
#include <algorithm>
#include <string>

template <typename T>
static inline constexpr const T& clamp_val(const T& v, const T& lo, const T& hi) {
    return (v < lo) ? lo : ((hi < v) ? hi : v);
}

namespace Menu
{
    using namespace RyomenX;

    static const char* kTitle = "ECLIPSE VIP";
    static const char* kVersion = "v1.0 (arm64)";

    static float  g_scale  = 1.0f;
    static ImVec4 g_accent = ImVec4(0.20f, 0.45f, 1.00f, 1.00f);

    // Track bounding boxes for iOS touch passthrough
    static ImVec2 g_window_pos(100.0f, 100.0f);
    static ImVec2 g_window_size(680.0f, 440.0f);
    static ImVec2 g_hud_pos(20.0f, 20.0f);
    static ImVec2 g_hud_size(240.0f, 36.0f);

    Config& Cfg()
    {
        static Config cfg;
        return cfg;
    }

    // Synchronize Menu::Cfg() to RyomenX::Settings so ESP overlay reads directly
    static void SyncToRyomenX()
    {
        auto& s = RyomenX::Settings::Get();
        const auto& c = Cfg();

        s.master_esp = c.espEnabled;
        s.show_box = c.espBox;
        s.show_skeleton = c.espSkeleton;
        s.show_name = c.espName;
        s.show_health = c.espHealth;
        s.show_distance = c.espDistance;
        s.show_lines = c.espLines;
        s.line_origin = static_cast<RyomenX::LineOrigin>(c.espLineOrigin);
        s.show_team = c.espShowTeam;
        s.show_arrows = c.espOffscreenArrows;
        s.filter_bots = c.espFilterBots;
        s.box_style = static_cast<RyomenX::BoxStyle>(c.espBoxStyle);
        s.esp_range = c.espMaxDistance;

        s.enemy_color = { c.espBoxColor[0], c.espBoxColor[1], c.espBoxColor[2], c.espBoxColor[3] };
        s.bot_color = { c.espBotColor[0], c.espBotColor[1], c.espBotColor[2], c.espBotColor[3] };
        s.skeleton_color = { c.espSkeletonColor[0], c.espSkeletonColor[1], c.espSkeletonColor[2], c.espSkeletonColor[3] };
        s.line_color = { c.espLineColor[0], c.espLineColor[1], c.espLineColor[2], c.espLineColor[3] };

        s.ipad_view = c.ipadView;
        s.ipad_fov = c.ipadFov;

        s.master_aim = c.aimEnabled;
        s.silent_aim = c.silentAim;
        s.silent_360 = c.aim360;
        s.instant_aim = c.instantAim;
        s.aim_power = c.aimStrength / 30.0f;
        s.target_bone = static_cast<RyomenX::TargetBone>(c.aimBone);
        s.aim_trigger = static_cast<RyomenX::AimTrigger>(c.aimActivation);
        s.fov_radius = c.aimFov * 4.0f;
        s.aim_smoothness = c.aimSmooth;
        s.aim_distance = c.aimDistance;
        s.show_fov_circle = c.aimDrawFov;
        s.visible_check = c.aimVisibleOnly;
        s.ignore_knocked = c.aimIgnoreKnocked;
        s.target_bots = c.aimTargetBots;

        s.screen_crosshair = c.screenCrosshair;
        s.show_watermark = c.showWatermark;
        s.menu_scale = c.menuScale;
    }

    // ---------- helpers ----------

    static ImU32 Col(const ImVec4& c, float alphaMul = 1.0f)
    {
        return ImGui::ColorConvertFloat4ToU32(
            ImVec4(c.x, c.y, c.z, c.w * alphaMul * ImGui::GetStyle().Alpha));
    }

    static ImU32 A(ImU32 col)
    {
        ImVec4 v = ImGui::ColorConvertU32ToFloat4(col);
        v.w *= ImGui::GetStyle().Alpha;
        return ImGui::ColorConvertFloat4ToU32(v);
    }

    static ImVec4 Gray(float v, float a = 1.0f) { return ImVec4(v, v, v, a); }

    static ImVec4 Mix(const ImVec4& a, const ImVec4& b, float t)
    {
        return ImVec4(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                      a.z + (b.z - a.z) * t, a.w + (b.w - a.w) * t);
    }

    static float RightPad() { return ImGui::GetFontSize() * 0.8f; }

    static void Section(const char* title)
    {
        const float u = ImGui::GetFontSize();
        ImGui::Dummy(ImVec2(0, u * 0.35f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
        ImGui::Dummy(ImVec2(0, u * 0.15f));
    }

    // Animated pill switch: label on left, rounded pill toggle on right
    static bool Toggle(const char* label, bool* v)
    {
        ImGui::PushID(label);

        const float rowW = ImGui::GetContentRegionAvail().x - RightPad();
        const float rowH = ImGui::GetFrameHeight() * 1.15f;
        const float h    = rowH * 0.68f;
        const float w    = h * 1.8f;

        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool pressed = ImGui::InvisibleButton("##toggle", ImVec2(rowW, rowH));
        if (pressed) *v = !*v;

        ImGuiStorage* st = ImGui::GetStateStorage();
        const ImGuiID id = ImGui::GetID("##anim");
        float t = st->GetFloat(id, *v ? 1.0f : 0.0f);
        const float target = *v ? 1.0f : 0.0f;
        t += (target - t) * std::min(1.0f, ImGui::GetIO().DeltaTime * 16.0f);
        if (std::abs(target - t) < 0.001f) t = target;
        st->SetFloat(id, t);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x, p.y + (rowH - ts.y) * 0.5f),
                    ImGui::GetColorU32(ImGuiCol_Text), label);

        const ImVec2 a(p.x + rowW - w, p.y + (rowH - h) * 0.5f);
        const ImVec2 b(a.x + w, a.y + h);
        const ImVec4 track = Mix(Gray(0.82f), g_accent, t);
        dl->AddRectFilled(a, b, Col(track), h * 0.5f);

        const float r = h * 0.5f - 3.0f * g_scale;
        const float cx = a.x + h * 0.5f + (w - h) * t;
        dl->AddCircleFilled(ImVec2(cx + 0.0f, a.y + h * 0.5f + 1.0f), r, A(IM_COL32(0, 0, 0, 28)), 24);
        dl->AddCircleFilled(ImVec2(cx, a.y + h * 0.5f), r, A(IM_COL32(255, 255, 255, 255)), 24);

        ImGui::PopID();
        return pressed;
    }

    // Custom slider: accent-filled progress track plus a round knob
    static bool SliderRow(const char* label, float* v, float lo, float hi,
                          const char* fmt, float step = 0.0f)
    {
        ImGui::PushID(label);

        const float rowW = ImGui::GetContentRegionAvail().x - RightPad();
        const ImVec2 labelPos = ImGui::GetCursorScreenPos();
        ImGui::TextUnformatted(label);

        const float h = ImGui::GetFrameHeight();
        const float r = h * 0.36f;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##slider", ImVec2(rowW, h));
        const bool active  = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();

        bool changed = false;
        const float span = hi - lo;
        if (active && span > 0.0f)
        {
            float t = (ImGui::GetIO().MousePos.x - (p.x + r)) / (rowW - r * 2.0f);
            t = clamp_val(t, 0.0f, 1.0f);
            float nv = lo + t * span;
            if (step > 0.0f) nv = lo + std::round((nv - lo) / step) * step;
            nv = clamp_val(nv, lo, hi);
            if (nv != *v) { *v = nv; changed = true; }
        }
        *v = clamp_val(*v, lo, hi);

        const float t  = span > 0.0f ? (*v - lo) / span : 0.0f;
        const float x0 = p.x + r;
        const float x1 = p.x + rowW - r;
        const float cx = x0 + (x1 - x0) * t;
        const float cy = p.y + h * 0.5f;
        const float th = h * 0.26f;

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 ta(p.x, cy - th * 0.5f);
        const ImVec2 tb(p.x + rowW, cy + th * 0.5f);
        dl->AddRectFilled(ta, tb, Col(Gray(0.85f)), th * 0.5f);
        dl->AddRectFilled(ta, ImVec2(cx, tb.y), Col(g_accent), th * 0.5f);

        if (step > 0.0f && span > 0.0f && span / step <= 20.0f)
        {
            const int n = (int)std::round(span / step);
            for (int i = 0; i <= n; ++i)
            {
                const float f  = (float)i / (float)n;
                const float tx = x0 + (x1 - x0) * f;
                const bool filled = f <= t + 0.001f;
                dl->AddCircleFilled(ImVec2(tx, cy), th * 0.22f,
                    filled ? A(IM_COL32(255, 255, 255, 200)) : Col(Gray(0.68f)), 8);
            }
        }

        const float kr = active ? r * 1.08f : (hovered ? r * 1.04f : r);
        dl->AddCircleFilled(ImVec2(cx, cy + 1.5f * g_scale), kr, A(IM_COL32(0, 0, 0, 34)), 24);
        dl->AddCircleFilled(ImVec2(cx, cy), kr, A(IM_COL32(255, 255, 255, 255)), 24);
        dl->AddCircle(ImVec2(cx, cy), kr, Col(g_accent), 24, 2.5f * g_scale);

        char buf[32];
        std::snprintf(buf, sizeof(buf), fmt, *v);
        const ImVec2 vs = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(labelPos.x + rowW - vs.x, labelPos.y), Col(g_accent), buf);

        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.15f));
        ImGui::PopID();
        return changed;
    }

    static bool ComboRow(const char* label, int* v, const char* const* items, int count)
    {
        ImGui::PushID(label);
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(-RightPad());
        const bool changed = ImGui::Combo("##c", v, items, count);
        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.15f));
        ImGui::PopID();
        return changed;
    }

    static bool ColorRow(const char* label, float* rgba)
    {
        ImGui::PushID(label);
        const float startX = ImGui::GetCursorPosX();
        const float avail  = ImGui::GetContentRegionAvail().x - RightPad();
        ImGui::AlignTextToFramePadding();
        ImGui::TextUnformatted(label);
        ImGui::SameLine();
        ImGui::SetCursorPosX(startX + avail - ImGui::GetFrameHeight());
        const bool changed = ImGui::ColorEdit4("##col", rgba,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
            ImGuiColorEditFlags_AlphaBar);
        ImGui::PopID();
        return changed;
    }

    static bool TabButton(const char* label, bool active, float w)
    {
        const float u = ImGui::GetFontSize();
        const float h = ImGui::GetFrameHeight() * 1.25f;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool pressed = ImGui::InvisibleButton(label, ImVec2(w, h));
        const bool hov = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float rounding = ImGui::GetStyle().FrameRounding;
        const ImVec2 b(p.x + w, p.y + h);

        if (active)
        {
            dl->AddRectFilled(p, b, Col(g_accent, 0.14f), rounding);
            dl->AddRectFilled(ImVec2(p.x, p.y + h * 0.22f),
                              ImVec2(p.x + 3.5f * g_scale, p.y + h * 0.78f),
                              Col(g_accent), 2.0f);
        }
        else if (hov)
        {
            dl->AddRectFilled(p, b, Col(Gray(0.95f)), rounding);
        }

        const ImVec4 tc = active ? g_accent : Gray(0.38f);
        const ImVec2 ts = ImGui::CalcTextSize(label);
        dl->AddText(ImVec2(p.x + u * 0.9f, p.y + (h - ts.y) * 0.5f), Col(tc), label);
        return pressed;
    }

    // Memory cheat row with animated pill switch and status badge
    static void MemoryCheatRow(MemoryCheats::Feature feat, float default_slider = 0.0f)
    {
        const char* label = MemoryCheats::GetFeatureLabel(feat);
        std::string status = MemoryCheats::GetFeatureStatus(feat);
        auto& ctrl = MemoryCheats::GetControl(feat);

        ImGui::PushID(static_cast<int>(feat));

        bool active = ctrl.active;
        if (Toggle(label, &active)) {
            MemoryCheats::RequestFeature(feat, active, default_slider);
        }

        // Status badge right under the row
        ImVec4 badgeCol = Gray(0.55f);
        if (status.find("ACTIVE") != std::string::npos) {
            badgeCol = ImVec4(0.06f, 0.73f, 0.51f, 1.0f); // vibrant green
        } else if (status.find("Scanning") != std::string::npos) {
            badgeCol = ImVec4(0.96f, 0.62f, 0.04f, 1.0f); // amber
        } else if (status.find("failed") != std::string::npos || status.find("not found") != std::string::npos) {
            badgeCol = ImVec4(0.93f, 0.26f, 0.30f, 1.0f); // red
        }

        ImGui::Indent(ImGui::GetFontSize() * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, badgeCol);
        ImGui::Text("Status: %s", status.c_str());
        ImGui::PopStyleColor();
        ImGui::Unindent(ImGui::GetFontSize() * 0.5f);

        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.2f));
        ImGui::PopID();
    }

    // ---------- theme ----------

    void ApplyWhiteTheme(float scale, const ImVec4& accent)
    {
        g_scale  = scale;
        g_accent = accent;

        ImGuiStyle& s = ImGui::GetStyle();
        s = ImGuiStyle();

        s.WindowPadding     = ImVec2(16, 16);
        s.FramePadding      = ImVec2(12, 9);
        s.ItemSpacing       = ImVec2(10, 12);
        s.ItemInnerSpacing  = ImVec2(8, 6);
        s.ScrollbarSize     = 14;
        s.GrabMinSize       = 22;
        s.WindowBorderSize  = 1;
        s.ChildBorderSize   = 0;
        s.PopupBorderSize   = 1;
        s.FrameBorderSize   = 0;
        s.WindowRounding    = 16;
        s.ChildRounding     = 12;
        s.FrameRounding     = 9;
        s.PopupRounding     = 10;
        s.ScrollbarRounding = 10;
        s.GrabRounding      = 9;
        s.WindowTitleAlign  = ImVec2(0.0f, 0.5f);
        s.ScaleAllSizes(scale);

        ImVec4* c = s.Colors;
        c[ImGuiCol_Text]                 = ImVec4(0.10f, 0.11f, 0.14f, 1.00f);
        c[ImGuiCol_TextDisabled]         = ImVec4(0.55f, 0.57f, 0.62f, 1.00f);
        c[ImGuiCol_WindowBg]             = ImVec4(0.965f, 0.970f, 0.980f, 1.00f);
        c[ImGuiCol_ChildBg]              = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_PopupBg]              = Gray(1.0f);
        c[ImGuiCol_Border]               = ImVec4(0.87f, 0.88f, 0.91f, 1.00f);
        c[ImGuiCol_BorderShadow]         = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_FrameBg]              = ImVec4(0.925f, 0.935f, 0.955f, 1.00f);
        c[ImGuiCol_FrameBgHovered]       = ImVec4(0.890f, 0.905f, 0.935f, 1.00f);
        c[ImGuiCol_FrameBgActive]        = ImVec4(0.850f, 0.870f, 0.920f, 1.00f);
        c[ImGuiCol_ScrollbarBg]          = ImVec4(0, 0, 0, 0);
        c[ImGuiCol_ScrollbarGrab]        = Gray(0.80f);
        c[ImGuiCol_ScrollbarGrabHovered] = Gray(0.70f);
        c[ImGuiCol_ScrollbarGrabActive]  = Gray(0.60f);
        c[ImGuiCol_CheckMark]            = accent;
        c[ImGuiCol_SliderGrab]           = accent;
        c[ImGuiCol_SliderGrabActive]     = Mix(accent, Gray(0.0f), 0.18f);
        c[ImGuiCol_Button]               = ImVec4(0.925f, 0.935f, 0.955f, 1.00f);
        c[ImGuiCol_ButtonHovered]        = ImVec4(0.880f, 0.895f, 0.930f, 1.00f);
        c[ImGuiCol_ButtonActive]         = ImVec4(0.830f, 0.850f, 0.910f, 1.00f);
        c[ImGuiCol_Header]               = ImVec4(accent.x, accent.y, accent.z, 0.12f);
        c[ImGuiCol_HeaderHovered]        = ImVec4(accent.x, accent.y, accent.z, 0.18f);
        c[ImGuiCol_HeaderActive]         = ImVec4(accent.x, accent.y, accent.z, 0.26f);
        c[ImGuiCol_Separator]            = ImVec4(0.89f, 0.90f, 0.92f, 1.00f);
        c[ImGuiCol_TextSelectedBg]       = ImVec4(accent.x, accent.y, accent.z, 0.25f);
        c[ImGuiCol_ModalWindowDimBg]     = ImVec4(0, 0, 0, 0.20f);
    }

    // Accent swatches
    static void AccentPresets(Config& cfg)
    {
        static const ImVec4 presets[] = {
            ImVec4(0.20f, 0.45f, 1.00f, 1.0f),   // blue
            ImVec4(0.93f, 0.26f, 0.30f, 1.0f),   // red
            ImVec4(0.55f, 0.36f, 0.96f, 1.0f),   // violet
            ImVec4(0.06f, 0.73f, 0.51f, 1.0f),   // emerald
            ImVec4(0.96f, 0.62f, 0.04f, 1.0f),   // amber
        };
        const float d = ImGui::GetFrameHeight() * 0.9f;
        ImDrawList* dl = ImGui::GetWindowDrawList();
        for (int i = 0; i < 5; ++i)
        {
            if (i > 0) ImGui::SameLine();
            ImGui::PushID(i);
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const bool pressed = ImGui::InvisibleButton("##preset", ImVec2(d, d));
            const ImVec2 c(p.x + d * 0.5f, p.y + d * 0.5f);
            dl->AddCircleFilled(c, d * 0.5f, Col(presets[i]), 24);
            const bool selected = std::fabs(cfg.accent[0] - presets[i].x) < 0.01f &&
                                  std::fabs(cfg.accent[1] - presets[i].y) < 0.01f &&
                                  std::fabs(cfg.accent[2] - presets[i].z) < 0.01f;
            if (selected)
                dl->AddCircle(c, d * 0.5f + 3.0f * g_scale, Col(presets[i]), 24, 2.0f * g_scale);
            if (pressed)
            {
                cfg.accent[0] = presets[i].x;
                cfg.accent[1] = presets[i].y;
                cfg.accent[2] = presets[i].z;
                ApplyWhiteTheme(g_scale, presets[i]);
            }
            ImGui::PopID();
        }
    }

    // ---------- tabs ----------

    static void TabVisuals(Config& cfg)
    {
        static const char* boxStyles[] = { "Full Box (2D)", "Corner Brackets", "3D Perspective Box" };
        static const char* lineOrigins[] = { "Top", "Crosshair", "Bottom", "HUD Bar" };

        Section("PLAYER ESP OVERLAY");
        Toggle("Enable ESP Master", &cfg.espEnabled);
        Toggle("Bounding Box", &cfg.espBox);
        ComboRow("Box Style", &cfg.espBoxStyle, boxStyles, 3);
        Toggle("Skeleton Joints", &cfg.espSkeleton);
        Toggle("Player Name", &cfg.espName);
        Toggle("Health Bar", &cfg.espHealth);
        Toggle("Distance Label", &cfg.espDistance);
        Toggle("Team ID", &cfg.espShowTeam);
        Toggle("Snaplines", &cfg.espLines);
        ComboRow("Snapline Origin", &cfg.espLineOrigin, lineOrigins, 4);
        Toggle("Offscreen Radar Arrows", &cfg.espOffscreenArrows);
        Toggle("Hide Bots", &cfg.espFilterBots);

        Section("ESP RANGE & GEOMETRY");
        SliderRow("Max ESP Distance", &cfg.espMaxDistance, 50.0f, 1500.0f, "%.0f m");

        Section("CUSTOM ESP COLORS");
        ColorRow("Enemy Box Color", cfg.espBoxColor);
        ColorRow("Bot Color", cfg.espBotColor);
        ColorRow("Skeleton Color", cfg.espSkeletonColor);
        ColorRow("Snapline Color", cfg.espLineColor);
        ColorRow("Text Info Color", cfg.espTextColor);

        Section("RADAR & CAMERA (IPAD VIEW)");
        Toggle("Enable Radar Widget", &cfg.radarEnabled);
        SliderRow("Radar Range", &cfg.radarRange, 25.0f, 500.0f, "%.0f m");
        Toggle("iPad View (Wide Camera FOV)", &cfg.ipadView);
        if (cfg.ipadView) {
            SliderRow("Camera FOV", &cfg.ipadFov, 60.0f, 130.0f, "%.0f deg");
        }
    }

    static void TabAim(Config& cfg)
    {
        static const char* bones[] = { "Head", "Chest", "Pelvis" };
        static const char* activations[] = { "Always", "Scoped Only", "Firing Only", "Scoped & Firing" };

        Section("AIM ASSIST & LOCK");
        Toggle("Enable Aim Assist", &cfg.aimEnabled);
        Toggle("Silent Aim", &cfg.silentAim);
        Toggle("Aim 360 (All Directions)", &cfg.aim360);
        Toggle("Instant Snap Aim", &cfg.instantAim);
        SliderRow("Assist Strength", &cfg.aimStrength, 10.0f, 100.0f, "%.0f %%", 10.0f);
        ComboRow("Target Bone", &cfg.aimBone, bones, 3);
        ComboRow("Trigger Activation", &cfg.aimActivation, activations, 4);

        Section("TARGETING CRITERIA");
        ImGui::BeginDisabled(cfg.aim360);
        SliderRow("FOV Radius", &cfg.aimFov, 5.0f, 180.0f, "%.0f deg");
        Toggle("Draw FOV Circle", &cfg.aimDrawFov);
        ImGui::EndDisabled();
        SliderRow("Aim Max Distance", &cfg.aimDistance, 25.0f, 800.0f, "%.0f m");
        SliderRow("Smoothing (1 = instant)", &cfg.aimSmooth, 1.0f, 30.0f, "%.1f");
        Toggle("Visible Targets Only", &cfg.aimVisibleOnly);
        Toggle("Ignore Knocked Down", &cfg.aimIgnoreKnocked);
        Toggle("Target Bots", &cfg.aimTargetBots);
    }

    static void TabMemory(Config& cfg)
    {
        static const char* regModes[] = { "OTHER", "ANONYMOUS", "C_ALLOC", "CODE_APP" };

        Section("DARWIN MACH VM SCANNER");
        if (ComboRow("Memory Region Mode", &cfg.memoryRegionMode, regModes, 4)) {
            MachMemory::SetRegionMode(static_cast<MachMemory::RegionMode>(cfg.memoryRegionMode));
        }

        Section("WEAPON CHEATS");
        MemoryCheatRow(MemoryCheats::Feature::AKM_1HIT);
        MemoryCheatRow(MemoryCheats::Feature::M4A1_1HIT);
        MemoryCheatRow(MemoryCheats::Feature::DESERT_EAGLE);
        MemoryCheatRow(MemoryCheats::Feature::G18C);
        MemoryCheatRow(MemoryCheats::Feature::ONE_HIT_ALL);
        MemoryCheatRow(MemoryCheats::Feature::NO_RECOIL);
        MemoryCheatRow(MemoryCheats::Feature::FAST_SHOT);
        MemoryCheatRow(MemoryCheats::Feature::FAST_RELOAD);

        Section("PLAYER & PHYSICS");
        static float speedVal = 2.5f;
        MemoryCheatRow(MemoryCheats::Feature::SPEED_HACK, speedVal);
        SliderRow("Speed Multiplier", &speedVal, 1.5f, 6.0f, "%.1f x");

        MemoryCheatRow(MemoryCheats::Feature::FLY_HACK);
        MemoryCheatRow(MemoryCheats::Feature::NO_FALL_DAMAGE);
        MemoryCheatRow(MemoryCheats::Feature::UNDER_GROUND);
        MemoryCheatRow(MemoryCheats::Feature::FAST_PARACHUTE);
        MemoryCheatRow(MemoryCheats::Feature::FAST_HEAL);

        Section("ENVIRONMENT & VISUAL CHEATS");
        MemoryCheatRow(MemoryCheats::Feature::WALL_HACK);
        MemoryCheatRow(MemoryCheats::Feature::NO_GRASS);
        MemoryCheatRow(MemoryCheats::Feature::CLEAR_FOG);
        MemoryCheatRow(MemoryCheats::Feature::CAMERA_WIDE);
        MemoryCheatRow(MemoryCheats::Feature::AUTO_SCOPE);
        MemoryCheatRow(MemoryCheats::Feature::HIGH_SCOPE);
    }

    static void TabMisc(Config& cfg)
    {
        Section("INTERFACE & OVERLAYS");
        Toggle("Watermark Pill", &cfg.showWatermark);
        Toggle("FPS Counter", &cfg.showFps);
        Toggle("Screen Crosshair", &cfg.screenCrosshair);
        SliderRow("Menu Opacity", &cfg.menuOpacity, 0.5f, 1.0f, "%.2f");
        SliderRow("Menu Scale", &cfg.menuScale, 0.8f, 1.4f, "%.2f x");

        Section("ANTI-DETECTION & DEVICE ID");
        bool stealthOk = RyomenX::StealthBypass::IsStealthActive();
        ImGui::Text("Stealth Protection: ");
        ImGui::SameLine();
        ImGui::TextColored(stealthOk ? ImVec4(0.06f, 0.73f, 0.51f, 1.0f) : ImVec4(0.93f, 0.26f, 0.30f, 1.0f),
                           stealthOk ? "ACTIVE (Jailbreak & Dylib Masked)" : "INITIALIZING");

        std::string idfv = RyomenX::StealthBypass::GetDeviceIDFV();
        ImGui::Text("Device IDFV: %s", idfv.c_str());

        if (ImGui::Button("Randomize Device ID (Instant Unban)", ImVec2(-RightPad(), 0))) {
            RyomenX::StealthBypass::RandomizeDeviceIDFV();
        }
        ImGui::TextDisabled("Generates clean synthetic vendor UUID to bypass device hardware bans.");
    }

    static bool s_force_recenter = false;

    static void TabSettings(Config& cfg)
    {
        Section("ECLIPSE ACCENT THEME");
        ImGui::TextUnformatted("Preset Accents");
        AccentPresets(cfg);

        ImGui::TextUnformatted("Custom Accent Color");
        ImGui::SetNextItemWidth(-RightPad());
        if (ImGui::ColorEdit3("##accent", cfg.accent,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                ImGuiColorEditFlags_PickerHueWheel))
        {
            ApplyWhiteTheme(g_scale, ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], 1.0f));
        }

        Section("CONFIGURATION PERSISTENCE");
        if (ImGui::Button("Re-center Menu on Screen", ImVec2(-RightPad(), 0)))
        {
            s_force_recenter = true;
        }

        if (ImGui::Button("Reset to Defaults", ImVec2(-RightPad(), 0)))
        {
            cfg = Config();
            ApplyWhiteTheme(g_scale, ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], 1.0f));
            s_force_recenter = true;
        }

        ImGui::Dummy(ImVec2(0, ImGui::GetFontSize() * 0.5f));
        ImGui::TextDisabled("RyomenX VIP - Reverse Engineered & Restructured");
        ImGui::TextDisabled("Dual NeoX BigWorld Engine & Metal Native Pipeline");
    }

    // ---------- main window ----------

    void Render(bool* open)
    {
        if (open && !*open) return;

        Config& cfg = Cfg();
        SyncToRyomenX();

        ImGuiIO& io = ImGui::GetIO();

        // Responsive mobile DPI scaling: base 1.35x for retina handheld screens
        const float activeScale = 1.35f * clamp_val(cfg.menuScale, 0.8f, 1.6f);
        io.FontGlobalScale = activeScale;

        const float u = ImGui::GetFontSize();

        ImGui::GetStyle().Alpha = clamp_val(cfg.menuOpacity, 0.5f, 1.0f);

        // Optimal mobile landscape proportions (fills ~72-88% of screen)
        const float targetW = std::min(io.DisplaySize.x * 0.90f, std::max(560.0f * (activeScale / 1.35f), io.DisplaySize.x * 0.72f));
        const float targetH = std::min(io.DisplaySize.y * 0.92f, std::max(340.0f * (activeScale / 1.35f), io.DisplaySize.y * 0.86f));
        const ImVec2 size(targetW, targetH);

        static bool s_has_centered = false;
        static float s_last_w = 0.0f;
        static float s_last_h = 0.0f;
        static float s_last_scale = 1.0f;

        bool display_changed = (std::abs(io.DisplaySize.x - s_last_w) > 40.0f ||
                                std::abs(io.DisplaySize.y - s_last_h) > 40.0f);
        if (display_changed) {
            s_last_w = io.DisplaySize.x;
            s_last_h = io.DisplaySize.y;
            s_has_centered = false; // Re-center on resolution/orientation change
        }

        bool scale_changed = (std::abs(cfg.menuScale - s_last_scale) > 0.03f);
        if (scale_changed) {
            s_last_scale = cfg.menuScale;
            ApplyWhiteTheme(activeScale, ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], 1.0f));
        }

        // Center on initial valid landscape frame or orientation/scale change or manual recenter
        if ((!s_has_centered || s_force_recenter) &&
            io.DisplaySize.x > io.DisplaySize.y && io.DisplaySize.x > 400.0f) {
            ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - targetW) * 0.5f,
                                           (io.DisplaySize.y - targetH) * 0.5f),
                                    ImGuiCond_Always);
            ImGui::SetNextWindowSize(size, ImGuiCond_Always);
            s_has_centered = true;
            s_force_recenter = false;
        } else if (scale_changed) {
            ImGui::SetNextWindowSize(size, ImGuiCond_Always);
        } else {
            ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
        }

        const ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoSavedSettings;

        static int tab = 0;

        if (ImGui::Begin("##eclipse_menu", nullptr, wf))
        {
            g_window_pos  = ImGui::GetWindowPos();
            g_window_size = ImGui::GetWindowSize();

            // Header (drag here to move the window)
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, g_accent);
            ImGui::TextUnformatted(kTitle);
            ImGui::PopStyleColor();

            // Circular Touch-Friendly 'X' Close Button
            if (open)
            {
                const float btnSize = std::max(30.0f, u * 1.6f);
                const float padX = ImGui::GetStyle().WindowPadding.x;
                const float padY = ImGui::GetStyle().WindowPadding.y;

                ImGui::SameLine();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - btnSize - padX);
                ImGui::SetCursorPosY(padY * 0.5f);

                const ImVec2 p = ImGui::GetCursorScreenPos();
                if (ImGui::InvisibleButton("##close_x_btn", ImVec2(btnSize, btnSize))) {
                    *open = false;
                    RyomenX::Settings::Get().menu_open = false;
                }

                const bool hovered = ImGui::IsItemHovered();
                const bool active  = ImGui::IsItemActive();
                ImDrawList* dl = ImGui::GetWindowDrawList();

                const float radius = btnSize * 0.5f;
                const ImVec2 center(p.x + radius, p.y + radius);

                // Circular background: red on hover/active, soft light gray on idle
                ImU32 bgCol = active ? IM_COL32(235, 60, 60, 255) :
                              (hovered ? IM_COL32(245, 90, 90, 230) : IM_COL32(235, 238, 245, 230));
                dl->AddCircleFilled(center, radius, bgCol, 24);

                // Anti-aliased mathematical 'X' cross lines
                const float crossR = radius * 0.38f;
                const float stroke = std::max(2.0f, 2.2f * g_scale);
                ImU32 xCol = (hovered || active) ? IM_COL32(255, 255, 255, 255) : IM_COL32(85, 90, 105, 255);

                dl->AddLine(ImVec2(center.x - crossR, center.y - crossR),
                            ImVec2(center.x + crossR, center.y + crossR), xCol, stroke);
                dl->AddLine(ImVec2(center.x + crossR, center.y - crossR),
                            ImVec2(center.x - crossR, center.y + crossR), xCol, stroke);
            }
            ImGui::Dummy(ImVec2(0, u * 0.15f));

            // Sidebar
            const float sideW = u * 10.5f;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Gray(1.0f));
            ImGui::BeginChild("##side", ImVec2(sideW, 0), false, ImGuiWindowFlags_NoScrollbar);
            {
                const float pad = u * 0.6f;
                ImGui::SetCursorPos(ImVec2(pad, pad));
                const float tw = sideW - pad * 2.0f;

                static const char* names[] = { "Visuals", "Aim", "Memory", "Misc", "Settings" };
                for (int i = 0; i < 5; ++i)
                {
                    ImGui::SetCursorPosX(pad);
                    if (TabButton(names[i], tab == i, tw)) tab = i;
                }

                ImGui::SetCursorPos(ImVec2(pad + u * 0.3f, ImGui::GetWindowHeight() - u * 2.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled));
                ImGui::TextUnformatted(kVersion);
                ImGui::PopStyleColor();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();

            ImGui::SameLine();

            // Content
            ImGui::BeginChild("##content", ImVec2(0, 0), false, ImGuiWindowFlags_None);
            {
                ImGui::Indent(u * 0.6f);
                switch (tab)
                {
                    case 0: TabVisuals(cfg);  break;
                    case 1: TabAim(cfg);      break;
                    case 2: TabMemory(cfg);   break;
                    case 3: TabMisc(cfg);     break;
                    default: TabSettings(cfg); break;
                }
                ImGui::Dummy(ImVec2(0, u * 1.0f));
                ImGui::Unindent(u * 0.6f);
            }
            ImGui::EndChild();
        }
        ImGui::End();
        ImGui::GetStyle().Alpha = 1.0f;
    }

    void RenderHud(const char* title, bool* open)
    {
        const Config& cfg = Cfg();
        if (!cfg.showWatermark && !cfg.showFps) return;

        int players = PythonBridge::Get().GetPlayerCount();
        int bots = PythonBridge::Get().GetBotCount();

        char buf[128];
        if (cfg.showFps) {
            std::snprintf(buf, sizeof(buf), "%s  |  P: %d  B: %d  |  %.0f fps",
                          title, players, bots, ImGui::GetIO().Framerate);
        } else {
            std::snprintf(buf, sizeof(buf), "%s  |  P: %d  B: %d", title, players, bots);
        }

        const float u = ImGui::GetFontSize();
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const ImVec2 a(u * 0.8f, u * 0.8f);
        const ImVec2 b(a.x + ts.x + u * 1.6f, a.y + ts.y + u * 0.8f);

        g_hud_pos  = a;
        g_hud_size = ImVec2(b.x - a.x, b.y - a.y);

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 240), (b.y - a.y) * 0.5f);
        dl->AddRect(a, b, IM_COL32(0, 0, 0, 24), (b.y - a.y) * 0.5f);
        dl->AddCircleFilled(ImVec2(a.x + u * 0.7f, (a.y + b.y) * 0.5f), u * 0.22f, Col(g_accent), 16);
        dl->AddText(ImVec2(a.x + u * 1.2f, a.y + u * 0.4f), IM_COL32(25, 28, 36, 255), buf);

        // Click / touch HUD pill to toggle menu open
        if (open && ImGui::IsMouseClicked(0)) {
            ImVec2 m = ImGui::GetIO().MousePos;
            if (m.x >= a.x && m.x <= b.x && m.y >= a.y && m.y <= b.y) {
                *open = !*open;
            }
        }
    }

    bool IsTouchInside(float x, float y, bool menuOpen)
    {
        // Check HUD pill bounding box
        if (x >= g_hud_pos.x && x <= (g_hud_pos.x + g_hud_size.x) &&
            y >= g_hud_pos.y && y <= (g_hud_pos.y + g_hud_size.y))
        {
            return true;
        }

        // Check Eclipse main menu window bounding box
        if (menuOpen)
        {
            if (x >= g_window_pos.x && x <= (g_window_pos.x + g_window_size.x) &&
                y >= g_window_pos.y && y <= (g_window_pos.y + g_window_size.y))
            {
                return true;
            }
        }

        return false;
    }
}
