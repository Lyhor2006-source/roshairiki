#include "IosUI.hpp"
#include "Config.hpp"
#include "Entity.hpp"
#include "PythonBridge.hpp"
#include "StealthBypass.hpp"
#include <imgui.h>
#include <cmath>
#include <string>

namespace IosUI {

using namespace RyomenX;

static ImVec2 s_win_touch_start_pos(150.0f, 100.0f);

bool IsTouchInsideUI(float touch_x, float touch_y) {
    auto& s = Settings::Get();
    ImVec2 pt(touch_x, touch_y);

    // Floating bar bounding box
    ImVec2 bar_min(s.bar_pos_x, s.bar_pos_y);
    ImVec2 bar_max(s.bar_pos_x + 180.0f, s.bar_pos_y + 40.0f);
    if (pt.x >= bar_min.x && pt.x <= bar_max.x && pt.y >= bar_min.y && pt.y <= bar_max.y) {
        return true;
    }

    // Main window bounding box if open
    if (s.menu_open) {
        ImVec2 win_pos = s_win_touch_start_pos;
        ImVec2 win_size(520.0f * s.menu_scale, 400.0f * s.menu_scale);
        if (pt.x >= win_pos.x && pt.x <= win_pos.x + win_size.x &&
            pt.y >= win_pos.y && pt.y <= win_pos.y + win_size.y) {
            return true;
        }
    }
    return false;
}

void RenderFloatingBar() {
    auto& s = Settings::Get();
    if (!s.show_floating_bar) return;

    ImGui::SetNextWindowPos(ImVec2(s.bar_pos_x, s.bar_pos_y), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(180.0f, 40.0f), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoFocusOnAppearing |
                             ImGuiWindowFlags_NoNav;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 25, 230));
    ImGui::PushStyleColor(ImGuiCol_Border, IM_COL32(255, 69, 0, 220));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);

    if (ImGui::Begin("##FloatingBarWindow", nullptr, flags)) {
        s.bar_pos_x = ImGui::GetWindowPos().x;
        s.bar_pos_y = ImGui::GetWindowPos().y;

        int players = PythonBridge::Get().GetPlayerCount();
        int bots = PythonBridge::Get().GetBotCount();

        char title[64];
        snprintf(title, sizeof(title), "RYOMENX [%d|%d]", players, bots);

        if (ImGui::Button(title, ImVec2(-1, -1))) {
            s.menu_open = !s.menu_open;
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void RenderFeatureRow(MemoryCheats::Feature feat, float default_slider) {
    const char* label = MemoryCheats::GetFeatureLabel(feat);
    std::string status = MemoryCheats::GetFeatureStatus(feat);
    auto& ctrl = MemoryCheats::GetControl(feat);

    ImGui::PushID(static_cast<int>(feat));

    bool active = ctrl.active;
    if (ImGui::Checkbox(label, &active)) {
        MemoryCheats::RequestFeature(feat, active, default_slider);
    }

    ImGui::SameLine(240.0f);

    // Dynamic color badge for status
    ImVec4 badge_col = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
    if (status.find("ACTIVE") != std::string::npos) {
        badge_col = ImVec4(0.2f, 0.9f, 0.3f, 1.0f);
    } else if (status.find("Scanning") != std::string::npos) {
        badge_col = ImVec4(0.9f, 0.8f, 0.2f, 1.0f);
    } else if (status.find("failed") != std::string::npos || status.find("not found") != std::string::npos) {
        badge_col = ImVec4(0.9f, 0.3f, 0.2f, 1.0f);
    }

    ImGui::TextColored(badge_col, "[%s]", status.c_str());

    ImGui::PopID();
}

void RenderMainMenu() {
    auto& s = Settings::Get();
    if (!s.menu_open) return;

    ImGui::SetNextWindowPos(s_win_touch_start_pos, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(540.0f * s.menu_scale, 420.0f * s.menu_scale), ImGuiCond_Always);

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(18, 18, 22, 245));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, IM_COL32(30, 30, 38, 255));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, IM_COL32(220, 50, 30, 255));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);

    if (ImGui::Begin("RYOMENX VIP [iOS]##RyomenXMainWindow", &s.menu_open, flags)) {
        s_win_touch_start_pos = ImGui::GetWindowPos();

        // Top Navigation Tabs
        ImGui::BeginGroup();
        const char* tab_names[] = {
            "\uf06e Visuals",
            "\uf1de Adv",
            "\uf05b Aim",
            "\uf044 Mem",
            "\uf013 Settings"
        };

        for (int i = 0; i < 5; ++i) {
            if (i > 0) ImGui::SameLine();
            if (s.current_tab == i) {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(220, 50, 30, 255));
            } else {
                ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(40, 40, 48, 200));
            }

            if (ImGui::Button(tab_names[i], ImVec2(96.0f * s.menu_scale, 32.0f * s.menu_scale))) {
                s.current_tab = i;
            }
            ImGui::PopStyleColor();
        }
        ImGui::EndGroup();

        ImGui::Separator();
        ImGui::Spacing();

        ImGui::BeginChild("##TabContentArea", ImVec2(0, 0), true);

        // TAB 0: VISUALS
        if (s.current_tab == 0) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "ESP OVERLAY CONFIGURATION");
            ImGui::Checkbox("Master ESP", &s.master_esp);
            ImGui::SameLine(240.0f);
            ImGui::Checkbox("Hide Bots", &s.filter_bots);

            ImGui::Separator();
            ImGui::Columns(2, "##VisualToggles", false);
            ImGui::Checkbox("Show Box", &s.show_box);
            ImGui::Checkbox("Skeleton", &s.show_skeleton);
            ImGui::Checkbox("Health Bar", &s.show_health);
            ImGui::Checkbox("Player Name", &s.show_name);
            ImGui::Checkbox("Team ID", &s.show_team);

            ImGui::NextColumn();
            ImGui::Checkbox("Player ID", &s.show_player_id);
            ImGui::Checkbox("ESP Lines", &s.show_lines);
            ImGui::Checkbox("Distance", &s.show_distance);
            ImGui::Checkbox("Offscreen Arrows", &s.show_arrows);
            ImGui::Checkbox("HUD Count", &s.show_hud_count);
            ImGui::Columns(1);

            ImGui::Separator();
            const char* origins[] = {"Top", "Crosshair", "Bottom", "Floating Bar"};
            int cur_orig = static_cast<int>(s.line_origin);
            if (ImGui::Combo("Line Origin##LineOrigin", &cur_orig, origins, 4)) {
                s.line_origin = static_cast<LineOrigin>(cur_orig);
            }

            ImGui::SliderFloat("ESP Distance##ESPRange", &s.esp_range, 50.0f, 1500.0f, "%.0fm");
        }
        // TAB 1: ADVANCED & GEOMETRY
        else if (s.current_tab == 1) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "ESP GEOMETRY & STYLE");
            const char* box_styles[] = {"2D Box (Standard)", "Corner Box (Brackets)", "3D Box (Perspective)"};
            int cur_box = static_cast<int>(s.box_style);
            if (ImGui::Combo("Box Style##BoxStyleCombo", &cur_box, box_styles, 3)) {
                s.box_style = static_cast<BoxStyle>(cur_box);
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "CUSTOM ESP COLORS");
            ImGui::ColorEdit4("Enemy Color##EnemyCol", &s.enemy_color.r);
            ImGui::ColorEdit4("Bot Color##BotCol", &s.bot_color.r);
            ImGui::ColorEdit4("Skeleton Color##SkelCol", &s.skeleton_color.r);
            ImGui::ColorEdit4("Snapline Color##LineCol", &s.line_color.r);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "IPAD VIEW (WIDE CAMERA FOV)");
            ImGui::Checkbox("Enable iPad View", &s.ipad_view);
            if (s.ipad_view) {
                ImGui::SliderFloat("Camera FOV##IpadFovSlider", &s.ipad_fov, 60.0f, 130.0f, "%.0f deg");
            }
        }
        // TAB 2: AIMBOT
        else if (s.current_tab == 2) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "AIMBOT MASTER");
            ImGui::Checkbox("Master Aimbot", &s.master_aim);

            const char* triggers[] = {"Always", "Scoped", "Firing", "Scoped & Firing"};
            int cur_trig = static_cast<int>(s.aim_trigger);
            if (ImGui::Combo("Trigger Mode##Trigger", &cur_trig, triggers, 4)) {
                s.aim_trigger = static_cast<AimTrigger>(cur_trig);
            }

            ImGui::Separator();
            ImGui::Columns(2, "##AimTogglesCol", false);
            ImGui::Checkbox("Silent Aim", &s.silent_aim);
            ImGui::Checkbox("Silent 360", &s.silent_360);
            ImGui::Checkbox("Instant Aim", &s.instant_aim);
            ImGui::Checkbox("Visible Check", &s.visible_check);

            ImGui::NextColumn();
            ImGui::Checkbox("Ignore Knocked", &s.ignore_knocked);
            ImGui::Checkbox("Target Bots", &s.target_bots);
            ImGui::Checkbox("Show FOV Circle", &s.show_fov_circle);

            const char* bones[] = {"Head", "Chest", "Pelvis"};
            int cur_bone = static_cast<int>(s.target_bone);
            if (ImGui::Combo("Target Bone##TargetBone", &cur_bone, bones, 3)) {
                s.target_bone = static_cast<TargetBone>(cur_bone);
            }
            ImGui::Columns(1);

            ImGui::Separator();
            ImGui::SliderFloat("FOV Radius##FOVRadius", &s.fov_radius, 30.0f, 500.0f, "%.0f px");
            ImGui::SliderFloat("Aim Distance##AimDist", &s.aim_distance, 50.0f, 800.0f, "%.0fm");
            ImGui::SliderFloat("Aim Power##Power", &s.aim_power, 0.5f, 5.0f, "%.1fx");
            ImGui::SliderFloat("Aim Smoothness##Smoothness", &s.aim_smoothness, 1.0f, 10.0f, "%.1f");
        }
        // TAB 3: MEMORY CHEATS
        else if (s.current_tab == 3) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "DARWIN MACH VM MEMORY CHEATS");

            const char* reg_modes[] = {"OTHER", "ANONYMOUS", "C_ALLOC", "CODE_APP"};
            if (ImGui::Combo("Memory Region##MemoryRegion", &s.selected_region_mode, reg_modes, 4)) {
                MachMemory::SetRegionMode(static_cast<MachMemory::RegionMode>(s.selected_region_mode));
            }

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "-- WEAPON MODS --");
            RenderFeatureRow(MemoryCheats::Feature::AKM_1HIT);
            RenderFeatureRow(MemoryCheats::Feature::M4A1_1HIT);
            RenderFeatureRow(MemoryCheats::Feature::DESERT_EAGLE);
            RenderFeatureRow(MemoryCheats::Feature::G18C);
            RenderFeatureRow(MemoryCheats::Feature::ONE_HIT_ALL);
            RenderFeatureRow(MemoryCheats::Feature::NO_RECOIL);
            RenderFeatureRow(MemoryCheats::Feature::FAST_SHOT);
            RenderFeatureRow(MemoryCheats::Feature::FAST_RELOAD);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "-- PLAYER & MOVEMENT --");
            RenderFeatureRow(MemoryCheats::Feature::SPEED_HACK, 2.5f);
            RenderFeatureRow(MemoryCheats::Feature::FLY_HACK);
            RenderFeatureRow(MemoryCheats::Feature::NO_FALL_DAMAGE);
            RenderFeatureRow(MemoryCheats::Feature::UNDER_GROUND);
            RenderFeatureRow(MemoryCheats::Feature::FAST_PARACHUTE);
            RenderFeatureRow(MemoryCheats::Feature::FAST_HEAL);

            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "-- ENVIRONMENT & VISUAL --");
            RenderFeatureRow(MemoryCheats::Feature::WALL_HACK);
            RenderFeatureRow(MemoryCheats::Feature::NO_GRASS);
            RenderFeatureRow(MemoryCheats::Feature::CLEAR_FOG);
            RenderFeatureRow(MemoryCheats::Feature::CAMERA_WIDE);
            RenderFeatureRow(MemoryCheats::Feature::AUTO_SCOPE);
            RenderFeatureRow(MemoryCheats::Feature::HIGH_SCOPE);
        }
        // TAB 4: SETTINGS & ANTI-DETECTION
        else if (s.current_tab == 4) {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "ANTI-DETECTION & DEVICE ID");

            bool stealth_ok = StealthBypass::IsStealthActive();
            ImGui::Text("Stealth Protection: ");
            ImGui::SameLine();
            ImGui::TextColored(stealth_ok ? ImVec4(0.2f, 0.9f, 0.3f, 1.0f) : ImVec4(0.9f, 0.2f, 0.2f, 1.0f),
                               stealth_ok ? "ACTIVE (JB, Dylib & Deb Masked)" : "INITIALIZING");

            std::string idfv = StealthBypass::GetDeviceIDFV();
            ImGui::Text("Device ID: %s", idfv.c_str());

            if (ImGui::Button("Randomize Device ID (Unban)")) {
                StealthBypass::RandomizeDeviceIDFV();
            }
            ImGui::TextDisabled("Spoofs UIDevice vendor ID. Tap Randomize if account/device banned.");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "OVERLAY SETTINGS");
            ImGui::Checkbox("Screen Crosshair", &s.screen_crosshair);
            ImGui::Checkbox("Show Watermark", &s.show_watermark);
            ImGui::SliderFloat("Menu Scale", &s.menu_scale, 0.75f, 1.5f, "%.2fx");

            ImGui::Separator();
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.2f, 1.0f), "VIP LICENSE STATUS");
            ImGui::Text("Plan: Lifetime VIP (Community Cracked)");
            ImGui::Text("Architecture: Native arm64 (Metal + NeoX Python Bridge)");

            if (ImGui::Button("Reset All Settings")) {
                s.LoadDefaults();
            }
        }

        ImGui::EndChild();
    }
    ImGui::End();

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(3);
}

