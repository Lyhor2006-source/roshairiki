#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <UIKit/UIKit.h>

#include "IosUI.hpp"
#include "Menu.h"
#include "Config.hpp"
#include <imgui.h>

// Standard ImGui Metal backend declarations
bool ImGui_ImplMetal_Init(id<MTLDevice> device);
void ImGui_ImplMetal_NewFrame(MTLRenderPassDescriptor* renderPassDescriptor);
void ImGui_ImplMetal_RenderDrawData(ImDrawData* draw_data, id<MTLCommandBuffer> commandBuffer, id<MTLRenderCommandEncoder> commandEncoder);

static bool s_menu_open = true;

@interface RyomenXOverlayView : MTKView <MTKViewDelegate, UIGestureRecognizerDelegate>
@property (nonatomic, strong) id<MTLCommandQueue> command_queue;
@property (nonatomic, assign) UITouch* primary_touch;
@property (nonatomic, assign) double last_frame_time;
- (void)attachThreeFingerTapToWindow:(UIWindow*)window;
@end

@implementation RyomenXOverlayView

- (void)ensureLandscapeBoundsAndScale {
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    CGFloat w = MAX(screenBounds.size.width, screenBounds.size.height);
    CGFloat h = MIN(screenBounds.size.width, screenBounds.size.height);
    CGRect target = CGRectMake(0, 0, w, h);

    if (!CGRectEqualToRect(self.frame, target)) {
        self.frame = target;
        self.bounds = target;
    }

    CGFloat scale = [UIScreen mainScreen].scale;
    if (scale < 1.0f) scale = 2.0f;
    self.contentScaleFactor = scale;

    CGSize targetDrawable = CGSizeMake(w * scale, h * scale);
    if (!CGSizeEqualToSize(self.drawableSize, targetDrawable)) {
        self.drawableSize = targetDrawable;
    }
}

- (instancetype)initWithFrame:(CGRect)frame device:(id<MTLDevice>)device {
    CGRect screenBounds = [UIScreen mainScreen].bounds;
    CGFloat w = MAX(screenBounds.size.width, screenBounds.size.height);
    CGFloat h = MIN(screenBounds.size.width, screenBounds.size.height);
    CGRect landscapeFrame = CGRectMake(0, 0, w, h);

    self = [super initWithFrame:landscapeFrame device:device];
    if (self) {
        self.device = device;
        self.command_queue = [device newCommandQueue];
        self.delegate = self;

        CGFloat scale = [UIScreen mainScreen].scale;
        if (scale < 1.0f) scale = 2.0f;
        self.contentScaleFactor = scale;
        self.drawableSize = CGSizeMake(w * scale, h * scale);

        self.backgroundColor = [UIColor clearColor];
        self.opaque = NO;
        self.clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
        self.colorPixelFormat = MTLPixelFormatBGRA8Unorm;
        self.framebufferOnly = YES;
        self.userInteractionEnabled = YES;
        self.multipleTouchEnabled = YES;
        self.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;

        // Initialize Dear ImGui
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(w, h);
        io.DisplayFramebufferScale = ImVec2(scale, scale);
        io.IniFilename = nullptr; // Stateless configuration

        // Touch-friendly scaling for mobile retina display
        io.FontGlobalScale = 1.35f;

        // Setup Eclipse White Modern Theme scaled for mobile touch
        Menu::ApplyWhiteTheme(1.35f, ImVec4(0.20f, 0.45f, 1.00f, 1.0f));

        ImGui_ImplMetal_Init(device);

        // Load saved preferences
        RyomenX::Settings::Get().LoadFromPreferences();

        // 3-Finger Tap Gesture on overlay view
        UITapGestureRecognizer* selfTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleThreeFingerTap:)];
        selfTap.numberOfTouchesRequired = 3;
        selfTap.numberOfTapsRequired = 1;
        selfTap.cancelsTouchesInView = NO;
        selfTap.delaysTouchesBegan = NO;
        selfTap.delaysTouchesEnded = NO;
        selfTap.delegate = self;
        [self addGestureRecognizer:selfTap];
    }
    return self;
}

- (void)didMoveToWindow {
    [super didMoveToWindow];
    if (self.window) {
        [self attachThreeFingerTapToWindow:self.window];
    }
}

