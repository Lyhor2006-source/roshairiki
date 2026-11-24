#import <UIKit/UIKit.h>
#import <Metal/Metal.h>
#include "PythonBridge.hpp"
#include "MemoryCheats.hpp"
#include "StealthBypass.hpp"
#include "Config.hpp"
#include <thread>
#include <chrono>

@interface RyomenXOverlayView : UIView
- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device;
- (void)attachThreeFingerTapToWindow:(UIWindow*)window;
@end

static RyomenXOverlayView* g_overlay_view = nil;

static void AttachOverlayToWindow() {
    dispatch_async(dispatch_get_main_queue(), ^{
        UIWindow* key_window = nil;

        // Compatible across iOS 13+ UIScene and legacy UIWindow
        if (@available(iOS 13.0, *)) {
            for (UIWindowScene* scene in [UIApplication sharedApplication].connectedScenes) {
                if (scene.activationState == UISceneActivationStateForegroundActive) {
                    for (UIWindow* w in scene.windows) {
                        if (w.isKeyWindow) {
                            key_window = w;
                            break;
                        }
                    }
                }
            }
        }

        if (!key_window) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
            key_window = [UIApplication sharedApplication].keyWindow;
#pragma clang diagnostic pop
        }
        if (!key_window) {
            key_window = [UIApplication sharedApplication].windows.firstObject;
        }

        if (!key_window) {
            // Retry after brief delay if window not loaded yet
            dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.0 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                AttachOverlayToWindow();
            });
            return;
        }

        if (!g_overlay_view) {
            id<MTLDevice> device = MTLCreateSystemDefaultDevice();
            if (!device) return;

            g_overlay_view = [[RyomenXOverlayView alloc] initWithFrame:key_window.bounds device:device];
            [key_window addSubview:g_overlay_view];
            [key_window bringSubviewToFront:g_overlay_view];
        }

        [g_overlay_view attachThreeFingerTapToWindow:key_window];
    });
}

// Dylib Constructor: Injected automatically upon Mach-O load
__attribute__((constructor))
static void RyomenX_Init() {
    // Defer initialization to main queue after UIKit and dyld have fully settled
    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(1.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
        // 1. Initialize Anti-Detection & IDFV Bypass
        RyomenX::StealthBypass::Initialize();

        // 2. Setup sandbox cache directory: Library/Caches/RyomenX
        NSArray* paths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
        NSString* cache_base = paths.firstObject ? paths.firstObject : NSTemporaryDirectory();
        NSString* ryomen_dir = [cache_base stringByAppendingPathComponent:@"RyomenX"];

        [[NSFileManager defaultManager] createDirectoryAtPath:ryomen_dir
                                  withIntermediateDirectories:YES
                                                   attributes:nil
                                                        error:nil];

        std::string cache_dir_std = ryomen_dir.UTF8String;
        MemoryCheats::SetDiagnosticsDirectory(cache_dir_std);

        // 3. Initialize Memory Cheats Engine
        MemoryCheats::Initialize();

        // 4. Spawn Python Bridge Worker safely outside dyld lock
        std::thread([cache_dir_std]() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            RyomenX::PythonBridge::Get().Initialize(cache_dir_std);
        }).detach();

        // 5. Attach Metal UI Overlay to Main Window
        AttachOverlayToWindow();
    });
}

// C-API Exports matching reverse engineered symbols
extern "C" {

void __ryomenx_ios_show_menu(int show) {
    RyomenX::Settings::Get().menu_open = (show != 0);
}

float __ryomenx_ios_get_battery_level() {
    [UIDevice currentDevice].batteryMonitoringEnabled = YES;
    return [UIDevice currentDevice].batteryLevel * 100.0f;
}

int __ryomenx_ios_is_battery_charging() {
    [UIDevice currentDevice].batteryMonitoringEnabled = YES;
    return ([UIDevice currentDevice].batteryState == UIDeviceBatteryStateCharging ||
            [UIDevice currentDevice].batteryState == UIDeviceBatteryStateFull) ? 1 : 0;
}

void __ryomenx_ios_esp_diagnostic() {
    RyomenX::PythonBridge::Get().UpdateFromEspFile();
}

}
