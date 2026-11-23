TARGET = iphone:clang:latest:14.0
ARCHS = arm64

include $(THEOS)/makefiles/common.mk

LIBRARY_NAME = RyomenX

RyomenX_FILES = \
    src/Main.mm \
    src/RyomenXOverlayView.mm \
    src/StealthBypass.mm \
    src/Menu.cpp \
    src/IosUI.cpp \
    src/OverlayMath.cpp \
    src/MachMemory.cpp \
    src/MemoryCheats.cpp \
    src/PythonBridge.cpp \
    src/Config.cpp \
    imgui/imgui.cpp \
    imgui/imgui_draw.cpp \
    imgui/imgui_tables.cpp \
    imgui/imgui_widgets.cpp \
    imgui/backends/imgui_impl_metal.mm

RyomenX_FRAMEWORKS = UIKit Metal MetalKit Foundation QuartzCore Security
RyomenX_CFLAGS = -fobjc-arc -Iinclude -Iimgui -Iimgui/backends -O2
RyomenX_CXXFLAGS = -std=c++17 -fobjc-arc -Iinclude -Iimgui -Iimgui/backends -O2

include $(THEOS_MAKE_PATH)/library.mk
