#include "layer_surface.hpp"

namespace wl {
    LayerSurface::LayerSurface(wl_compositor* compositor, zwlr_layer_shell_v1* shell)
        : compositor(compositor), layer_shell(shell) {}

    LayerSurface::~LayerSurface() {
        if (layer_surface)
            zwlr_layer_surface_v1_destroy(layer_surface);
        if (surface_)
            wl_surface_destroy(surface_);
    }

    // create the layer surface at given position and size
    void LayerSurface::create(int x, int y, int width, int height) {
        width = width;
        height_ = height;

        surface_ = wl_compositor_create_surface(compositor);
        layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            layer_shell, surface_, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "popup_menu");

        static const zwlr_layer_surface_v1_listener listener = {.configure = configureHandler, .closed = closedHandler};
        zwlr_layer_surface_v1_add_listener(layer_surface, &listener, this);

        zwlr_layer_surface_v1_set_size(layer_surface, width, height);
        zwlr_layer_surface_v1_set_anchor(layer_surface,
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
        zwlr_layer_surface_v1_set_margin(layer_surface, y, 0, 0, x);
        zwlr_layer_surface_v1_set_keyboard_interactivity(layer_surface,
                                                         ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_EXCLUSIVE);
        zwlr_layer_surface_v1_set_exclusive_zone(layer_surface, 0);

        wl_surface_commit(surface_);
    }

    // resize the layer surface and associated EGL window
    void LayerSurface::resize(int new_width, int new_height, egl::Context& egl) {
        width_ = new_width;
        height_ = new_height;

        // Resize the layer surface
        zwlr_layer_surface_v1_set_size(layer_surface, new_width, new_height);
        wl_surface_commit(surface_);

        // Resize the EGL window
        if (egl.window()) {
            const int buf_w = new_width * (scale_ > 0 ? scale_ : 1);
            const int buf_h = new_height * (scale_ > 0 ? scale_ : 1);
            wl_egl_window_resize(egl.window(), buf_w, buf_h, 0, 0);
        }
    }

    // set the buffer scale for hidpi support
    void LayerSurface::bufferScale(int32_t scale) {
        scale_ = scale > 0 ? scale : 1;
        wl_surface_set_buffer_scale(surface_, scale_);
        wl_surface_commit(surface_);
    }

    void LayerSurface::configureHandler(
        void* data, zwlr_layer_surface_v1* layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
        LayerSurface* self = static_cast<LayerSurface*>(data);

        if (width > 0)
            self->width_ = width;
        if (height > 0)
            self->height_ = height;

        zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
        wl_surface_commit(self->surface_);
        self->configured = true;
    }

    void LayerSurface::closedHandler(void* data, zwlr_layer_surface_v1*) {
        LayerSurface* self = static_cast<LayerSurface*>(data);
        self->should_exit = true;
    }

    // logical pixel coordinates
    void LayerSurface::reposition(
        int x, int y, int viewport_width, int viewport_height, int window_width, int window_height) {
        int final_x = x;
        int final_y = y;

        if (x + window_width > viewport_width) {
            final_x = viewport_width - window_width;
        }

        if (y + window_height > viewport_height) {
            final_y = viewport_height - window_height;
        }

        final_x = std::max(0, final_x);
        final_y = std::max(0, final_y);

        zwlr_layer_surface_v1_set_margin(layer_surface, final_y, 0, 0, final_x);
        wl_surface_commit(surface_);
    }
} // namespace wl
