cmake_minimum_required(VERSION 3.16)
project(RyomenX LANGUAGES C CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Automatically download Dear ImGui v1.90.4
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.90.4
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()

# Source files
set(SOURCES
    src/Main.mm
    src/RyomenXOverlayView.mm
    src/StealthBypass.mm
    src/Menu.cpp
    src/IosUI.cpp
    src/OverlayMath.cpp
    src/MachMemory.cpp
    src/MemoryCheats.cpp
    src/PythonBridge.cpp
    src/Config.cpp
    # ImGui core & Metal backend
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_metal.mm
)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Shared dylib target
add_library(RyomenX SHARED ${SOURCES})

# Target-specific compile options
if(APPLE)
    target_compile_options(RyomenX PRIVATE
        -fobjc-arc
        -Wall
        -Wextra
        -O2
    )

    target_link_libraries(RyomenX PRIVATE
        "-framework Metal"
        "-framework MetalKit"
        "-framework UIKit"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework Security"
    )

    set_target_properties(RyomenX PROPERTIES
        PREFIX ""
        OUTPUT_NAME "RyomenX"
        SUFFIX ".dylib"
        XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED "NO"
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED "NO"
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )
endif()
