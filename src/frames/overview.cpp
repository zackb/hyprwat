#define GL_GLEXT_PROTOTYPES 1
#include "overview.hpp"
#include <algorithm>
#include <imgui.h>
#include <iostream>
#include <sstream>
#include <unistd.h>

extern "C" {
// mock for undefined reference in hyprland-toplevel-export protocol
extern const struct wl_interface zwlr_foreign_toplevel_handle_v1_interface = {
    "zwlr_foreign_toplevel_handle_v1", 3, 0, nullptr, 0, nullptr};
}

#define GL_GLEXT_PROTOTYPES 1

const struct hyprland_toplevel_export_frame_v1_listener OverviewFrame::export_frame_listener = {
    .buffer = OverviewFrame::handle_buffer,
    .damage = OverviewFrame::handle_damage,
    .flags = OverviewFrame::handle_flags,
    .ready = OverviewFrame::handle_ready,
    .failed = OverviewFrame::handle_failed,
    .linux_dmabuf = OverviewFrame::handle_linux_dmabuf,
    .buffer_done = OverviewFrame::handle_buffer_done,
};

OverviewFrame::OverviewFrame(hyprland::Control& hyprctl, wl::Display& wlDisplay, int logicalWidth, int logicalHeight)
    : hyprctl(hyprctl), wlDisplay(wlDisplay), logicalWidth(logicalWidth), logicalHeight(logicalHeight) {
    captureClients();
}

OverviewFrame::~OverviewFrame() {
    for (auto& w : workspaces) {
        for (auto& c : w.clients) {
            if (c->frame)
                hyprland_toplevel_export_frame_v1_destroy(c->frame);
            if (c->texture) {
                glDeleteTextures(1, &c->texture);
            }
            if (c->buffer)
                wl_buffer_destroy(c->buffer);
            if (c->bo)
                gbm_bo_destroy(c->bo);
            if (c->fdToClose >= 0)
                close(c->fdToClose);
        }
    }
}

void OverviewFrame::captureClients() {
    auto allWorkspaces = hyprctl.getWorkspaces();
    auto allClients = hyprctl.getClients();

    // sort workspaces by id
    std::sort(allWorkspaces.begin(), allWorkspaces.end(), [](const auto& a, const auto& b) { return a.id < b.id; });

    int activeWsId = hyprctl.getActiveWorkspaceId();

    for (size_t i = 0; i < allWorkspaces.size(); ++i) {
        const auto& w = allWorkspaces[i];
        if (w.id == activeWsId) {
            selectedIndex = (int)i;
        }

        WorkspaceView wv;
        wv.workspace = w;
        for (const auto& c : allClients) {
            if (c.workspaceId == w.id && c.mapped && !c.hidden) {
                auto capture = std::make_shared<CapturedClient>();
                capture->owner = this;
                capture->client = c;
                wv.clients.push_back(capture);
            }
        }
        workspaces.push_back(wv);
    }
}

void OverviewFrame::requestCapture(std::shared_ptr<CapturedClient> c) {
    if (!wlDisplay.exportManager() || c->frame || c->failed || c->ready)
        return;

    try {
        unsigned long long addr;
        std::stringstream ss;
        ss << std::hex << c->client.address;
        ss >> addr;

        c->frame = hyprland_toplevel_export_manager_v1_capture_toplevel(wlDisplay.exportManager(), 0, (uint32_t)addr);

        if (c->frame) {
            hyprland_toplevel_export_frame_v1_add_listener(c->frame, &export_frame_listener, c.get());
        } else {
            c->failed = true;
        }
    } catch (...) {
        debug::log(ERR,
                   "Failed to capture client {} on workspace {}",
                   c->client.title.empty() ? std::to_string(c->client.workspaceId) : c->client.title,
                   c->client.workspaceId);
        c->failed = true;
    }
}

void OverviewFrame::handle_buffer(void* data,
                                  struct hyprland_toplevel_export_frame_v1* export_frame,
                                  uint32_t format,
                                  uint32_t width,
                                  uint32_t height,
                                  uint32_t stride) {
    auto* c = static_cast<CapturedClient*>(data);
    c->captureFormat = format;
    c->captureWidth = width;
    c->captureHeight = height;
    c->captureStride = stride;
}

