// Integration snippet, not a standalone file. Paste into your existing
// imgui_impl_metal setup. Adapt names (fontPath, pt, isDown) to your project.
#include "imgui.h"
#include "Menu.h"

// once, after ImGui::CreateContext() and before the first frame
void InitMenu(const char* fontPath)
{
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;
    float scale = 1.0f;                                     // UI scale on top of the base sizes
    io.Fonts->AddFontFromFileTTF(fontPath, 17.0f * scale);  // your own TTF
    Menu::ApplyWhiteTheme(scale, ImVec4(0.20f, 0.45f, 1.0f, 1.0f));
}

// from your touch handler (coordinates in points)
void OnTouch(float x, float y, bool isDown)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent(x, y);
    io.AddMouseButtonEvent(0, isDown);
}

// each frame, after ImGui_ImplMetal_NewFrame(...) and ImGui::NewFrame()
void DrawMenu()
{
    static bool menuOpen = true;
    Menu::Render(&menuOpen);
    Menu::RenderHud("ECLIPSE");
}
// then: ImGui::Render(); and submit draw data as usual.
