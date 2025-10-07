#pragma once

#include "../choice.hpp"
#include "imgui.h"
#include "src/ui.hpp"
#include <vector>

class Selector : public Frame {

public:
    void add(Choice& choice) { choices.emplace_back(choice); }
    void add(Choice&& choice) { choices.emplace_back(choice); }
    void setSelected(int index) { selected = index; }

    bool RoundedSelectableFullWidth(const char* label, bool selected, float rounding = 6.0f);

    virtual bool render();
    virtual Vec2 getSize() { return Vec2{lastSize.x, lastSize.y}; }

private:
    int selected = -1;
    std::vector<Choice> choices;
    ImVec4 highlightedColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    ImVec4 selectedColor = ImVec4(0.2f, 0.4f, 0.7f, 0.4f);
    ImVec2 lastSize = ImVec2(0, 0);
};
