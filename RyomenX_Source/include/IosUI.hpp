#pragma once

#include "Config.hpp"
#include "MemoryCheats.hpp"

struct ImDrawList;
struct ImVec2;

namespace IosUI {

// Main UI rendering components
void RenderFloatingBar();
void RenderMainMenu();
void RenderEspOverlay(ImDrawList* draw_list, ImVec2 display_size);
void RenderFeatureRow(MemoryCheats::Feature feature, float default_slider = 0.0f);

// Touch helpers
bool IsTouchInsideUI(float touch_x, float touch_y);

} // namespace IosUI
