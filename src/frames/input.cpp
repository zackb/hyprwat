#include "input.hpp"

bool TextInput::render() {
    // Calculate desired size based on content
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 framePadding = style.FramePadding;
    ImVec2 windowPadding = style.WindowPadding;

    // Calculate input box size
    ImVec2 textSize = ImGui::CalcTextSize(hint.c_str());
    float inputWidth = 300.0f; // default width for input
    float inputHeight = textSize.y + framePadding.y * 2;

    // Total window size
    float desiredWidth = inputWidth + windowPadding.x * 2;
    float desiredHeight = inputHeight + windowPadding.y * 2;

    lastSize = ImVec2(desiredWidth, desiredHeight);

    // Set the window to fill the entire display
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("Select",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    // Set the input text width
    ImGui::SetNextItemWidth(inputWidth);

    // Set focus to the input field on first frame
    if (ImGui::IsWindowAppearing()) {
        ImGui::SetKeyboardFocusHere();
    }

    bool enterPressed =
        ImGui::InputTextWithHint("##pass", hint.c_str(), inputBuffer, bufSize, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::End();

    if (enterPressed) {
        std::cout << inputBuffer << std::endl;
        std::cout.flush();
    }
    return !enterPressed;
}

Vec2 TextInput::getSize() { return Vec2{lastSize.x, lastSize.y}; }
