#include "egl_context.hpp"
#include <cstdio>

namespace egl {

    Context::Context(wl_display* display) : display(display) {
        egl_display = eglGetDisplay((EGLNativeDisplayType)display);
        if (egl_display == EGL_NO_DISPLAY) {
            fprintf(stderr, "Failed to get EGL display\n");
            return;
        }

        if (!eglInitialize(egl_display, nullptr, nullptr)) {
            fprintf(stderr, "Failed to initialize EGL\n");
            return;
        }

        EGLint num_config;
        // static const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT, EGL_NONE};
        static const EGLint attribs[] = {EGL_SURFACE_TYPE,
                                         EGL_WINDOW_BIT,
                                         EGL_RENDERABLE_TYPE,
                                         EGL_OPENGL_ES2_BIT, // Use ES2 instead
                                         EGL_RED_SIZE,
                                         8,
                                         EGL_GREEN_SIZE,
                                         8,
                                         EGL_BLUE_SIZE,
                                         8,
                                         EGL_ALPHA_SIZE,
                                         8,
                                         EGL_NONE};
        eglChooseConfig(egl_display, attribs, &egl_config, 1, &num_config);

        eglBindAPI(EGL_OPENGL_ES_API);
        static const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION,
                                                 2, // Request ES 2.0
                                                 EGL_NONE};
        egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);
        if (egl_context == EGL_NO_CONTEXT) {
            fprintf(stderr, "Failed to create EGL context\n");
            fprintf(stderr, "Error code: %d\n", eglGetError());
            return;
        }

        egl_surface = EGL_NO_SURFACE;
    }

    Vec2 Context::getBufferSize() const {
        int width, height;
        wl_egl_window_get_attached_size(egl_window, &width, &height);
        return {(float)width, (float)height};
    }

    Context::~Context() {
        if (egl_window)
            wl_egl_window_destroy(egl_window);
        if (egl_surface != EGL_NO_SURFACE)
            eglDestroySurface(egl_display, egl_surface);
        if (egl_context != EGL_NO_CONTEXT)
            eglDestroyContext(egl_display, egl_context);
        if (egl_display != EGL_NO_DISPLAY)
            eglTerminate(egl_display);
    }

    bool Context::createWindowSurface(wl_surface* surface, int width, int height) {
        egl_window = wl_egl_window_create(surface, width, height);
        if (!egl_window) {
            fprintf(stderr, "Failed to create EGL window\n");
            return false;
        }

        EGLint surface_attribs[] = {EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE};

        egl_surface = eglCreateWindowSurface(egl_display, egl_config, (EGLNativeWindowType)egl_window, surface_attribs);
        if (egl_surface == EGL_NO_SURFACE) {
            fprintf(stderr, "Failed to create EGL surface\n");
            fprintf(stderr, "Error code: 0x%x\n", eglGetError());
            return false;
        }

        makeCurrent();
        return true;
    }

    void Context::makeCurrent() { eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context); }

    void Context::swapBuffers() { eglSwapBuffers(egl_display, egl_surface); }
} // namespace egl
