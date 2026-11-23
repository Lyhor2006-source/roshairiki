ECLIPSE white-theme ImGui menu
==============================

Files
  src/Menu.h                    config struct + public API
  src/Menu.cpp                  theme, widgets, tabs, HUD pill
  examples/MetalIntegration.mm  wiring snippet for imgui_impl_metal
  CMakeLists.txt                static lib build (needs -DIMGUI_DIR)

Requirements
  C++17, Dear ImGui 1.89 or newer. Not compiled against your project.

Usage
  1. Add src/Menu.cpp to your target (or build the CMake lib).
  2. Load a real TTF at your scale, then call Menu::ApplyWhiteTheme().
  3. Each frame: Menu::Render(&open); Menu::RenderHud("ECLIPSE");
  4. Read feature settings from Menu::Cfg().

Notes
  - Nothing here reads game data. Feature code consumes Menu::Cfg().
  - aimStrength is a percent (10-100, steps of 10). Consumers use
    aimStrength / 100 as the correction multiplier.
  - Sliders are custom-drawn: accent-filled progress track + knob.
    Pick the red swatch in Settings > Accent presets for a red fill.
  - aimActivation (Always / Scope only / Scope and firing / Firing only)
    is only stored here. Scope and firing state come from your game reader.
  - aim360 ignores aimFov. The FOV controls are dimmed while it is on.
  - silentAim is a flag only. What it does is up to the consumer.
  - aimSmooth is a unitless divisor (1 = instant). The consumer decides
    the time basis.
  - Finger-pan scrolling is not included. Convert touch pan deltas to
    io.AddMouseWheelEvent over the content area if you want it.
