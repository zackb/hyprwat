cmake_minimum_required(VERSION 3.15)
project(hyprwat CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Wayland QUIET COMPONENTS Client Egl)
if(NOT Wayland_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client wayland-egl)
endif()

find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)

# Fontconfig
find_package(PkgConfig REQUIRED)
pkg_check_modules(Fontconfig REQUIRED fontconfig)
link_directories(${Fontconfig_LIBRARY_DIRS})


# ImGui sources
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

# Wayland protocol sources
set(WAYLAND_PROTOCOLS
    src/wayland/protocols/wlr-layer-shell-unstable-v1-client-protocol.c
    src/wayland/protocols/xdg-shell-client-protocol.c
)

# Main executable
add_executable(hyprwat
    src/main.cpp
    src/hyprland/ipc.cpp
    src/wayland/wayland.cpp
    src/wayland/display.cpp
    src/wayland/layer_surface.cpp
    src/wayland/input.cpp
    src/renderer/egl_context.cpp
    src/font/font.cpp
    src/input.cpp
    src/selection/selector.cpp
    src/input.cpp
    src/ui.cpp
    src/menu/popup_menu.cpp
    ${IMGUI_SOURCES}
    ${WAYLAND_PROTOCOLS}
)

# Include directories
target_include_directories(hyprwat PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

if(Wayland_FOUND)
    target_link_libraries(hyprwat PRIVATE
        Wayland::Client
        Wayland::Egl
    )
else()
    target_link_libraries(hyprwat PRIVATE
        PkgConfig::WAYLAND
    )
endif()

target_link_libraries(hyprwat PRIVATE
    OpenGL::OpenGL
    OpenGL::EGL
    ${Fontconfig_LIBRARIES}
    ${CMAKE_DL_LIBS}
    m
    pthread
)

# Install the executable
install(TARGETS hyprwat
        RUNTIME DESTINATION bin)


