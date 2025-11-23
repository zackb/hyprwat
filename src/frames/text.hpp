#pragma once

#include "../imgui/imgui.h"
#include "../ui.hpp"
#include <string>

class Text : public Frame {

public:
    Text(const std::string& txt, ImVec4 col = ImVec4(1, 1, 1, 1)) : text(txt), color(col), lastSize(0, 0) {}

    void setText(const std::string& txt, ImVec4 col = ImVec4(1, 1, 1, 1)) {
        text = txt;
        color = col;
    }

    virtual FrameResult render() override;

    Vec2 getSize() override { return Vec2{lastSize.x, lastSize.y}; }
    void done() { _done = true; }

private:
    std::string text;
    bool _done = false;
    ImVec4 color;
    ImVec2 lastSize;
};
