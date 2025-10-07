#include "wayland.hpp"
#include <stdexcept>

namespace wl {
    Wayland::Wayland() : m_display() {
        if (!m_display.connect()) {
            throw std::runtime_error("Failed to connect to Wayland display");
        }
        m_input = std::make_unique<InputHandler>(m_display.seat());
    }

    Wayland::~Wayland() {}

} // namespace wl
