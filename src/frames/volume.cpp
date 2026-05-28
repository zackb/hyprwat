#include "volume.hpp"
#include <algorithm>

#ifndef IM_PI
#define IM_PI 3.14159265358979323846f
#endif

VolumeFrame::VolumeFrame(AudioManagerClient& audioManager, VolumeAction initialAction)
    : audioManager(audioManager), initialAction(initialAction) {
    lastUpdateTime = std::chrono::steady_clock::now();
}

void VolumeFrame::applyTheme(const Config& config) { baseAlpha = config.getFloat("theme", "background_blur", 0.95f); }

void VolumeFrame::adjustVolume(float delta) {
    std::lock_guard<std::mutex> lock(frameMutex);
    auto info = audioManager.getVolume();
    float new_vol = info.volume + delta;
    if (new_vol < 0.0f)
        new_vol = 0.0f;
    if (new_vol > 1.0f)
        new_vol = 1.0f;
    audioManager.setVolume(new_vol);
    lastUpdateTime = std::chrono::steady_clock::now();
}

void VolumeFrame::toggleMute() {
    std::lock_guard<std::mutex> lock(frameMutex);
    auto info = audioManager.getVolume();
    audioManager.setMute(!info.mute);
    lastUpdateTime = std::chrono::steady_clock::now();
}

static void DrawSpeakerIcon(ImDrawList* draw_list, ImVec2 pos, float size, float volume, bool mute) {
    ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);

    // Draw speaker body (rectangle + trapezoid)
    float box_w = size * 0.3f;
    float box_h = size * 0.35f;
    ImVec2 box_p0 = ImVec2(pos.x, pos.y + (size - box_h) * 0.5f);
    ImVec2 box_p1 = ImVec2(box_p0.x + box_w, box_p0.y + box_h);
    draw_list->AddRectFilled(box_p0, box_p1, color);

    // Draw cone
    ImVec2 cone_p0 = ImVec2(box_p1.x, box_p0.y);
    ImVec2 cone_p1 = ImVec2(box_p1.x, box_p1.y);
    ImVec2 cone_p2 = ImVec2(pos.x + size * 0.55f, pos.y + size * 0.85f);
    ImVec2 cone_p3 = ImVec2(pos.x + size * 0.55f, pos.y + size * 0.15f);

    draw_list->AddTriangleFilled(cone_p0, cone_p1, cone_p2, color);
    draw_list->AddTriangleFilled(cone_p0, cone_p2, cone_p3, color);

    if (mute || volume < 0.01f) {
        ImU32 mute_color = ImGui::GetColorU32(ImVec4(0.9f, 0.3f, 0.3f, 1.0f));
        draw_list->AddLine(ImVec2(pos.x + size * 0.65f, pos.y + size * 0.3f),
                           ImVec2(pos.x + size * 0.85f, pos.y + size * 0.7f),
                           mute_color,
                           2.0f);
        draw_list->AddLine(ImVec2(pos.x + size * 0.85f, pos.y + size * 0.3f),
                           ImVec2(pos.x + size * 0.65f, pos.y + size * 0.7f),
                           mute_color,
                           2.0f);
    } else {
        float center_x = pos.x + size * 0.3f;
        float center_y = pos.y + size * 0.5f;

        // low wave
        if (volume > 0.0f) {
            draw_list->PathArcTo(ImVec2(center_x, center_y), size * 0.35f, -IM_PI / 4, IM_PI / 4);
            draw_list->PathStroke(color, 0, 2.0f);
        }
        // medium wave
        if (volume > 0.33f) {
            draw_list->PathArcTo(ImVec2(center_x, center_y), size * 0.5f, -IM_PI / 4, IM_PI / 4);
            draw_list->PathStroke(color, 0, 2.0f);
        }
        // high wave
        if (volume > 0.66f) {
            draw_list->PathArcTo(ImVec2(center_x, center_y), size * 0.65f, -IM_PI / 4, IM_PI / 4);
            draw_list->PathStroke(color, 0, 2.0f);
        }
    }
}

