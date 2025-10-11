#include "text.hpp"

FrameResult Text::render() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 windowPadding = style.WindowPadding;

    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("Status",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    ImGui::TextColored(color, "%s", text.c_str());

    ImVec2 contentSize = ImGui::GetItemRectSize();
    lastSize = ImVec2(contentSize.x + windowPadding.x * 2, contentSize.y + windowPadding.y * 2);

    bool escPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

    ImGui::End();

    if (escPressed || _done) {
        return FrameResult::Cancel();
    }

    return FrameResult::Continue();
}
