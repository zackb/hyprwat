#pragma once

#include "../renderer/egl_context.hpp"
extern "C" {
#include "protocols/wlr-layer-shell-unstable-v1-client-protocol.h"
#include <wayland-client.h>
}

namespace wl {

    // manages a wl_layer_surface for a Wayland compositor using the wlr-layer-shell protocol
    class LayerSurface {
    public:
        LayerSurface(wl_compositor* compositor, zwlr_layer_shell_v1* shell);
        ~LayerSurface();

        void create(int x, int y, int width, int height);
        bool isConfigured() const { return configured; }
        wl_surface* surface() const { return surface_; }
        void resize(int newWidth, int newHeight, egl::Context& egl);

        void requestExit() { shouldExit_ = true; }
        bool shouldExit() const { return shouldExit_; }

        // logical size
        int width() const { return width_; }
        int height() const { return height_; }

        void bufferScale(int32_t scale);
        int32_t bufferScale() const { return scale_; }
        void reposition(int x, int y, int viewportWidth, int viewportHeight, int windowWidth, int windowHeight);

    private:
        wl_compositor* compositor;
        zwlr_layer_shell_v1* layerShell;
        wl_surface* surface_ = nullptr;
        zwlr_layer_surface_v1* layerSurface = nullptr;
        bool configured = false;
        bool shouldExit_ = false;
        int width_ = 0;
        int height_ = 0;
        int32_t scale_ = 1;

        static void configureHandler(
            void* data, zwlr_layer_surface_v1* layerSurface, uint32_t serial, uint32_t width, uint32_t height);
        static void closedHandler(void* data, zwlr_layer_surface_v1* layerSurface);
    };
} // namespace wl
