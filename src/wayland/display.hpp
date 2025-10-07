#pragma once

extern "C" {
#include "protocols/wlr-layer-shell-unstable-v1-client-protocol.h"
#include <wayland-client.h>
}

#include <functional>
#include <vector>

namespace wl {
    struct Output {
        wl_output* output;
        int32_t scale = 1;
        uint32_t id;
    };

    class Display {
    public:
        Display();
        ~Display();

        bool connect();
        void dispatch();
        void dispatchPending();
        void roundtrip();
        void prepareRead();
        void readEvents();
        void flush();

        wl_display* display() const { return m_display; }
        wl_compositor* compositor() const { return m_compositor; }
        zwlr_layer_shell_v1* layerShell() const { return m_layer_shell; }
        wl_seat* seat() const { return m_seat; }

        // Output scale management
        const std::vector<Output>& outputs() const { return m_outputs; }
        int32_t getMaxScale() const;
        void setScaleChangeCallback(std::function<void(int32_t)> callback) { m_scale_callback = callback; }

    private:
        wl_display* m_display;
        wl_registry* m_registry = nullptr;
        wl_compositor* m_compositor = nullptr;
        zwlr_layer_shell_v1* m_layer_shell = nullptr;
        wl_seat* m_seat = nullptr;

        std::vector<Output> m_outputs;
        std::function<void(int32_t)> m_scale_callback;

        static void
            registry_handler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version);
        static void registry_remover(void* data, wl_registry* registry, uint32_t id);

        // Output callbacks
        static void output_geometry(void* data,
                                    wl_output* output,
                                    int32_t x,
                                    int32_t y,
                                    int32_t physical_width,
                                    int32_t physical_height,
                                    int32_t subpixel,
                                    const char* make,
                                    const char* model,
                                    int32_t transform);
        static void
            output_mode(void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
        static void output_done(void* data, wl_output* output);
        static void output_scale(void* data, wl_output* output, int32_t factor);
        static void output_name(void* data, wl_output* output, const char* name);
        static void output_description(void* data, wl_output* output, const char* description);
    };
} // namespace wl
