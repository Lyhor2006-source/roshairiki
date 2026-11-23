#include "Config.hpp"

#if defined(__APPLE__)
#import <Foundation/Foundation.h>
#endif

namespace RyomenX {

void Settings::LoadDefaults() {
    menu_open = false;
    show_floating_bar = true;
    menu_scale = 1.0f;
    bar_pos_x = 100.0f;
    bar_pos_y = 100.0f;
    current_tab = 0;

    master_esp = true;
    esp_style = 0;
    box_style = BoxStyle::Corner;
    show_box = true;
    show_skeleton = true;
    show_health = true;
    show_name = true;
    show_team = true;
    show_player_id = false;
    show_lines = true;
    line_origin = LineOrigin::Bottom;
    show_distance = true;
    show_arrows = true;
    show_hud_count = true;
    filter_bots = false;
    esp_range = 500.0f;

    enemy_color = {1.0f, 0.2f, 0.2f, 1.0f};
    bot_color = {0.2f, 0.7f, 1.0f, 1.0f};
    skeleton_color = {1.0f, 1.0f, 1.0f, 1.0f};
    line_color = {1.0f, 0.9f, 0.0f, 0.8f};

    ipad_view = false;
    ipad_fov = 95.0f;

    master_aim = false;
    aim_trigger = AimTrigger::Always;
    silent_aim = false;
    silent_360 = false;
    instant_aim = false;
    visible_check = true;
    ignore_knocked = true;
    target_bots = true;
    show_fov_circle = true;
    target_bone = TargetBone::Head;
    fov_radius = 120.0f;
    aim_distance = 300.0f;
    aim_power = 1.5f;
    silent_distance = 150.0f;
    aim_smoothness = 3.0f;

    screen_crosshair = false;
    show_watermark = true;
    selected_region_mode = 0;
}

void Settings::LoadFromPreferences() {
#if defined(__APPLE__)
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSDictionary* dict = [defaults dictionaryForKey:@"RyomenX.Settings.v1"];
    if (!dict) {
        LoadDefaults();
        return;
    }

    if (dict[@"esp"]) master_esp = [dict[@"esp"] boolValue];
    if (dict[@"box"]) show_box = [dict[@"box"] boolValue];
    if (dict[@"skeleton"]) show_skeleton = [dict[@"skeleton"] boolValue];
    if (dict[@"health"]) show_health = [dict[@"health"] boolValue];
    if (dict[@"name"]) show_name = [dict[@"name"] boolValue];
    if (dict[@"team"]) show_team = [dict[@"team"] boolValue];
    if (dict[@"lines"]) show_lines = [dict[@"lines"] boolValue];
    if (dict[@"distance"]) show_distance = [dict[@"distance"] boolValue];
    if (dict[@"filter_bots"]) filter_bots = [dict[@"filter_bots"] boolValue];
    if (dict[@"esp_range"]) esp_range = [dict[@"esp_range"] floatValue];
    if (dict[@"scale"]) menu_scale = [dict[@"scale"] floatValue];
    if (dict[@"crosshair"]) screen_crosshair = [dict[@"crosshair"] boolValue];
    if (dict[@"watermark"]) show_watermark = [dict[@"watermark"] boolValue];
    if (dict[@"bar_x"]) bar_pos_x = [dict[@"bar_x"] floatValue];
    if (dict[@"bar_y"]) bar_pos_y = [dict[@"bar_y"] floatValue];
    if (dict[@"ipad_view"]) ipad_view = [dict[@"ipad_view"] boolValue];
    if (dict[@"ipad_fov"]) ipad_fov = [dict[@"ipad_fov"] floatValue];

    if (dict[@"aim"]) master_aim = [dict[@"aim"] boolValue];
    if (dict[@"fov"]) fov_radius = [dict[@"fov"] floatValue];
    if (dict[@"aim_range"]) aim_distance = [dict[@"aim_range"] floatValue];
    if (dict[@"silent"]) silent_aim = [dict[@"silent"] boolValue];
    if (dict[@"visible"]) visible_check = [dict[@"visible"] boolValue];
    if (dict[@"ignore_knocked"]) ignore_knocked = [dict[@"ignore_knocked"] boolValue];
    if (dict[@"fov_circle"]) show_fov_circle = [dict[@"fov_circle"] boolValue];
    if (dict[@"smooth"]) aim_smoothness = [dict[@"smooth"] floatValue];
#else
    LoadDefaults();
#endif
}

void Settings::SaveToPreferences() {
#if defined(__APPLE__)
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSMutableDictionary* dict = [NSMutableDictionary dictionary];

    dict[@"esp"] = @(master_esp);
    dict[@"box"] = @(show_box);
    dict[@"skeleton"] = @(show_skeleton);
    dict[@"health"] = @(show_health);
    dict[@"name"] = @(show_name);
    dict[@"team"] = @(show_team);
    dict[@"lines"] = @(show_lines);
    dict[@"distance"] = @(show_distance);
    dict[@"filter_bots"] = @(filter_bots);
    dict[@"esp_range"] = @(esp_range);
    dict[@"scale"] = @(menu_scale);
    dict[@"crosshair"] = @(screen_crosshair);
    dict[@"watermark"] = @(show_watermark);
    dict[@"bar_x"] = @(bar_pos_x);
    dict[@"bar_y"] = @(bar_pos_y);
    dict[@"ipad_view"] = @(ipad_view);
    dict[@"ipad_fov"] = @(ipad_fov);

    dict[@"aim"] = @(master_aim);
    dict[@"fov"] = @(fov_radius);
    dict[@"aim_range"] = @(aim_distance);
    dict[@"silent"] = @(silent_aim);
    dict[@"visible"] = @(visible_check);
    dict[@"ignore_knocked"] = @(ignore_knocked);
    dict[@"fov_circle"] = @(show_fov_circle);
    dict[@"smooth"] = @(aim_smoothness);

    [defaults setObject:dict forKey:@"RyomenX.Settings.v1"];
    [defaults synchronize];
#endif
}

} // namespace RyomenX
