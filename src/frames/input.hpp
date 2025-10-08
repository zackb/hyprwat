#pragma once

#include "../ui.hpp"
#include <string>

class TextInput : public Frame {
public:
    TextInput(const std::string& hint = "") : hint(hint) {}
    virtual bool render();
    virtual Vec2 getSize();

private:
    static constexpr uint32_t bufSize = 128;
    char inputBuffer[bufSize] = {0};
    ImVec2 lastSize = ImVec2(0, 0);
    std::string hint;
};
