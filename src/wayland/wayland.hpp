#pragma once

#include "src/wayland/display.hpp"
#include "src/wayland/input.hpp"
#include <memory>

namespace wl {
    class Wayland {
    public:
        Wayland();
        ~Wayland();
        Display& display() { return m_display; }
        InputHandler& input() { return *m_input; }

    private:
        Display m_display;
        std::unique_ptr<InputHandler> m_input;
    };
} // namespace wl