void OverviewFrame::handle_damage(void* data,
                                  struct hyprland_toplevel_export_frame_v1* export_frame,
                                  uint32_t x,
                                  uint32_t y,
                                  uint32_t width,
                                  uint32_t height) {}
void OverviewFrame::handle_flags(void* data, struct hyprland_toplevel_export_frame_v1* export_frame, uint32_t flags) {}

void OverviewFrame::handle_ready(void* data,
                                 struct hyprland_toplevel_export_frame_v1* export_frame,
                                 uint32_t tv_sec_hi,
                                 uint32_t tv_sec_lo,
                                 uint32_t tv_nsec) {
    auto* c = static_cast<CapturedClient*>(data);
    c->ready = true;
}

void OverviewFrame::handle_failed(void* data, struct hyprland_toplevel_export_frame_v1* export_frame) {
    auto* c = static_cast<CapturedClient*>(data);
    c->failed = true;
}

void OverviewFrame::handle_linux_dmabuf(void* data,
                                        struct hyprland_toplevel_export_frame_v1* export_frame,
                                        uint32_t format,
                                        uint32_t width,
                                        uint32_t height) {
    auto* c = static_cast<CapturedClient*>(data);
    c->dmabufFormat = format;
    c->dmabufWidth = width;
    c->dmabufHeight = height;
}

void OverviewFrame::handle_buffer_done(void* data, struct hyprland_toplevel_export_frame_v1* export_frame) {
    auto* c = static_cast<CapturedClient*>(data);
    if (!c->owner)
        return;

    try {
        if (c->owner->wlDisplay.gbmDevice() && c->owner->wlDisplay.linuxDmabuf() && c->dmabufFormat != 0) {
            c->bo = gbm_bo_create(c->owner->wlDisplay.gbmDevice(),
                                  c->dmabufWidth,
                                  c->dmabufHeight,
                                  c->dmabufFormat,
                                  GBM_BO_USE_LINEAR | GBM_BO_USE_RENDERING);

            if (!c->bo) {
                c->failed = true;
                return;
            }

            int fd = gbm_bo_get_fd(c->bo);
            uint32_t stride = gbm_bo_get_stride(c->bo);
            uint32_t offset = gbm_bo_get_offset(c->bo, 0);
            uint64_t modifier = gbm_bo_get_modifier(c->bo);

            zwp_linux_buffer_params_v1* params = zwp_linux_dmabuf_v1_create_params(c->owner->wlDisplay.linuxDmabuf());
            zwp_linux_buffer_params_v1_add(params, fd, 0, offset, stride, modifier >> 32, modifier & 0xffffffff);

            c->buffer =
                zwp_linux_buffer_params_v1_create_immed(params, c->dmabufWidth, c->dmabufHeight, c->dmabufFormat, 0);
            zwp_linux_buffer_params_v1_destroy(params);

            c->fdToClose = fd;
            hyprland_toplevel_export_frame_v1_copy(export_frame, c->buffer, 1);
        } else {
            c->shmBuffer = std::make_unique<wl::ShmBuffer>(
                c->owner->wlDisplay.shm(), c->captureWidth, c->captureHeight, c->captureStride, c->captureFormat);
            hyprland_toplevel_export_frame_v1_copy(export_frame, c->shmBuffer->getBuffer(), 1);
        }
    } catch (...) {
        c->failed = true;
    }
}

