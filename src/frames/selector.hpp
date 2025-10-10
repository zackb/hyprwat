#pragma once

#include "../choice.hpp"
#include "imgui.h"
#include "src/ui.hpp"
#include "src/vec.hpp"
#include <vector>

class Selector : public Frame {

public:
    /*
    void add(Choice& choice) { choices.emplace_back(choice); }
    void add(Choice&& choice) { choices.emplace_back(choice); }
    void add(const Choice& choice) { choices.emplace_back(choice); }
    void add(const Choice&& choice) { choices.emplace_back(choice); }
    */
    template <typename T> void add(T&& choice) {
        choices.emplace_back(std::forward<T>(choice));
        if (choice.selected) {
            selected = choices.size() - 1;
        }
    }

    // finds a pointer to choice by its id
    // choice can by mutated
    Choice* findChoiceById(const std::string& id) {
        for (auto& c : choices)
            if (c.id == id)
                return &c;
        return nullptr;
    }

    void setSelected(int index) { selected = index; }

    bool RoundedSelectableFullWidth(const char* label, bool selected, float rounding = 6.0f);

    virtual FrameResult render() override;
    virtual Vec2 getSize() override { return Vec2{lastSize.x, lastSize.y}; }
    virtual void applyTheme(const Config& config) override;

private:
    int selected = -1;
    std::vector<Choice> choices;
    ImVec4 activeColor = ImVec4(0.2f, 0.4f, 0.7f, 1.0f);
    ImVec4 hoverColor = ImVec4(0.2f, 0.4f, 0.7f, 0.4f);
    ImVec2 lastSize = ImVec2(0, 0);
};
