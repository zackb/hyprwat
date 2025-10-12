cmake_minimum_required(VERSION 3.15)
project(hyprwat CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)

# Find Wayland
find_package(Wayland QUIET COMPONENTS Client Egl)
if(NOT Wayland_FOUND)
    # Fallback to pkg-config if Wayland CMake package is not found
    pkg_check_modules(WAYLAND REQUIRED IMPORTED_TARGET wayland-client wayland-egl)
endif()

find_package(OpenGL REQUIRED COMPONENTS OpenGL EGL)

# Fontconfig
pkg_check_modules(Fontconfig REQUIRED fontconfig)

# xkbcommon
pkg_check_modules(XKBCOMMON REQUIRED IMPORTED_TARGET xkbcommon)

# sdbus
pkg_check_modules(SDBUSCPP REQUIRED sdbus-c++)

# pipewire
pkg_check_modules(PIPEWIRE REQUIRED libpipewire-0.3)


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

# INIH
include(FetchContent)
FetchContent_Declare(
  inih
  GIT_REPOSITORY https://github.com/benhoyt/inih.git
  GIT_TAG r58
)
FetchContent_MakeAvailable(inih)

add_library(inih STATIC
    ${inih_SOURCE_DIR}/ini.c
    ${inih_SOURCE_DIR}/cpp/INIReader.cpp
)
target_include_directories(inih PUBLIC
    ${XKBCOMMON_INCLUDE_DIRS}
    ${inih_SOURCE_DIR}
    ${PIPEWIRE_INCLUDE_DIRS}
    ${SDBUSCPP_INCLUDE_DIRS}
    ${inih_SOURCE_DIR}/cpp
)

# Main executable
add_executable(hyprwat
    src/main.cpp
    src/input.cpp
    src/ui.cpp
    src/util.cpp
    src/hyprland/ipc.cpp
    src/wayland/wayland.cpp
    src/wayland/display.cpp
    src/wayland/layer_surface.cpp
    src/wayland/input.cpp
    src/renderer/egl_context.cpp
    src/font/font.cpp
    src/frames/selector.cpp
    src/frames/input.cpp
    src/frames/text.cpp
    src/flows/simple_flows.cpp
    src/flows/wifi_flow.cpp
    src/flows/audio_flow.cpp
    src/net/network_manager.cpp
    src/audio/audio.cpp
    ${IMGUI_SOURCES}
    ${WAYLAND_PROTOCOLS}
)

target_compile_options(hyprwat PRIVATE
    ${PIPEWIRE_CFLAGS_OTHER}
    ${SDBUS_CPP_CFLAGS_OTHER}
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
    PkgConfig::XKBCOMMON
    ${PIPEWIRE_LIBRARIES}
    ${SDBUSCPP_LIBRARIES}
    inih
    ${CMAKE_DL_LIBS}
    m
    pthread
)

install(TARGETS hyprwat
        RUNTIME DESTINATION bin)

install(FILES man/hyprwat.6
        DESTINATION share/man/man6)

# generate .deb .rpm and .tgz
include(InstallRequiredSystemLibraries)

set(CPACK_PACKAGE_NAME "hyprwat")
set(CPACK_PACKAGE_VERSION "0.6.0")
set(CPACK_PACKAGE_CONTACT "zack@bartel.com")
set(CPACK_GENERATOR "DEB;RPM;TGZ")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Zack Bartel")
set(CPACK_RPM_PACKAGE_RELEASE 1)

# deb and rpm dependencies
set(CPACK_DEBIAN_PACKAGE_DEPENDS
    "libwayland-client0, wayland-protocols, libegl1-mesa, libgl1-mesa-glx, libfontconfig1, libxkbcommon0, libpipewire-0.3-0, libsdbus-c++-1")
set(CPACK_RPM_PACKAGE_REQUIRES
    "wayland-libs, wayland-protocols, mesa-libEGL, mesa-libGL, fontconfig, libxkbcommon, pipewire, sdbus-c++")


# remove the "-Linux" suffix
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")

include(CPack)


