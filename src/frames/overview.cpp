#include "overview.hpp"
#include <algorithm>
#include <imgui.h>
#include <iostream>
#include <sstream>

extern "C" {
// mock for undefined reference in hyprland-toplevel-export protocol
extern const struct wl_interface zwlr_foreign_toplevel_handle_v1_interface = {
    "zwlr_foreign_toplevel_handle_v1", 3, 0, nullptr, 0, nullptr};
}

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
        }
    }
}

void OverviewFrame::captureClients() {
    auto allWorkspaces = hyprctl.getWorkspaces();
    auto allClients = hyprctl.getClients();

    // Sort workspaces by id
    std::sort(allWorkspaces.begin(), allWorkspaces.end(), [](const auto& a, const auto& b) { return a.id < b.id; });

    for (const auto& w : allWorkspaces) {
        WorkspaceView wv;
        wv.workspace = w;
        for (const auto& c : allClients) {
            if (c.workspaceId == w.id && c.mapped && !c.hidden) {
                auto capture = std::make_shared<CapturedClient>();
                capture->owner = this;
                capture->client = c;
                wv.clients.push_back(capture);
                // Also add to global capture list
                pendingCaptures.push_back(capture);
            }
        }
        workspaces.push_back(wv);
    }

    if (!wlDisplay.exportManager()) {
        std::cerr << "Toplevel export manager not available" << std::endl;
        return;
    }

    for (auto& c : pendingCaptures) {
        try {
            unsigned long long addr;
            std::stringstream ss;
            ss << std::hex << c->client.address;
            ss >> addr;

            c->frame =
                hyprland_toplevel_export_manager_v1_capture_toplevel(wlDisplay.exportManager(), 0, (uint32_t)addr);

            if (c->frame) {
                hyprland_toplevel_export_frame_v1_add_listener(c->frame, &export_frame_listener, c.get());
            }
        } catch (...) {
        }
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
                                        uint32_t height) {}

void OverviewFrame::handle_buffer_done(void* data, struct hyprland_toplevel_export_frame_v1* export_frame) {
    auto* c = static_cast<CapturedClient*>(data);
    if (!c->owner)
        return;

    try {
        c->shmBuffer = std::make_unique<wl::ShmBuffer>(
            c->owner->wlDisplay.shm(), c->captureWidth, c->captureHeight, c->captureStride, c->captureFormat);
        hyprland_toplevel_export_frame_v1_copy(export_frame, c->shmBuffer->getBuffer(), 1);
    } catch (...) {
        c->failed = true;
    }
}

void OverviewFrame::createTexture(CapturedClient& c) {
    if (c.texture != 0 || !c.ready || !c.shmBuffer)
        return;

    // ARGB to RGBA if necessary. SHM format is typically ARGB8888 or XRGB8888.
    // OpenGL expects RGBA, so we might need to swap channels.
    glGenTextures(1, &c.texture);
    glBindTexture(GL_TEXTURE_2D, c.texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // assuming format is WL_SHM_FORMAT_XRGB8888 or ARGB8888 which is BGRA in memory (little endian)
    // upload using GL_BGRA_EXT or we manual swizzle.
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA,
                 c.captureWidth,
                 c.captureHeight,
                 0,
                 GL_BGRA_EXT,
                 GL_UNSIGNED_BYTE,
                 c.shmBuffer->getData());

    // clean up shmBuffer as it's no longer needed in CPU RAM
    c.shmBuffer.reset();
}

FrameResult OverviewFrame::render() {
    // process generated textures
    for (auto& w : workspaces) {
        for (auto& c : w.clients) {
            if (c->ready && c->texture == 0) {
                createTexture(*c);
            }
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_H))
        navigate(-1);
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_L))
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
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(size.x, size.y), ImGuiCond_Always);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0)); // transparent
    ImGui::Begin("Overview",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();

    float wsWidth = logicalWidth * scaleRatio;
    float wsHeight = logicalHeight * scaleRatio;

    for (size_t i = 0; i < workspaces.size(); ++i) {
        ImVec2 wsMin = ImVec2(p.x + padding + i * (wsWidth + padding), p.y + padding);
        ImVec2 wsMax = ImVec2(wsMin.x + wsWidth, wsMin.y + wsHeight);

        // highlight active or selected
        if (i == selectedIndex) {
            drawList->AddRectFilled(wsMin, wsMax, ImGui::GetColorU32(hoverColor), workspaceRounding);
            drawList->AddRect(wsMin, wsMax, IM_COL32(255, 255, 255, 200), workspaceRounding, 0, 2.0f);
        } else {
            drawList->AddRectFilled(wsMin, wsMax, ImGui::GetColorU32(workspaceColor), workspaceRounding);
            drawList->AddRect(wsMin, wsMax, IM_COL32(100, 100, 100, 150), workspaceRounding, 0, 1.0f);
        }

        // add workspace name text
        ImVec2 textSize = ImGui::CalcTextSize(workspaces[i].workspace.name.c_str());
        drawList->AddText(ImVec2(wsMin.x + (wsWidth - textSize.x) / 2, wsMax.y + 5),
                          IM_COL32(255, 255, 255, 255),
                          workspaces[i].workspace.name.c_str());

        for (const auto& c : workspaces[i].clients) {
            if (c->texture == 0)
                continue;

            // Calc scaled bounds
            float cx = wsMin.x + c->client.x * scaleRatio;
            float cy = wsMin.y + c->client.y * scaleRatio;
            float cw = c->client.width * scaleRatio;
            float ch = c->client.height * scaleRatio;

            // Draw client texture
            drawList->AddImageRounded((void*)(intptr_t)c->texture,
                                      ImVec2(cx, cy),
                                      ImVec2(cx + cw, cy + ch),
                                      ImVec2(0, 0),
                                      ImVec2(1, 1),
                                      IM_COL32_WHITE,
                                      clientRounding);
            drawList->AddRect(
                ImVec2(cx, cy), ImVec2(cx + cw, cy + ch), IM_COL32(50, 50, 50, 150), clientRounding, 0, 1.0f);
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();

    return FrameResult::Continue();
}

Vec2 OverviewFrame::getSize() {
    float wsWidth = logicalWidth * scaleRatio;
    float wsHeight = logicalHeight * scaleRatio;
    float totalWidth = padding + (workspaces.size() * (wsWidth + padding));
    float totalHeight = padding + wsHeight + padding + 30.0f; // extra 30 for text labels
    return Vec2{totalWidth, totalHeight};
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
}
