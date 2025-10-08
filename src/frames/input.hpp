#pragma once

#include "../ui.hpp"

class TextInput : public Frame {
public:
    virtual bool render();
    virtual Vec2 getSize();

private:
    static constexpr uint32_t bufSize = 128;
    char inputBuffer[bufSize] = {0};
    ImVec2 lastSize = ImVec2(0, 0);
};
