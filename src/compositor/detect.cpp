#include "../debug/log.hpp"
#include "../hyprland/ipc.hpp"
#include "fenriz.hpp"

#include <cstdlib>

namespace compositor {

    std::unique_ptr<Compositor> detect() {
        try {
            // fenriz first: a fenriz session nested inside Hyprland inherits
            // HYPRLAND_INSTANCE_SIGNATURE, but FENRIZ_SOCKET is the more specific signal.
            if (std::getenv("FENRIZ_SOCKET")) {
                return std::make_unique<Fenriz>();
            }
            if (std::getenv("HYPRLAND_INSTANCE_SIGNATURE")) {
                return std::make_unique<hyprland::Control>();
            }
        } catch (const std::exception& e) {
            debug::log(ERR, "Failed to connect to compositor: {}", e.what());
            return nullptr;
        }
        return nullptr;
    }
} // namespace compositor
