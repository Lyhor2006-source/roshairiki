#pragma once
#include "imgui.h"

namespace Menu
{
    // When aim assist is allowed to act. The menu only stores the choice.
    // Scope and firing state must come from your game reader.
    enum AimActivation
    {
        AimAlways = 0,        // no condition
        AimScopeOnly,         // only while scoped
        AimScopeAndFiring,    // only while scoped AND firing
        AimFiringOnly         // only while firing (scoped or not)
    };

    struct Config
    {
        // Visuals
        bool  espEnabled     = false;
        bool  espBox         = true;
        bool  espName        = true;
        bool  espHealth      = true;
        bool  espDistance    = false;
        int   espBoxStyle    = 0;       // 0 = full, 1 = corners
        float espMaxDistance = 300.0f;  // meters
        float espBoxColor[4]  = { 0.20f, 0.45f, 1.00f, 1.00f };
        float espTextColor[4] = { 0.10f, 0.11f, 0.14f, 1.00f };
        bool  radarEnabled   = false;
        float radarRange     = 150.0f;  // meters

        // Aim
        bool  aimEnabled     = false;
        bool  silentAim      = false;
        bool  aim360         = false;   // consider targets in all directions; aimFov is ignored
        float aimStrength    = 50.0f;   // percent, 10-100 in steps of 10
        int   aimBone        = 0;       // 0 head, 1 neck, 2 chest, 3 pelvis
        int   aimActivation  = AimAlways;  // AimActivation
        float aimFov         = 30.0f;   // degrees, ignored while aim360 is on
        float aimSmooth      = 8.0f;    // 1 = instant
        float aimDistance    = 300.0f;  // meters, targets beyond this are skipped
        bool  aimDrawFov     = true;
        bool  aimVisibleOnly = true;

        // Misc
        bool  recoilAdjust   = false;
        float recoilStrength = 50.0f;   // percent
        bool  showWatermark  = true;
        bool  showFps        = true;
        float menuOpacity    = 1.0f;

        // Settings
        float accent[3]      = { 0.20f, 0.45f, 1.00f };
    };

    Config& Cfg();

    // Call once after ImGui::CreateContext() and font loading.
    // scale = UI scale on top of the base sizes (1.0f default).
    void ApplyWhiteTheme(float scale, const ImVec4& accent);

    // Call every frame between ImGui::NewFrame() and ImGui::Render().
    // open: pass a bool* to let the menu's Hide button close it, or nullptr.
    void Render(bool* open);

    // Optional watermark / FPS pill. Call every frame as well.
    void RenderHud(const char* title);
}
