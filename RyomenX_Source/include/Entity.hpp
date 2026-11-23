#pragma once

#include "OverlayMath.hpp"
#include <string>
#include <vector>

namespace RyomenX {

enum WeaponType : int {
    WEAPON_UNKNOWN = 0,
    WEAPON_RIFLE = 1,
    WEAPON_SNIPER = 2,
    WEAPON_SMG = 3,
    WEAPON_SHOTGUN = 4,
    WEAPON_PISTOL = 5,
    WEAPON_MELEE = 6,
    WEAPON_GRENADE = 7
};

struct BoneJoint {
    bool valid = false;
    float x = 0.0f;
    float y = 0.0f;
};

// 16 Standard Bones supported by NeoX / BigWorld extracted from Python bridge
enum BoneIndex : int {
    BONE_HEAD = 0,
    BONE_NECK = 1,
    BONE_CHEST = 2,
    BONE_PELVIS = 3,
    BONE_L_SHOULDER = 4,
    BONE_L_ELBOW = 5,
    BONE_L_HAND = 6,
    BONE_R_SHOULDER = 7,
    BONE_R_ELBOW = 8,
    BONE_R_HAND = 9,
    BONE_L_HIP = 10,
    BONE_L_KNEE = 11,
    BONE_L_FOOT = 12,
    BONE_R_HIP = 13,
    BONE_R_KNEE = 14,
    BONE_R_FOOT = 15,
    BONE_COUNT = 16
};

struct BoneLink {
    int from;
    int to;
};

// 15 Bone skeleton link pairs matching RyomenX address 0xa7d20
static const BoneLink SKELETON_LINKS[15] = {
    {BONE_HEAD, BONE_NECK},
    {BONE_NECK, BONE_CHEST},
    {BONE_CHEST, BONE_PELVIS},
    {BONE_CHEST, BONE_L_SHOULDER},
    {BONE_L_SHOULDER, BONE_L_ELBOW},
    {BONE_L_ELBOW, BONE_L_HAND},
    {BONE_CHEST, BONE_R_SHOULDER},
    {BONE_R_SHOULDER, BONE_R_ELBOW},
    {BONE_R_ELBOW, BONE_R_HAND},
    {BONE_PELVIS, BONE_L_HIP},
    {BONE_L_HIP, BONE_L_KNEE},
    {BONE_L_KNEE, BONE_L_FOOT},
    {BONE_PELVIS, BONE_R_HIP},
    {BONE_R_HIP, BONE_R_KNEE},
    {BONE_R_KNEE, BONE_R_FOOT}
};

struct PlayerEntity {
    // Screen coords
    OverlayMath::Vec2 feet_screen;
    OverlayMath::Vec2 head_screen;
    bool is_on_screen = false;

    // 3D coords
    OverlayMath::Vec3 world_pos;
    OverlayMath::Vec3 view_pos;

    // Attributes
    int hp = 100;
    int max_hp = 100;
    int is_enemy = 1;
    int is_visible = 0;
    float distance = 0.0f;
    int is_knocked = 0;
    int is_bot = 0;
    int team_id = 0;
    int shield = 0;
    int max_shield = 0;
    int in_vehicle = 0;
    uint64_t entity_id = 0;
    std::string name;
    int weapon_kind = 0;

    // Skeletons
    BoneJoint bones[BONE_COUNT];

    OverlayMath::EspBox GetBox() const {
        OverlayMath::EspBox box;
        float h = std::abs(feet_screen.y - head_screen.y);
        if (h < 5.0f) h = 40.0f;
        float w = h * 0.48f;
        float center_x = (feet_screen.x + head_screen.x) * 0.5f;
        float top_y = (head_screen.y < feet_screen.y) ? head_screen.y : feet_screen.y;

        box.min_x = center_x - (w * 0.5f);
        box.max_x = center_x + (w * 0.5f);
        box.min_y = top_y;
        box.max_y = top_y + h;
        return box;
    }
};

} // namespace RyomenX
