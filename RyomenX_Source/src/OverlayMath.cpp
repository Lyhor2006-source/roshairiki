#include "OverlayMath.hpp"
#include <imgui.h>
#include <cmath>

namespace OverlayMath {

void Draw2DBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness) {
    if (!draw_list || !box.IsValid()) return;

    // Corner coordinates
    ImVec2 min_pt(box.min_x, box.min_y);
    ImVec2 max_pt(box.max_x, box.max_y);

    // Draw outline box
    draw_list->AddRect(min_pt, max_pt, color, 0.0f, 0, thickness);

    // Draw thin black drop-shadow / outer border for visibility on bright textures
    draw_list->AddRect(ImVec2(min_pt.x - 1.0f, min_pt.y - 1.0f),
                       ImVec2(max_pt.x + 1.0f, max_pt.y + 1.0f),
                       IM_COL32(0, 0, 0, 160), 0.0f, 0, 1.0f);
}

void DrawCornerBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness) {
    if (!draw_list || !box.IsValid()) return;

    float w = box.Width();
    float h = box.Height();
    float corner_len = w * 0.25f;
    if (corner_len < 4.0f) corner_len = 4.0f;

    float min_x = box.min_x;
    float min_y = box.min_y;
    float max_x = box.max_x;
    float max_y = box.max_y;

    // Top-Left Corner
    draw_list->AddLine(ImVec2(min_x, min_y), ImVec2(min_x + corner_len, min_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, min_y), ImVec2(min_x, min_y + corner_len), color, thickness);

    // Top-Right Corner
    draw_list->AddLine(ImVec2(max_x, min_y), ImVec2(max_x - corner_len, min_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, min_y), ImVec2(max_x, min_y + corner_len), color, thickness);

    // Bottom-Left Corner
    draw_list->AddLine(ImVec2(min_x, max_y), ImVec2(min_x + corner_len, max_y), color, thickness);
    draw_list->AddLine(ImVec2(min_x, max_y), ImVec2(min_x, max_y - corner_len), color, thickness);

    // Bottom-Right Corner
    draw_list->AddLine(ImVec2(max_x, max_y), ImVec2(max_x - corner_len, max_y), color, thickness);
    draw_list->AddLine(ImVec2(max_x, max_y), ImVec2(max_x, max_y - corner_len), color, thickness);
}

void Draw3DBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness) {
    if (!draw_list || !box.IsValid()) return;

    float min_x = box.min_x;
    float min_y = box.min_y;
    float max_x = box.max_x;
    float max_y = box.max_y;

    // Perspective depth projection offsets decompiled from RyomenX
    // offset_x = (max_x - min_x) * 0.22f
    // offset_y = (max_y - min_y) * (-0.12f)
    float off_x = (max_x - min_x) * 0.22f;
    float off_y = (max_y - min_y) * -0.12f;

    // Front Face (4 points)
    ImVec2 f_tl(min_x, min_y);
    ImVec2 f_tr(max_x, min_y);
    ImVec2 f_br(max_x, max_y);
    ImVec2 f_bl(min_x, max_y);

    // Back Face (4 points projected with depth offset)
    ImVec2 b_tl(min_x + off_x, min_y + off_y);
    ImVec2 b_tr(max_x + off_x, min_y + off_y);
    ImVec2 b_br(max_x + off_x, max_y + off_y);
    ImVec2 b_bl(min_x + off_x, max_y + off_y);

    // Draw Front Rect
    draw_list->AddLine(f_tl, f_tr, color, thickness);
    draw_list->AddLine(f_tr, f_br, color, thickness);
    draw_list->AddLine(f_br, f_bl, color, thickness);
    draw_list->AddLine(f_bl, f_tl, color, thickness);

    // Draw Back Rect
    draw_list->AddLine(b_tl, b_tr, color, thickness);
    draw_list->AddLine(b_tr, b_br, color, thickness);
    draw_list->AddLine(b_br, b_bl, color, thickness);
    draw_list->AddLine(b_bl, b_tl, color, thickness);

    // Connect Front to Back (4 edges)
    draw_list->AddLine(f_tl, b_tl, color, thickness);
    draw_list->AddLine(f_tr, b_tr, color, thickness);
    draw_list->AddLine(f_br, b_br, color, thickness);
    draw_list->AddLine(f_bl, b_bl, color, thickness);
}

bool WorldToScreen(const Vec3& world_pos, const Matrix4x4& view_proj, float screen_w, float screen_h, Vec2& screen_out) {
    float w = world_pos.x * view_proj.m[3] +
              world_pos.y * view_proj.m[7] +
              world_pos.z * view_proj.m[11] +
              view_proj.m[15];

    if (w <= 0.001f) return false;

    float inv_w = 1.0f / w;
    float x = (world_pos.x * view_proj.m[0] +
               world_pos.y * view_proj.m[4] +
               world_pos.z * view_proj.m[8] +
               view_proj.m[12]) * inv_w;

    float y = (world_pos.x * view_proj.m[1] +
               world_pos.y * view_proj.m[5] +
               world_pos.z * view_proj.m[9] +
               view_proj.m[13]) * inv_w;

    screen_out.x = (screen_w * 0.5f) + (x * screen_w * 0.5f);
    screen_out.y = (screen_h * 0.5f) - (y * screen_h * 0.5f);
    return true;
}

} // namespace OverlayMath
