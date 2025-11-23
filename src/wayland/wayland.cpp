#include "wayland.hpp"
#include <stdexcept>

namespace wl {
    Wayland::Wayland() : display_() {
        if (!display_.connect()) {
            throw std::runtime_error("Failed to connect to Wayland display");
        }
        input_ = std::make_unique<InputHandler>(display_.seat());
        display_.roundtrip();
    }

    Wayland::~Wayland() {}

} // namespace wl
