#pragma once
#include <imgui.h>

namespace Menu
{
    enum AimActivation
    {
        AimAlways = 0,        // no condition
        AimScopeOnly,         // only while scoped
        AimScopeAndFiring,    // only while scoped AND firing
        AimFiringOnly         // only while firing (scoped or not)
    };

    struct Config
    {
        // Visuals (ESP)
        bool  espEnabled        = true;
        bool  espBox            = true;
        bool  espSkeleton       = true;
        bool  espName           = true;
        bool  espHealth         = true;
        bool  espDistance       = true;
        bool  espLines          = true;
        int   espLineOrigin     = 2;          // 0 = Top, 1 = Crosshair, 2 = Bottom, 3 = HUD
        bool  espShowTeam       = true;
        bool  espOffscreenArrows = true;
        bool  espFilterBots     = false;
        int   espBoxStyle       = 1;          // 0 = full, 1 = corners, 2 = 3d
        float espMaxDistance    = 500.0f;     // meters
        float espBoxColor[4]    = { 0.20f, 0.45f, 1.00f, 1.00f };
        float espTextColor[4]   = { 0.10f, 0.11f, 0.14f, 1.00f };
        float espBotColor[4]    = { 0.20f, 0.70f, 1.00f, 1.00f };
        float espSkeletonColor[4] = { 1.00f, 1.00f, 1.00f, 1.00f };
        float espLineColor[4]   = { 1.00f, 0.90f, 0.00f, 0.80f };
        bool  radarEnabled      = false;
        float radarRange        = 150.0f;     // meters

        // Camera / iPad View
        bool  ipadView          = false;
        float ipadFov           = 95.0f;

        // Aim
        bool  aimEnabled        = false;
        bool  silentAim         = false;
        bool  aim360            = false;      // consider targets in all directions; aimFov is ignored
        bool  instantAim        = false;
        float aimStrength       = 50.0f;      // percent, 10-100 in steps of 10
        int   aimBone           = 0;          // 0 head, 1 neck, 2 chest, 3 pelvis
        int   aimActivation     = AimAlways;  // AimActivation
        float aimFov            = 30.0f;      // degrees, ignored while aim360 is on
        float aimSmooth         = 8.0f;       // 1 = instant
        float aimDistance       = 300.0f;     // meters, targets beyond this are skipped
        bool  aimDrawFov        = true;
        bool  aimVisibleOnly    = true;
        bool  aimIgnoreKnocked  = true;
        bool  aimTargetBots     = true;

        // Memory Cheats Region
        int   memoryRegionMode  = 0;          // 0: OTHER, 1: ANONYMOUS, 2: C_ALLOC, 3: CODE_APP

        // Misc
        bool  recoilAdjust      = false;
        float recoilStrength    = 50.0f;      // percent
        bool  showWatermark     = true;
        bool  showFps           = true;
        bool  screenCrosshair   = false;
        float menuOpacity       = 1.0f;
        float menuScale         = 1.0f;

        // Settings
        float accent[3]         = { 0.20f, 0.45f, 1.00f };
    };

    Config& Cfg();

    // Call once after ImGui::CreateContext() and font loading.
    // scale = UI scale on top of the base sizes (1.0f default).
    void ApplyWhiteTheme(float scale, const ImVec4& accent);

    // Synchronize Menu::Cfg() into RyomenX::Settings even while menu is closed
    void SyncSettings();

    // Call every frame between ImGui::NewFrame() and ImGui::Render().
    // open: pass a bool* to let the menu's Hide button close it, or nullptr.
    void Render(bool* open);

    // Floating watermark / FPS / entity counter pill. Call every frame as well.
    // Clicking the pill toggles the menu open/closed.
    void RenderHud(const char* title, bool* open);

    // Touch passthrough detection for iOS MTKView
    bool IsTouchInside(float x, float y, bool menuOpen);
}
