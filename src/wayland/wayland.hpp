#pragma once

#include "src/wayland/display.hpp"
#include "src/wayland/input.hpp"
#include <memory>

namespace wl {
    class Wayland {
    public:
        Wayland();
        ~Wayland();
        Display& display() { return display_; }
        InputHandler& input() { return *input_; }

    private:
        Display display_;
        std::unique_ptr<InputHandler> input_;
    };
} // namespace wl
