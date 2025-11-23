#include "input.hpp"

FrameResult TextInput::render() {
    // Calculate desired size based on content
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 framePadding = style.FramePadding;
    ImVec2 windowPadding = style.WindowPadding;

    // Calculate input box size
    float inputWidth = 300.0f; // default width for input

    // calculate actual size after rendering
    lastSize = ImVec2(inputWidth + windowPadding.x * 2, 50);

    // Set the window to fill the entire display
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("Select",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    // Set the input text width
    ImGui::SetNextItemWidth(inputWidth);

    // if (ImGui::IsWindowAppearing())
    if (shouldFocus) {
        ImGui::SetKeyboardFocusHere();
        shouldFocus = false;
    }

    ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
    if (password) {
        flags |= ImGuiInputTextFlags_Password;
    }
    bool enterPressed = ImGui::InputTextWithHint("##pass", hint.c_str(), inputBuffer, bufSize, flags);

    bool escPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

    // actual content size after rendering
    ImVec2 contentSize = ImGui::GetItemRectSize();
    lastSize = ImVec2(inputWidth + windowPadding.x * 2, contentSize.y + windowPadding.y * 2);

    ImGui::End();

    if (enterPressed) {
        return FrameResult::Submit(std::string(inputBuffer));
    }

    if (escPressed) {
        return FrameResult::Cancel();
    }

    return FrameResult::Continue();
}

Vec2 TextInput::getSize() { return Vec2{lastSize.x, lastSize.y}; }
