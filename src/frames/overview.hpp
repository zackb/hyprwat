#pragma once

#include "../hyprland/ipc.hpp"
#include "../ui.hpp"
#include "../wayland/display.hpp"
#include "../wayland/protocols/hyprland-toplevel-export-v1-client-protocol.h"
#include "../wayland/shm.hpp"
#include <GL/gl.h>
#include <memory>
#include <mutex>
#include <vector>

class OverviewFrame : public Frame {
public:
    OverviewFrame(hyprland::Control& hyprctl, wl::Display& wlDisplay, int logicalWidth, int logicalHeight);
    ~OverviewFrame() override;

    FrameResult render() override;
    Vec2 getSize() override;
    void applyTheme(const Config& config) override;
    bool shouldRepositionOnResize() const override { return false; }
    bool shouldPositionAtCursor() const override { return false; }

private:
    struct CapturedClient {
        OverviewFrame* owner = nullptr;
        hyprland::Client client;
        struct hyprland_toplevel_export_frame_v1* frame = nullptr;
        std::unique_ptr<wl::ShmBuffer> shmBuffer;
        GLuint texture = 0;
        bool ready = false;
        bool failed = false;
        int captureFormat = 0;
        int captureWidth = 0;
        int captureHeight = 0;
        int captureStride = 0;
    };

    struct WorkspaceView {
        hyprland::Workspace workspace;
        std::vector<std::shared_ptr<CapturedClient>> clients;
    };

    int selectedIndex = 0;
    int logicalWidth;
    int logicalHeight;
    hyprland::Control& hyprctl;
    wl::Display& wlDisplay;

    std::vector<WorkspaceView> workspaces;
    std::vector<std::shared_ptr<CapturedClient>> pendingCaptures;
    std::mutex captureMutex;

    float padding = 20.0f;
    float workspaceRounding = 12.0f;
    float clientRounding = 6.0f;
    float scaleRatio = 0.2f; // Workspace thumbnail size scaled down relative to monitor
    float widthRatio = 0.8f;
    ImVec4 hoverColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    ImVec4 workspaceColor = ImVec4(0.1f, 0.1f, 0.15f, 0.8f);
    float scrollOffset = 0.0f;

    void captureClients();
    void navigate(int direction);
    void createTexture(CapturedClient& c);

    static void handle_buffer(void* data,
                              struct hyprland_toplevel_export_frame_v1* export_frame,
                              uint32_t format,
                              uint32_t width,
                              uint32_t height,
                              uint32_t stride);
    static void handle_damage(void* data,
                              struct hyprland_toplevel_export_frame_v1* export_frame,
                              uint32_t x,
                              uint32_t y,
                              uint32_t width,
                              uint32_t height);
    static void handle_flags(void* data, struct hyprland_toplevel_export_frame_v1* export_frame, uint32_t flags);
    static void handle_ready(void* data,
                             struct hyprland_toplevel_export_frame_v1* export_frame,
                             uint32_t tv_sec_hi,
                             uint32_t tv_sec_lo,
                             uint32_t tv_nsec);
    static void handle_failed(void* data, struct hyprland_toplevel_export_frame_v1* export_frame);
    static void handle_linux_dmabuf(void* data,
                                    struct hyprland_toplevel_export_frame_v1* export_frame,
                                    uint32_t format,
                                    uint32_t width,
                                    uint32_t height);
    static void handle_buffer_done(void* data, struct hyprland_toplevel_export_frame_v1* export_frame);

    static const struct hyprland_toplevel_export_frame_v1_listener export_frame_listener;
};