FrameResult VolumeFrame::render() {
    // 1. Handle initial volume adjustment once
    if (!initialAdjustDone) {
        if (initialAction == VolumeAction::UP) {
            adjustVolume(0.05f);
        } else if (initialAction == VolumeAction::DOWN) {
            adjustVolume(-0.05f);
        }
        initialAdjustDone = true;
    }

    // 2. Read current volume state
    auto info = audioManager.getVolume();

    // 3. Handle Keyboard Inputs
    bool upPressed = ImGui::IsKeyPressed(ImGuiKey_F13) || ImGui::IsKeyPressed(ImGuiKey_UpArrow) ||
                     ImGui::IsKeyPressed(ImGuiKey_RightArrow) || ImGui::IsKeyPressed(ImGuiKey_Equal) ||
                     ImGui::IsKeyPressed(ImGuiKey_KeypadAdd);

    bool downPressed = ImGui::IsKeyPressed(ImGuiKey_F14) || ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
                       ImGui::IsKeyPressed(ImGuiKey_LeftArrow) || ImGui::IsKeyPressed(ImGuiKey_Minus) ||
                       ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract);

    bool mutePressed = ImGui::IsKeyPressed(ImGuiKey_F15) || ImGui::IsKeyPressed(ImGuiKey_M);

    if (upPressed) {
        adjustVolume(0.05f);
        info = audioManager.getVolume();
    } else if (downPressed) {
        adjustVolume(-0.05f);
        info = audioManager.getVolume();
    } else if (mutePressed) {
        toggleMute();
        info = audioManager.getVolume();
    }

    // 4. Handle time-based close and fading
    auto now = std::chrono::steady_clock::now();
    long long elapsed;
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();
    }

    if (elapsed > 1500) {
        return FrameResult::Submit("");
    }

    float alpha = 1.0f;
    if (elapsed > 1200) {
        alpha = 1.0f - (elapsed - 1200) / 300.0f;
        alpha = std::max(0.0f, alpha);
    }

    ImGuiStyle& style = ImGui::GetStyle();
    style.Alpha = alpha * baseAlpha;

    // 5. Draw UI
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Volume OSD",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 win_pos = ImGui::GetWindowPos();
    ImVec2 win_size = ImGui::GetWindowSize();

    // Draw layout
    float speaker_size = 20.0f;
    ImVec2 speaker_pos = ImVec2(win_pos.x + 16.0f, win_pos.y + 14.0f);
    DrawSpeakerIcon(draw_list, speaker_pos, speaker_size, info.volume, info.mute);

    // Draw Percentage Text
    std::string pct_text = info.mute ? "Muted" : (std::to_string((int)(info.volume * 100)) + "%");
    ImVec2 pct_text_size = ImGui::CalcTextSize(pct_text.c_str());
    ImGui::SetCursorScreenPos(
        ImVec2(win_pos.x + win_size.x - 16.0f - pct_text_size.x, win_pos.y + (win_size.y - pct_text_size.y) * 0.5f));
    ImGui::Text("%s", pct_text.c_str());

    // Draw 10 volume blocks/rects
    float rect_width = 12.0f;
    float rect_height = 12.0f;
    float spacing = 4.0f;
    ImVec2 bar_start = ImVec2(speaker_pos.x + speaker_size + 12.0f, win_pos.y + 18.0f);

    ImU32 border_col = ImGui::GetColorU32(ImGuiCol_Border);
    ImU32 active_col = ImGui::GetColorU32(ImGuiCol_Border); // Let's use the theme border color as filled color

    // Fallback if border color has zero alpha
    ImVec4 border_v4 = ImGui::GetStyleColorVec4(ImGuiCol_Border);
    if (border_v4.w == 0.0f) {
        border_col = ImGui::GetColorU32(ImVec4(0.2f, 0.6f, 1.0f, 1.0f));
        active_col = border_col;
    }

    ImU32 bg_col = ImGui::GetColorU32(ImGuiCol_FrameBg);

    for (int i = 0; i < 10; i++) {
        ImVec2 p0 = ImVec2(bar_start.x + i * (rect_width + spacing), bar_start.y);
        ImVec2 p1 = ImVec2(p0.x + rect_width, p0.y + rect_height);

        float threshold_start = i * 0.1f;
        float threshold_end = (i + 1) * 0.1f;
        float threshold_mid = threshold_start + 0.05f;

        bool fully_filled = !info.mute && (info.volume >= threshold_end - 0.005f);
        bool half_filled = !info.mute && !fully_filled && (info.volume >= threshold_mid - 0.005f);

        // draw background block (empty state)
        draw_list->AddRectFilled(p0, p1, bg_col, style.FrameRounding);

        // draw active color fill
        if (fully_filled) {
            draw_list->AddRectFilled(p0, p1, active_col, style.FrameRounding);
        } else if (half_filled) {
            ImVec2 clip_p0 = p0;
            ImVec2 clip_p1 = ImVec2(p0.x + rect_width * 0.5f, p1.y);
            draw_list->PushClipRect(clip_p0, clip_p1, true);
            draw_list->AddRectFilled(p0, p1, active_col, style.FrameRounding);
            draw_list->PopClipRect();
        }

        // draw block border
        draw_list->AddRect(p0, p1, border_col, style.FrameRounding, 0, 1.0f);
    }

    ImGui::End();

    bool escPressed = ImGui::IsKeyPressed(ImGuiKey_Escape) || ImGui::IsKeyPressed(ImGuiKey_Enter);
    if (escPressed) {
        return FrameResult::Submit("");
    }

    return FrameResult::Continue();
}
