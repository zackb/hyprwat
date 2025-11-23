#include "layer_surface.hpp"

namespace wl {
    LayerSurface::LayerSurface(wl_compositor* compositor, zwlr_layer_shell_v1* shell)
        : compositor(compositor), layerShell(shell) {}

    LayerSurface::~LayerSurface() {
        if (layerSurface)
            zwlr_layer_surface_v1_destroy(layerSurface);
        if (surface_)
            wl_surface_destroy(surface_);
    }

    // create the layer surface at given position and size
    void LayerSurface::create(int x, int y, int width, int height) {
        width_ = width;
        height_ = height;

        surface_ = wl_compositor_create_surface(compositor);
        layerSurface = zwlr_layer_shell_v1_get_layer_surface(
            layerShell, surface_, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "popup_menu");

        static const zwlr_layer_surface_v1_listener listener = {.configure = configureHandler, .closed = closedHandler};
        zwlr_layer_surface_v1_add_listener(layerSurface, &listener, this);

        zwlr_layer_surface_v1_set_size(layerSurface, width, height);
        zwlr_layer_surface_v1_set_anchor(layerSurface,
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
        zwlr_layer_surface_v1_set_margin(layerSurface, y, 0, 0, x);
        zwlr_layer_surface_v1_set_keyboard_interactivity(layerSurface,
                                                         ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
        zwlr_layer_surface_v1_set_exclusive_zone(layerSurface, 0);

        wl_surface_commit(surface_);
    }

    // resize the layer surface and associated EGL window
    void LayerSurface::resize(int newWidth, int newHeight, egl::Context& egl) {
        width_ = newWidth;
        height_ = newHeight;

        // Resize the layer surface
        zwlr_layer_surface_v1_set_size(layerSurface, newWidth, newHeight);
        wl_surface_commit(surface_);

        // Resize the EGL window
        if (egl.window()) {
            const int bufW = newWidth * (scale_ > 0 ? scale_ : 1);
            const int bufH = newHeight * (scale_ > 0 ? scale_ : 1);
            wl_egl_window_resize(egl.window(), bufW, bufH, 0, 0);
        }
    }

    // set the buffer scale for hidpi support
    void LayerSurface::bufferScale(int32_t scale) {
        scale_ = scale > 0 ? scale : 1;
        wl_surface_set_buffer_scale(surface_, scale_);
        wl_surface_commit(surface_);
    }

    void LayerSurface::configureHandler(
        void* data, zwlr_layer_surface_v1* layerSurface, uint32_t serial, uint32_t width, uint32_t height) {
        LayerSurface* self = static_cast<LayerSurface*>(data);

        if (width > 0)
            self->width_ = width;
        if (height > 0)
            self->height_ = height;

        zwlr_layer_surface_v1_ack_configure(layerSurface, serial);
        wl_surface_commit(self->surface_);
        self->configured = true;
    }

    void LayerSurface::closedHandler(void* data, zwlr_layer_surface_v1*) {
        LayerSurface* self = static_cast<LayerSurface*>(data);
        self->shouldExit_ = true;
    }

    // logical pixel coordinates
    void LayerSurface::reposition(
        int x, int y, int viewportWidth, int viewportHeight, int windowWidth, int windowHeight) {
        int finalX = x;
        int finalY = y;

        if (x + windowWidth > viewportWidth) {
            finalX = viewportWidth - windowWidth;
        }

        if (y + windowHeight > viewportHeight) {
            finalY = viewportHeight - windowHeight;
        }

        finalX = std::max(0, finalX);
        finalY = std::max(0, finalY);

        zwlr_layer_surface_v1_set_margin(layerSurface, finalY, 0, 0, finalX);
        wl_surface_commit(surface_);
    }
} // namespace wl
