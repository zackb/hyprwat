#pragma once

#include "../ui.hpp"
#include <string>

class TextInput : public Frame {
public:
    TextInput(const std::string& hint = "", bool password = false) : hint(hint), password(password) {}
    virtual FrameResult render() override;
    virtual Vec2 getSize() override;

private:
    static constexpr uint32_t buf_size = 128;
    char input_buffer[buf_size] = {0};
    ImVec2 last_size = ImVec2(0, 0);
    std::string hint;
    bool password = false;
    bool should_focus = true;
};
