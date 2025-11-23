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
        int32_t width;
        int32_t height;
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

        wl_display* display() const { return display_; }
        wl_compositor* compositor() const { return compositor_; }
        zwlr_layer_shell_v1* layerShell() const { return layer_shell; }
        wl_seat* seat() const { return seat_; }

        // Output scale management
        const std::vector<Output>& outputs() const { return outputs_; }
        int32_t getMaxScale() const;
        void setScaleChangeCallback(std::function<void(int32_t)> callback) { scale_callback = callback; }

        // get the size of the first output in physical pixels
        std::pair<int32_t, int32_t> getOutputSize() const;

    private:
        wl_display* display_;
        wl_registry* registry = nullptr;
        wl_compositor* compositor_ = nullptr;
        zwlr_layer_shell_v1* layer_shell = nullptr;
        wl_seat* seat_ = nullptr;

        std::vector<Output> outputs_;
        std::function<void(int32_t)> scale_callback;

        static void
            registryHandler(void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version);
        static void registryRemover(void* data, wl_registry* registry, uint32_t id);

        // Output callbacks
        static void outputGeometry(void* data,
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
            outputMode(void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh);
        static void outputDone(void* data, wl_output* output);
        static void outputScale(void* data, wl_output* output, int32_t factor);
        static void outputName(void* data, wl_output* output, const char* name);
        static void outputDescription(void* data, wl_output* output, const char* description);
    };
} // namespace wl