void OverviewFrame::createTexture(CapturedClient& c) {
    if (c.texture != 0 || !c.ready)
        return;

    if (c.bo && c.buffer && c.fdToClose >= 0) {
        typedef void (*PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)(GLenum target, void* image);
        static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR_ptr = nullptr;
        static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR_ptr = nullptr;
        static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES_ptr = nullptr;

        if (!eglCreateImageKHR_ptr) {
            eglCreateImageKHR_ptr = (PFNEGLCREATEIMAGEKHRPROC)eglGetProcAddress("eglCreateImageKHR");
            eglDestroyImageKHR_ptr = (PFNEGLDESTROYIMAGEKHRPROC)eglGetProcAddress("eglDestroyImageKHR");
            glEGLImageTargetTexture2DOES_ptr =
                (PFNGLEGLIMAGETARGETTEXTURE2DOESPROC)eglGetProcAddress("glEGLImageTargetTexture2DOES");
        }

        EGLDisplay display = eglGetDisplay((EGLNativeDisplayType)wlDisplay.display());
        uint64_t modifier = gbm_bo_get_modifier(c.bo);
        EGLint attribs[] = {EGL_WIDTH,
                            c.dmabufWidth,
                            EGL_HEIGHT,
                            c.dmabufHeight,
                            EGL_LINUX_DRM_FOURCC_EXT,
                            c.dmabufFormat,
                            EGL_DMA_BUF_PLANE0_FD_EXT,
                            c.fdToClose,
                            EGL_DMA_BUF_PLANE0_OFFSET_EXT,
                            (EGLint)gbm_bo_get_offset(c.bo, 0),
                            EGL_DMA_BUF_PLANE0_PITCH_EXT,
                            (EGLint)gbm_bo_get_stride(c.bo),
                            EGL_DMA_BUF_PLANE0_MODIFIER_LO_EXT,
                            (EGLint)(modifier & 0xFFFFFFFF),
                            EGL_DMA_BUF_PLANE0_MODIFIER_HI_EXT,
                            (EGLint)(modifier >> 32),
                            EGL_NONE};

        EGLImageKHR image =
            eglCreateImageKHR_ptr(display, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, (EGLClientBuffer) nullptr, attribs);

        glGenTextures(1, &c.texture);
        glBindTexture(GL_TEXTURE_2D, c.texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glEGLImageTargetTexture2DOES_ptr(GL_TEXTURE_2D, image);

        close(c.fdToClose);
        c.fdToClose = -1;
        eglDestroyImageKHR_ptr(display, image);

    } else if (c.shmBuffer) {
        glGenTextures(1, &c.texture);
        glBindTexture(GL_TEXTURE_2D, c.texture);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // assuming format is WL_SHM_FORMAT_XRGB8888 or ARGB8888 which is BGRA in memory (little endian)
        glTexImage2D(GL_TEXTURE_2D,
                     0,
                     GL_RGBA,
                     c.captureWidth,
                     c.captureHeight,
                     0,
                     GL_BGRA_EXT,
                     GL_UNSIGNED_BYTE,
                     c.shmBuffer->getData());

        c.shmBuffer.reset();
    }
}

FrameResult OverviewFrame::render() {
    // request captures for visible workspaces (expanded buffer window)
    int startIdx = std::max(0, selectedIndex - 4);
    int endIdx = std::min((int)workspaces.size() - 1, selectedIndex + 4);
    for (int i = startIdx; i <= endIdx; ++i) {
        if (i >= 0 && i < workspaces.size()) {
            for (auto& c : workspaces[i].clients) {
                if (!c->frame && !c->failed && !c->ready) {
                    requestCapture(c);
                }
            }
        }
    }

    // process generated textures time-sliced
    int texturesCreatedThisFrame = 0;
    const int MAX_TEXTURES_PER_FRAME = 2;

    for (auto& w : workspaces) {
        for (auto& c : w.clients) {
            if (c->ready && c->texture == 0) {
                if (texturesCreatedThisFrame < MAX_TEXTURES_PER_FRAME) {
                    createTexture(*c);
                    texturesCreatedThisFrame++;
                }
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_H) ||
        (ImGui::IsKeyPressed(ImGuiKey_Tab) && ImGui::GetIO().KeyShift))
        navigate(-1);
    else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_L) ||
             (ImGui::IsKeyPressed(ImGuiKey_Tab) && !ImGui::GetIO().KeyShift))
        navigate(1);
    if (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_Space)) {
        if (!workspaces.empty() && selectedIndex >= 0 && selectedIndex < workspaces.size()) {
            return FrameResult::Submit(std::to_string(workspaces[selectedIndex].workspace.id));
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        return FrameResult::Cancel();
    }

    Vec2 size = getSize();
    float edge_padding = 20.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(edge_padding, edge_padding));
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);

    ImGui::Begin("Overview",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    float wsWidth = logicalWidth * scaleRatio;
    float wsHeight = logicalHeight * scaleRatio;
    float spacing = 20.0f;
    float totalWidthPerWs = wsWidth + spacing;

    float targetScroll = selectedIndex * totalWidthPerWs - (contentRegion.x - wsWidth) * 0.5f;
    if (targetScroll < 0)
        targetScroll = 0;
    scrollOffset += (targetScroll - scrollOffset) * 0.15f;

    if (!workspaces.empty()) {
        ImGui::BeginChild("ScrollRegion",
                          ImVec2(0, contentRegion.y),
                          false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        ImGui::SetScrollX(scrollOffset);

        for (size_t i = 0; i < workspaces.size(); ++i) {
            if (i > 0)
                ImGui::SameLine();

            ImGui::BeginGroup();

            ImVec2 wsMin = ImGui::GetCursorScreenPos();
            ImVec2 wsMax = ImVec2(wsMin.x + wsWidth, wsMin.y + wsHeight);

            bool is_selected = (i == selectedIndex);

            // highlight active or selected
            ImU32 bgColor = is_selected ? ImGui::GetColorU32(hoverColor) : ImGui::GetColorU32(workspaceColor);
            ImGui::GetWindowDrawList()->AddRectFilled(wsMin, wsMax, bgColor, workspaceRounding);

            if (is_selected) {
                ImGui::GetForegroundDrawList()->AddRect(
                    wsMin, wsMax, IM_COL32(255, 255, 255, 200), workspaceRounding, 0, 2.0f);
            } else {
                ImGui::GetWindowDrawList()->AddRect(
                    wsMin, wsMax, IM_COL32(100, 100, 100, 150), workspaceRounding, 0, 1.0f);
            }

            for (const auto& c : workspaces[i].clients) {
                if (c->texture == 0)
                    continue;

                // calc scaled bounds
                float cx = wsMin.x + c->client.x * scaleRatio;
                float cy = wsMin.y + c->client.y * scaleRatio;
                float cw = c->client.width * scaleRatio;
                float ch = c->client.height * scaleRatio;

                // draw client texture
                ImGui::GetWindowDrawList()->AddImageRounded((void*)(intptr_t)c->texture,
                                                            ImVec2(cx, cy),
                                                            ImVec2(cx + cw, cy + ch),
                                                            ImVec2(0, 0),
                                                            ImVec2(1, 1),
                                                            IM_COL32_WHITE,
                                                            clientRounding);
                ImGui::GetWindowDrawList()->AddRect(
                    ImVec2(cx, cy), ImVec2(cx + cw, cy + ch), IM_COL32(50, 50, 50, 150), clientRounding, 0, 1.0f);
            }

            ImGui::Dummy(ImVec2(wsWidth, wsHeight));
            ImGui::EndGroup();

            if (i < workspaces.size() - 1) {
                ImGui::SameLine(0.0f, spacing);
            }
        }
        ImGui::EndChild();
    }

    ImGui::End();
    ImGui::PopStyleVar();

    return FrameResult::Continue();
}

Vec2 OverviewFrame::getSize() {
    float wsHeight = logicalHeight * scaleRatio;
    float wsWidth = logicalWidth * scaleRatio;
    float spacing = 20.0f;

    float requiredWidth = workspaces.size() * wsWidth + (workspaces.empty() ? 0 : (workspaces.size() - 1) * spacing);
    float maxW = (float)logicalWidth * widthRatio;

    float w = std::min(requiredWidth, maxW);
    if (w < wsWidth)
        w = wsWidth; // At least one workspace width if empty

    float edgePadding = 20.0f;
    return Vec2{w + (edgePadding * 2), wsHeight + (edgePadding * 2)};
}

void OverviewFrame::navigate(int direction) {
    if (workspaces.empty())
        return;

    selectedIndex += direction;
    if (selectedIndex < 0)
        selectedIndex = 0;
    if (selectedIndex >= workspaces.size())
        selectedIndex = workspaces.size() - 1;
}

void OverviewFrame::applyTheme(const Config& config) {
    hoverColor = config.getColor("theme", "hover_color", "#3366B366");
    workspaceColor = config.getColor("theme", "workspace_color", "#1A1A1CCC");
    workspaceRounding = config.getFloat("theme", "frame_rounding", 12.0f);
    widthRatio = config.getFloat("theme", "wallpaper_width_ratio", 0.8f);
}
