#include "Menu.h"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace Menu
{
    static const char* kTitle = "ECLIPSE";
    static const char* kVersion = "v1.0";

    static float  g_scale  = 1.0f;
    static ImVec4 g_accent = ImVec4(0.20f, 0.45f, 1.00f, 1.00f);

    Config& Cfg()
    {
        static Config cfg;
        return cfg;
    }

    // ---------- helpers ----------

    // Both helpers follow the current style alpha, so custom-drawn widgets
    // fade with the menu opacity and dim inside BeginDisabled().
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

    // Animated pill switch, label left, switch right-aligned.
    static bool Toggle(const char* label, bool* v)
    {
        ImGui::PushID(label);

        const float rowW = ImGui::GetContentRegionAvail().x - RightPad();
        const float rowH = ImGui::GetFrameHeight() * 1.1f;
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

    // Custom slider: accent-filled progress track plus a round knob.
    // step > 0 snaps the value to lo + n * step and draws one tick per step.
    static bool SliderRow(const char* label, float* v, float lo, float hi,
                          const char* fmt, float step = 0.0f)
    {
        ImGui::PushID(label);

        const float rowW = ImGui::GetContentRegionAvail().x - RightPad();
        const ImVec2 labelPos = ImGui::GetCursorScreenPos();
        ImGui::TextUnformatted(label);

        const float h = ImGui::GetFrameHeight();
        const float r = h * 0.36f;                       // knob radius
        const ImVec2 p = ImGui::GetCursorScreenPos();
        ImGui::InvisibleButton("##slider", ImVec2(rowW, h));
        const bool active  = ImGui::IsItemActive();
        const bool hovered = ImGui::IsItemHovered();

        bool changed = false;
        const float span = hi - lo;
        if (active && span > 0.0f)
        {
            float t = (ImGui::GetIO().MousePos.x - (p.x + r)) / (rowW - r * 2.0f);
            t = std::clamp(t, 0.0f, 1.0f);
            float nv = lo + t * span;
            if (step > 0.0f) nv = lo + std::round((nv - lo) / step) * step;
            nv = std::clamp(nv, lo, hi);
            if (nv != *v) { *v = nv; changed = true; }
        }
        *v = std::clamp(*v, lo, hi);

        const float t  = span > 0.0f ? (*v - lo) / span : 0.0f;
        const float x0 = p.x + r;
        const float x1 = p.x + rowW - r;
        const float cx = x0 + (x1 - x0) * t;
        const float cy = p.y + h * 0.5f;
        const float th = h * 0.26f;                      // track thickness

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const ImVec2 ta(p.x, cy - th * 0.5f);
        const ImVec2 tb(p.x + rowW, cy + th * 0.5f);
        dl->AddRectFilled(ta, tb, Col(Gray(0.85f)), th * 0.5f);              // empty track
        dl->AddRectFilled(ta, ImVec2(cx, tb.y), Col(g_accent), th * 0.5f);   // progress fill

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
        const float h = ImGui::GetFrameHeight() * 1.2f;
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const bool pressed = ImGui::InvisibleButton(label, ImVec2(w, h));
        const bool hov = ImGui::IsItemHovered();

        ImDrawList* dl = ImGui::GetWindowDrawList();
        const float rounding = ImGui::GetStyle().FrameRounding;
        const ImVec2 b(p.x + w, p.y + h);

        if (active)
        {
            dl->AddRectFilled(p, b, Col(g_accent, 0.12f), rounding);
            dl->AddRectFilled(ImVec2(p.x, p.y + h * 0.25f),
                              ImVec2(p.x + 3.0f * g_scale, p.y + h * 0.75f),
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

    // ---------- theme ----------

    void ApplyWhiteTheme(float scale, const ImVec4& accent)
    {
        g_scale  = scale;
        g_accent = accent;

        ImGuiStyle& s = ImGui::GetStyle();
        s = ImGuiStyle();

        s.WindowPadding     = ImVec2(14, 14);
        s.FramePadding      = ImVec2(12, 9);
        s.ItemSpacing       = ImVec2(10, 12);
        s.ItemInnerSpacing  = ImVec2(8, 6);
        s.ScrollbarSize     = 14;       // wide enough for a thumb
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
        c[ImGuiCol_WindowBg]             = ImVec4(0.955f, 0.960f, 0.972f, 1.00f);
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

    // Accent swatches. Picking one re-themes the whole menu, slider fills included.
    static void AccentPresets(Config& cfg)
    {
        static const ImVec4 presets[] = {
            ImVec4(0.20f, 0.45f, 1.00f, 1.0f),   // blue
            ImVec4(0.93f, 0.26f, 0.30f, 1.0f),   // red
            ImVec4(0.55f, 0.36f, 0.96f, 1.0f),   // violet
            ImVec4(0.06f, 0.73f, 0.51f, 1.0f),   // green
            ImVec4(0.96f, 0.62f, 0.04f, 1.0f),   // amber
        };
        const float d = ImGui::GetFrameHeight() * 0.85f;
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
        static const char* boxStyles[] = { "Full box", "Corners" };

        Section("PLAYER ESP");
        Toggle("Enable ESP", &cfg.espEnabled);
        Toggle("Box", &cfg.espBox);
        Toggle("Name", &cfg.espName);
        Toggle("Health bar", &cfg.espHealth);
        Toggle("Distance", &cfg.espDistance);
        ComboRow("Box style", &cfg.espBoxStyle, boxStyles, 2);
        SliderRow("Max distance", &cfg.espMaxDistance, 25.0f, 1000.0f, "%.0f m");
        ColorRow("Box color", cfg.espBoxColor);
        ColorRow("Text color", cfg.espTextColor);

        Section("RADAR");
        Toggle("Enable radar", &cfg.radarEnabled);
        SliderRow("Radar range", &cfg.radarRange, 25.0f, 500.0f, "%.0f m");
    }

    static void TabAim(Config& cfg)
    {
        static const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
        static const char* activations[] = { "Always", "Scope only", "Scope and firing", "Firing only" };

        Section("AIM ASSIST");
        Toggle("Enable aim assist", &cfg.aimEnabled);
        Toggle("Silent aim", &cfg.silentAim);
        Toggle("Aim 360", &cfg.aim360);
        SliderRow("Assist strength", &cfg.aimStrength, 10.0f, 100.0f, "%.0f %%", 10.0f);
        ComboRow("Target bone", &cfg.aimBone, bones, 4);
        ComboRow("Activation", &cfg.aimActivation, activations, 4);

        Section("TARGETING");
        ImGui::BeginDisabled(cfg.aim360);   // FOV does not apply when aiming all around
        SliderRow("FOV", &cfg.aimFov, 1.0f, 180.0f, "%.0f deg");
        Toggle("Draw FOV circle", &cfg.aimDrawFov);
        ImGui::EndDisabled();
        SliderRow("Aim distance", &cfg.aimDistance, 25.0f, 1000.0f, "%.0f m");
        SliderRow("Smoothing (1 = instant)", &cfg.aimSmooth, 1.0f, 30.0f, "%.1f");
        Toggle("Visible targets only", &cfg.aimVisibleOnly);
    }

    static void TabMisc(Config& cfg)
    {
        Section("WEAPON");
        Toggle("Recoil adjustment", &cfg.recoilAdjust);
        SliderRow("Recoil strength", &cfg.recoilStrength, 0.0f, 100.0f, "%.0f %%");

        Section("INTERFACE");
        Toggle("Watermark", &cfg.showWatermark);
        Toggle("FPS counter", &cfg.showFps);
        SliderRow("Menu opacity", &cfg.menuOpacity, 0.5f, 1.0f, "%.2f");
    }

    static void TabSettings(Config& cfg)
    {
        Section("THEME");
        ImGui::TextUnformatted("Accent presets");
        AccentPresets(cfg);
        ImGui::TextUnformatted("Custom accent");
        ImGui::SetNextItemWidth(-RightPad());
        if (ImGui::ColorEdit3("##accent", cfg.accent,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel |
                ImGuiColorEditFlags_PickerHueWheel))
        {
            ApplyWhiteTheme(g_scale, ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], 1.0f));
        }

        Section("CONFIG");
        if (ImGui::Button("Reset to defaults", ImVec2(-RightPad(), 0)))
        {
            cfg = Config();
            ApplyWhiteTheme(g_scale, ImVec4(cfg.accent[0], cfg.accent[1], cfg.accent[2], 1.0f));
        }
    }

    // ---------- main window ----------

    void Render(bool* open)
    {
        if (open && !*open) return;

        Config& cfg = Cfg();
        ImGuiIO& io = ImGui::GetIO();
        const float u = ImGui::GetFontSize();

        ImGui::GetStyle().Alpha = std::clamp(cfg.menuOpacity, 0.5f, 1.0f);

        const ImVec2 size(std::min(io.DisplaySize.x * 0.94f, u * 52.0f),
                          std::min(io.DisplaySize.y * 0.88f, u * 32.0f));
        ImGui::SetNextWindowSize(size, ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f),
                                ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));

        const ImGuiWindowFlags wf = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                                    ImGuiWindowFlags_NoSavedSettings;

        static int tab = 0;

        if (ImGui::Begin("##eclipse_menu", nullptr, wf))
        {
            // Header (drag here to move the window)
            ImGui::AlignTextToFramePadding();
            ImGui::PushStyleColor(ImGuiCol_Text, g_accent);
            ImGui::TextUnformatted(kTitle);
            ImGui::PopStyleColor();
            if (open)
            {
                ImGui::SameLine();
                const float bw = u * 4.0f;
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() - bw - ImGui::GetStyle().WindowPadding.x);
                if (ImGui::Button("Hide", ImVec2(bw, 0))) *open = false;
            }
            ImGui::Dummy(ImVec2(0, u * 0.1f));

            // Sidebar
            const float sideW = u * 9.5f;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, Gray(1.0f));
            ImGui::BeginChild("##side", ImVec2(sideW, 0), false, ImGuiWindowFlags_NoScrollbar);
            {
                const float pad = u * 0.6f;
                ImGui::SetCursorPos(ImVec2(pad, pad));
                const float tw = sideW - pad * 2.0f;

                static const char* names[] = { "Visuals", "Aim", "Misc", "Settings" };
                for (int i = 0; i < 4; ++i)
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
                    case 2: TabMisc(cfg);     break;
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

    void RenderHud(const char* title)
    {
        const Config& cfg = Cfg();
        if (!cfg.showWatermark && !cfg.showFps) return;

        char buf[96];
        if (cfg.showWatermark && cfg.showFps)
            std::snprintf(buf, sizeof(buf), "%s  |  %.0f fps", title, ImGui::GetIO().Framerate);
        else if (cfg.showWatermark)
            std::snprintf(buf, sizeof(buf), "%s", title);
        else
            std::snprintf(buf, sizeof(buf), "%.0f fps", ImGui::GetIO().Framerate);

        const float u = ImGui::GetFontSize();
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        const ImVec2 a(u * 0.8f, u * 0.8f);
        const ImVec2 b(a.x + ts.x + u * 1.4f, a.y + ts.y + u * 0.8f);

        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddRectFilled(a, b, IM_COL32(255, 255, 255, 235), (b.y - a.y) * 0.5f);
        dl->AddRect(a, b, IM_COL32(0, 0, 0, 24), (b.y - a.y) * 0.5f);
        dl->AddCircleFilled(ImVec2(a.x + u * 0.6f, (a.y + b.y) * 0.5f), u * 0.18f, Col(g_accent), 16);
        dl->AddText(ImVec2(a.x + u * 1.0f, a.y + u * 0.4f), IM_COL32(25, 28, 36, 255), buf);
    }
}