void RenderEspOverlay(ImDrawList* draw_list, ImVec2 display_size) {
    auto& s = Settings::Get();
    if (!draw_list || !s.master_esp) return;

    // Crosshair
    if (s.screen_crosshair) {
        ImVec2 center(display_size.x * 0.5f, display_size.y * 0.5f);
        draw_list->AddLine(ImVec2(center.x - 8.0f, center.y), ImVec2(center.x + 8.0f, center.y), IM_COL32(255, 255, 255, 200), 1.5f);
        draw_list->AddLine(ImVec2(center.x, center.y - 8.0f), ImVec2(center.x, center.y + 8.0f), IM_COL32(255, 255, 255, 200), 1.5f);
    }

    // Watermark
    if (s.show_watermark) {
        draw_list->AddText(ImVec2(20.0f, 20.0f), IM_COL32(255, 69, 0, 220), "RYOMENX VIP [iOS Native]");
    }

    // FOV Circle
    if (s.master_aim && s.show_fov_circle) {
        ImVec2 center(display_size.x * 0.5f, display_size.y * 0.5f);
        draw_list->AddCircle(center, s.fov_radius, IM_COL32(255, 255, 255, 120), 48, 1.0f);
    }

    auto entities = PythonBridge::Get().GetEntities();
    ImVec2 screen_center(display_size.x * 0.5f, display_size.y * 0.5f);

    // Determine snapline origin
    ImVec2 line_orig(display_size.x * 0.5f, display_size.y);
    switch (s.line_origin) {
        case LineOrigin::Top: line_orig = ImVec2(display_size.x * 0.5f, 0.0f); break;
        case LineOrigin::Crosshair: line_orig = screen_center; break;
        case LineOrigin::Bottom: line_orig = ImVec2(display_size.x * 0.5f, display_size.y); break;
        case LineOrigin::FloatingBar: line_orig = ImVec2(s.bar_pos_x + 90.0f, s.bar_pos_y + 20.0f); break;
    }

    for (const auto& ent : entities) {
        if (s.filter_bots && ent.is_bot) continue;
        if (ent.distance > s.esp_range) continue;

        uint32_t box_color = ent.is_bot ? s.bot_color.ToU32() : (ent.is_enemy == 0 ? IM_COL32(70, 230, 120, 255) : s.enemy_color.ToU32());
        uint32_t skel_color = s.skeleton_color.ToU32();
        uint32_t snap_color = s.line_color.ToU32();

        // 1. Offscreen radar arrow
        if (!ent.is_on_screen) {
            if (s.show_arrows) {
                float dx = ent.world_pos.x - PythonBridge::Get().GetLocalPosition().x;
                float dz = ent.world_pos.z - PythonBridge::Get().GetLocalPosition().z;
                float angle = std::atan2(dz, dx);

                float radius = std::min(display_size.x, display_size.y) * 0.38f;
                ImVec2 arrow_pos(screen_center.x + std::cos(angle) * radius,
                                 screen_center.y + std::sin(angle) * radius);

                draw_list->AddCircleFilled(arrow_pos, 5.0f, box_color);
            }
            continue;
        }

        auto box = ent.GetBox();
        if (!box.IsValid() || box.Height() > display_size.y * 4.0f) continue;
        if (box.max_x < 0.0f || box.min_x > display_size.x ||
            box.max_y < 0.0f || box.min_y > display_size.y) {
            continue;
        }

        // 2. Bounding Box
        if (s.show_box) {
            switch (s.box_style) {
                case BoxStyle::Box2D:
                    OverlayMath::Draw2DBox(draw_list, box, box_color, 1.5f);
                    break;
                case BoxStyle::Corner:
                    OverlayMath::DrawCornerBox(draw_list, box, box_color, 1.8f);
                    break;
                case BoxStyle::Box3D:
                    OverlayMath::Draw3DBox(draw_list, box, box_color, 1.5f);
                    break;
            }
        }

        // 3. Skeleton Joints & Links
        if (s.show_skeleton) {
            for (const auto& link : SKELETON_LINKS) {
                const auto& b1 = ent.bones[link.from];
                const auto& b2 = ent.bones[link.to];
                if (b1.valid && b2.valid) {
                    draw_list->AddLine(ImVec2(b1.x, b1.y), ImVec2(b2.x, b2.y), skel_color, 1.5f);
                }
            }
            // Draw head circle if valid
            if (ent.bones[BONE_HEAD].valid) {
                draw_list->AddCircle(ImVec2(ent.bones[BONE_HEAD].x, ent.bones[BONE_HEAD].y),
                                    box.Width() * 0.18f, skel_color, 12, 1.2f);
            }
        }

        // 4. Health Bar (left side of box)
        if (s.show_health) {
            float hp_pct = static_cast<float>(ent.hp) / static_cast<float>(ent.max_hp > 0 ? ent.max_hp : 100);
            if (hp_pct < 0.0f) hp_pct = 0.0f;
            if (hp_pct > 1.0f) hp_pct = 1.0f;

            float bar_x = box.min_x - 6.0f;
            float bar_h = box.Height();
            float filled_h = bar_h * hp_pct;

            // Health bar background
            draw_list->AddRectFilled(ImVec2(bar_x - 3.0f, box.min_y),
                                     ImVec2(bar_x, box.max_y),
                                     IM_COL32(0, 0, 0, 180));

            // Color gradient (green -> yellow -> red)
            uint32_t hp_color = IM_COL32(
                static_cast<int>((1.0f - hp_pct) * 255.0f),
                static_cast<int>(hp_pct * 255.0f),
                0, 240
            );

            // Health bar fill
            draw_list->AddRectFilled(ImVec2(bar_x - 3.0f, box.max_y - filled_h),
                                     ImVec2(bar_x, box.max_y),
                                     hp_color);
        }

        // 5. Snapline
        if (s.show_lines) {
            draw_list->AddLine(line_orig, ImVec2(box.min_x + box.Width() * 0.5f, box.max_y), snap_color, 1.2f);
        }

        // 6. Text info (Name, Distance, Knocked status)
        if (s.show_name || s.show_team) {
            char info_top[128];
            if (ent.is_knocked) {
                snprintf(info_top, sizeof(info_top), "[DOWN] %s", ent.name.c_str());
            } else if (ent.is_bot) {
                snprintf(info_top, sizeof(info_top), "BOT %s", ent.name.c_str());
            } else if (s.show_team && s.show_name) {
                snprintf(info_top, sizeof(info_top), "[%d] %s", ent.team_id, ent.name.c_str());
            } else if (s.show_team) {
                snprintf(info_top, sizeof(info_top), "[T%d]", ent.team_id);
            } else {
                snprintf(info_top, sizeof(info_top), "%s", ent.name.c_str());
            }

            ImVec2 text_sz = ImGui::CalcTextSize(info_top);
            ImVec2 text_pos(box.min_x + (box.Width() - text_sz.x) * 0.5f, box.min_y - text_sz.y - 2.0f);
            draw_list->AddText(text_pos, IM_COL32(255, 255, 255, 240), info_top);
        }

        if (s.show_distance) {
            char dist_str[32];
            snprintf(dist_str, sizeof(dist_str), "%.0fm", ent.distance);
            ImVec2 dist_sz = ImGui::CalcTextSize(dist_str);
            ImVec2 dist_pos(box.min_x + (box.Width() - dist_sz.x) * 0.5f, box.max_y + 2.0f);
            draw_list->AddText(dist_pos, IM_COL32(255, 215, 0, 240), dist_str);
        }
    }
}

} // namespace IosUI
