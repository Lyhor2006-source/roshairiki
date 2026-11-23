#include "StealthBypass.hpp"

#if defined(__APPLE__)
#import <UIKit/UIKit.h>
#import <objc/runtime.h>
#endif

namespace RyomenX {
namespace StealthBypass {

static bool s_stealth_active = false;
static std::string s_custom_idfv = "";

#if defined(__APPLE__)

static NSUUID* (*orig_identifierForVendor)(id, SEL) = nullptr;
static BOOL (*orig_fileExistsAtPath)(id, SEL, NSString*) = nullptr;

static NSUUID* hooked_identifierForVendor(id self, SEL _cmd) {
    if (!s_custom_idfv.empty()) {
        NSString* uuid_str = [NSString stringWithUTF8String:s_custom_idfv.c_str()];
        NSUUID* custom_uuid = [[NSUUID alloc] initWithUUIDString:uuid_str];
        if (custom_uuid) return custom_uuid;
    }
    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    NSString* saved = [defaults stringForKey:@"RyomenX.SpoofedIDFV"];
    if (saved) {
        NSUUID* saved_uuid = [[NSUUID alloc] initWithUUIDString:saved];
        if (saved_uuid) return saved_uuid;
    }
    return orig_identifierForVendor ? orig_identifierForVendor(self, _cmd) : nil;
}

static BOOL hooked_fileExistsAtPath(id self, SEL _cmd, NSString* path) {
    if (path && [path isKindOfClass:[NSString class]]) {
        // Hide common jailbreak detection paths
        if ([path containsString:@"Cydia.app"] ||
            [path containsString:@"Sileo.app"] ||
            [path containsString:@"MobileSubstrate"] ||
            [path containsString:@"/bin/bash"] ||
            [path containsString:@"/usr/sbin/sshd"] ||
            [path containsString:@"/etc/apt"] ||
            [path containsString:@"RyomenX"]) {
            return NO;
        }
    }
    return orig_fileExistsAtPath ? orig_fileExistsAtPath(self, _cmd, path) : NO;
}

#endif

void Initialize() {
#if defined(__APPLE__)
    // 1. Hook UIDevice identifierForVendor
    Class uiDeviceClass = [UIDevice class];
    Method idfvMethod = class_getInstanceMethod(uiDeviceClass, @selector(identifierForVendor));
    if (idfvMethod) {
        orig_identifierForVendor = (NSUUID* (*)(id, SEL))method_getImplementation(idfvMethod);
        method_setImplementation(idfvMethod, (IMP)hooked_identifierForVendor);
    }

    // 2. Hook NSFileManager fileExistsAtPath:
    Class fmClass = [NSFileManager class];
    Method existsMethod = class_getInstanceMethod(fmClass, @selector(fileExistsAtPath:));
    if (existsMethod) {
        orig_fileExistsAtPath = (BOOL (*)(id, SEL, NSString*))method_getImplementation(existsMethod);
        method_setImplementation(existsMethod, (IMP)hooked_fileExistsAtPath);
    }

    s_stealth_active = true;
#else
    s_stealth_active = true;
#endif
}

std::string GetDeviceIDFV() {
#if defined(__APPLE__)
    NSUUID* idfv = [[UIDevice currentDevice] identifierForVendor];
    return idfv ? idfv.UUIDString.UTF8String : "00000000-0000-0000-0000-000000000000";
#else
    return "00000000-0000-0000-0000-000000000000";
#endif
}

std::string RandomizeDeviceIDFV() {
#if defined(__APPLE__)
    NSString* new_uuid = [[NSUUID UUID] UUIDString];
    s_custom_idfv = new_uuid.UTF8String;

    NSUserDefaults* defaults = [NSUserDefaults standardUserDefaults];
    [defaults setObject:new_uuid forKey:@"RyomenX.SpoofedIDFV"];
    [defaults synchronize];

    return s_custom_idfv;
#else
    s_custom_idfv = "RANDOMIZED-UUID-DEVICE-SPOOF";
    return s_custom_idfv;
#endif
}

bool IsStealthActive() {
    return s_stealth_active;
}

} // namespace StealthBypass
} // namespace RyomenX