// Attach 3-finger tap recognizer to root window so it opens even when overlay passes touches through
- (void)attachThreeFingerTapToWindow:(UIWindow*)window {
    if (!window) return;
    for (UIGestureRecognizer* gr in window.gestureRecognizers) {
        if ([gr isKindOfClass:[UITapGestureRecognizer class]]) {
            UITapGestureRecognizer* tap = (UITapGestureRecognizer*)gr;
            if (tap.numberOfTouchesRequired == 3) {
                return; // Already attached
            }
        }
    }
    UITapGestureRecognizer* winTap = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleThreeFingerTap:)];
    winTap.numberOfTouchesRequired = 3;
    winTap.numberOfTapsRequired = 1;
    winTap.cancelsTouchesInView = NO;
    winTap.delaysTouchesBegan = NO;
    winTap.delaysTouchesEnded = NO;
    winTap.delegate = self;
    [window addGestureRecognizer:winTap];
}

- (void)handleThreeFingerTap:(UITapGestureRecognizer*)recognizer {
    if (recognizer.state == UIGestureRecognizerStateEnded) {
        s_menu_open = !s_menu_open;
        RyomenX::Settings::Get().menu_open = s_menu_open;
    }
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    return YES;
}

// Touch Passthrough: Allows gameplay controls to receive touches outside of cheat menus
- (BOOL)pointInside:(CGPoint)point withEvent:(UIEvent*)event {
    return Menu::IsTouchInside(point.x, point.y, s_menu_open);
}

- (void)updateTouch:(NSSet<UITouch*>*)touches {
    ImGuiIO& io = ImGui::GetIO();
    for (UITouch* touch in touches) {
        CGPoint pt = [touch locationInView:self];
        io.AddMousePosEvent(pt.x, pt.y);

        if (touch.phase == UITouchPhaseBegan) {
            io.AddMouseButtonEvent(0, true);
        } else if (touch.phase == UITouchPhaseEnded || touch.phase == UITouchPhaseCancelled) {
            io.AddMouseButtonEvent(0, false);
        }
    }
}

- (void)touchesBegan:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self updateTouch:touches];
}

- (void)touchesMoved:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self updateTouch:touches];
}

- (void)touchesEnded:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self updateTouch:touches];
}

- (void)touchesCancelled:(NSSet<UITouch*>*)touches withEvent:(UIEvent*)event {
    [self updateTouch:touches];
}

#pragma mark - MTKViewDelegate

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(size.width, size.height);
}

- (void)drawInMTKView:(MTKView*)view {
    MTLRenderPassDescriptor* renderPassDesc = view.currentRenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;

    if (!renderPassDesc || !drawable) return;

    [self ensureLandscapeBoundsAndScale];

    ImGuiIO& io = ImGui::GetIO();
    CGFloat scale = self.contentScaleFactor > 0.0f ? self.contentScaleFactor : [UIScreen mainScreen].scale;
    if (scale < 1.0f) scale = 2.0f;

    io.DisplaySize = ImVec2(self.bounds.size.width, self.bounds.size.height);
    io.DisplayFramebufferScale = ImVec2(scale, scale);

    double current_time = CACurrentMediaTime();
    io.DeltaTime = self.last_frame_time > 0.0 ? (float)(current_time - self.last_frame_time) : (1.0f / 60.0f);
    self.last_frame_time = current_time;

    // Sync state with global settings
    s_menu_open = RyomenX::Settings::Get().menu_open;

    // Begin ImGui Metal Frame
    ImGui_ImplMetal_NewFrame(renderPassDesc);
    ImGui::NewFrame();

    ImDrawList* background_draw_list = ImGui::GetBackgroundDrawList();

    // 1. Render ESP Visualization Overlay (Bones, Boxes, Snaplines, HP, Radar)
    IosUI::RenderEspOverlay(background_draw_list, io.DisplaySize);

    // 2. Render Eclipse Modern HUD Pill (Title, Player/Bot Counts, FPS, Tap to Open)
    Menu::RenderHud("ECLIPSE VIP", &s_menu_open);

    // 3. Render Eclipse 5-Tab Main Menu (Visuals, Aim, Memory, Misc, Settings)
    Menu::Render(&s_menu_open);

    // Keep global settings in sync
    RyomenX::Settings::Get().menu_open = s_menu_open;

    // End ImGui Frame & Render
    ImGui::Render();

    id<MTLCommandBuffer> commandBuffer = [self.command_queue commandBuffer];
    id<MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDesc];

    ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

    [renderEncoder endEncoding];
    [commandBuffer presentDrawable:drawable];
    [commandBuffer commit];
}

@end
