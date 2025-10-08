#include "input.hpp"

bool TextInput::render() {
    ImGui::Begin("Select",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoResize);

    ImGui::InputTextWithHint("##pass", "Passphrase", inputBuffer, bufSize, ImGuiInputTextFlags_EnterReturnsTrue);

    ImGui::End();
    return true;
}

Vec2 TextInput::getSize() { return Vec2{500, 500}; }
