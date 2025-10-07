#pragma once

#include "src/vec2.hpp"
extern "C" {
#include <wayland-client.h>
#include <wayland-egl.h>
}

#include <EGL/egl.h>

namespace egl {
    class Context {
    public:
        Context(wl_display* display);
        ~Context();

        bool createWindowSurface(wl_surface* surface, int width, int height);
        void makeCurrent();
        void swapBuffers();
        Vec2 getBufferSize() const;
        wl_egl_window* window() const { return egl_window; }

    private:
        wl_display* display;
        EGLDisplay egl_display;
        EGLContext egl_context;
        EGLSurface egl_surface;
        EGLConfig egl_config;
        wl_egl_window* egl_window = nullptr;
    };
} // namespace egl
