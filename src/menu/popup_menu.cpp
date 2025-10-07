#include "popup_menu.hpp"
#include "imgui.h"

void PopupMenu::addItem(const std::string& label, std::function<void()> callback) {
    items.push_back({label, callback});
}

void PopupMenu::clear() { items.clear(); }

bool PopupMenu::render(int width, int height) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)width, (float)height));

    ImGui::Begin("Popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::Text("Select an item");

    bool itemClicked = false;
    float button_width = (float)width - 20.0f;

    for (auto& item : items) {
        if (ImGui::Button(item.label.c_str(), ImVec2(button_width, 40))) {
            if (item.callback) {
                item.callback();
            }
            itemClicked = true;
        }
    }

    if (ImGui::Button("Cancel", ImVec2(button_width, 40))) {
        itemClicked = true;
    }

    ImGui::End();

    return itemClicked;
}
