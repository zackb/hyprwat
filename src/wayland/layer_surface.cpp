#include "layer_surface.hpp"

namespace wl {
    LayerSurface::LayerSurface(wl_compositor* compositor, zwlr_layer_shell_v1* shell)
        : m_compositor(compositor), m_layer_shell(shell) {}

    LayerSurface::~LayerSurface() {
        if (m_layer_surface)
            zwlr_layer_surface_v1_destroy(m_layer_surface);
        if (m_surface)
            wl_surface_destroy(m_surface);
    }

    // create the layer surface at given position and size
    void LayerSurface::create(int x, int y, int width, int height) {
        m_width = width;
        m_height = height;

        m_surface = wl_compositor_create_surface(m_compositor);
        m_layer_surface = zwlr_layer_shell_v1_get_layer_surface(
            m_layer_shell, m_surface, nullptr, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "popup_menu");

        static const zwlr_layer_surface_v1_listener listener = {.configure = configure_handler,
                                                                .closed = closed_handler};
        zwlr_layer_surface_v1_add_listener(m_layer_surface, &listener, this);

        zwlr_layer_surface_v1_set_size(m_layer_surface, width, height);
        zwlr_layer_surface_v1_set_anchor(m_layer_surface,
                                         ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
        zwlr_layer_surface_v1_set_margin(m_layer_surface, y, 0, 0, x);
        zwlr_layer_surface_v1_set_keyboard_interactivity(m_layer_surface,
                                                         ZWLR_LAYER_SURFACE_V1_KEYBOARD_INTERACTIVITY_NONE);
        zwlr_layer_surface_v1_set_exclusive_zone(m_layer_surface, 0);

        wl_surface_commit(m_surface);
    }

    // resize the layer surface and associated EGL window
    void LayerSurface::resize(int new_width, int new_height, egl::Context& egl) {
        m_width = new_width;
        m_height = new_height;

        // Resize the layer surface
        zwlr_layer_surface_v1_set_size(m_layer_surface, new_width, new_height);
        wl_surface_commit(m_surface);

        // Resize the EGL window
        if (egl.window()) {
            const int buf_w = new_width * (m_scale > 0 ? m_scale : 1);
            const int buf_h = new_height * (m_scale > 0 ? m_scale : 1);
            wl_egl_window_resize(egl.window(), buf_w, buf_h, 0, 0);
        }
    }

    // set the buffer scale for hidpi support
    void LayerSurface::bufferScale(int32_t scale) {
        m_scale = scale > 0 ? scale : 1;
        wl_surface_set_buffer_scale(m_surface, m_scale);
        wl_surface_commit(m_surface);
    }

    void LayerSurface::configure_handler(
        void* data, zwlr_layer_surface_v1* layer_surface, uint32_t serial, uint32_t width, uint32_t height) {
        LayerSurface* self = static_cast<LayerSurface*>(data);

        if (width > 0)
            self->m_width = width;
        if (height > 0)
            self->m_height = height;

        zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
        wl_surface_commit(self->m_surface);
        self->m_configured = true;
    }

    void LayerSurface::closed_handler(void* data, zwlr_layer_surface_v1*) {
        LayerSurface* self = static_cast<LayerSurface*>(data);
        self->m_should_exit = true;
    }
} // namespace wl
