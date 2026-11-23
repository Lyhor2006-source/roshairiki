cmake_minimum_required(VERSION 3.16)
project(eclipse_menu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Path to your Dear ImGui checkout (folder containing imgui.h).
set(IMGUI_DIR "" CACHE PATH "Path to Dear ImGui sources")
if(NOT IMGUI_DIR)
    message(FATAL_ERROR "Set -DIMGUI_DIR=/path/to/imgui")
endif()

add_library(eclipse_menu STATIC
    src/Menu.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(eclipse_menu PUBLIC src ${IMGUI_DIR})

# Link this into your project, which provides the imgui_impl_metal backend.
