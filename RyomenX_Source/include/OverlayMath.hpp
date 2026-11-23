#pragma once

#include <cmath>
#include <cstdint>

struct ImDrawList;

namespace OverlayMath {

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float f) const { return {x * f, y * f}; }
    float Length() const { return std::sqrt(x * x + y * y); }
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    Vec3() = default;
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float f) const { return {x * f, y * f, z * f}; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float Distance(const Vec3& o) const { return (*this - o).Length(); }
};

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;

    Vec4() = default;
    Vec4(float _x, float _y, float _z, float _w) : x(_x), y(_y), z(_z), w(_w) {}
};

struct Matrix4x4 {
    float m[16];

    Matrix4x4() {
        for (int i = 0; i < 16; ++i) m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
    }
};

struct EspBox {
    float min_x = 0.0f;
    float min_y = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;

    bool IsValid() const {
        if (std::isnan(min_x) || std::isinf(min_x) ||
            std::isnan(min_y) || std::isinf(min_y) ||
            std::isnan(max_x) || std::isinf(max_x) ||
            std::isnan(max_y) || std::isinf(max_y)) {
            return false;
        }
        return (max_x > min_x) && (max_y > min_y);
    }

    float Width() const { return max_x - min_x; }
    float Height() const { return max_y - min_y; }
};

// Box rendering routines extracted directly from RyomenX
void Draw2DBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness);
void DrawCornerBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness);
void Draw3DBox(ImDrawList* draw_list, const EspBox& box, uint32_t color, float thickness);

// World-To-Screen matrix multiplication
bool WorldToScreen(const Vec3& world_pos, const Matrix4x4& view_proj, float screen_w, float screen_h, Vec2& screen_out);

} // namespace OverlayMath
