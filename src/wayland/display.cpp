#include "display.hpp"
#include "../debug/log.hpp"
#include <algorithm>
#include <cstring>

namespace wl {
    Display::Display() {}

    Display::~Display() {
        for (auto& output : outputs_) {
            if (output.output)
                wl_output_destroy(output.output);
        }
        if (seat_)
            wl_seat_destroy(seat_);
        if (layerShell_)
            zwlr_layer_shell_v1_destroy(layerShell_);
        if (compositor_)
            wl_compositor_destroy(compositor_);
        if (registry)
            wl_registry_destroy(registry);
        if (display_)
            wl_display_disconnect(display_);
    }

    bool Display::connect() {
        display_ = wl_display_connect(nullptr);
        if (!display_) {
            debug::log(ERR, "Failed to connect to Wayland display");
            return false;
        }

        registry = wl_display_get_registry(display_);
        wl_registry_listener listener = {.global = registryHandler, .global_remove = registryRemover};
        wl_registry_add_listener(registry, &listener, this);
        wl_display_roundtrip(display_);

        if (!compositor_ || !layerShell_) {
            debug::log(ERR, "Compositor or layer shell not available");
            return false;
        }

        return true;
    }

    void Display::dispatch() { wl_display_dispatch(display_); }

    void Display::dispatchPending() { wl_display_dispatch_pending(display_); }

    void Display::roundtrip() { wl_display_roundtrip(display_); }

    void Display::prepareRead() {
        while (wl_display_prepare_read(display_) != 0) {
            wl_display_dispatch_pending(display_);
        }
    }

    void Display::readEvents() { wl_display_read_events(display_); }

    void Display::flush() { wl_display_flush(display_); }

    void Display::registryHandler(
        void* data, wl_registry* registry, uint32_t id, const char* interface, uint32_t version) {
        Display* self = static_cast<Display*>(data);

        if (strcmp(interface, wl_compositor_interface.name) == 0) {
            self->compositor_ =
                static_cast<wl_compositor*>(wl_registry_bind(registry, id, &wl_compositor_interface, 4));
        } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0) {
            self->layerShell_ =
                static_cast<zwlr_layer_shell_v1*>(wl_registry_bind(registry, id, &zwlr_layer_shell_v1_interface, 1));
        } else if (strcmp(interface, wl_seat_interface.name) == 0) {
            self->seat_ = static_cast<wl_seat*>(wl_registry_bind(registry, id, &wl_seat_interface, 5));
        } else if (strcmp(interface, wl_output_interface.name) == 0) {
            wl_output* output = static_cast<wl_output*>(wl_registry_bind(registry, id, &wl_output_interface, 4));

            static const wl_output_listener output_listener = {.geometry = outputGeometry,
                                                               .mode = outputMode,
                                                               .done = outputDone,
                                                               .scale = outputScale,
                                                               .name = outputName,
                                                               .description = outputDescription};
            wl_output_add_listener(output, &output_listener, self);

            self->outputs_.push_back({output, 1, id, 0, 0});
        }
    }

    int32_t Display::getMaxScale() const {
        int32_t max = 1;
        for (const auto& output : outputs_) {
            if (output.scale > max) {
                max = output.scale;
            }
        }
        return max;
    }

    std::pair<int32_t, int32_t> Display::getOutputSize() const {
        if (outputs_.empty()) {
            return {1920, 1080}; // fallback?
        }
        // TODO: how do we handle multiple outputs with different sizes?
        return {outputs_[0].width, outputs_[0].height};
    }

    void Display::registryRemover(void* data, wl_registry*, uint32_t id) {
        Display* self = static_cast<Display*>(data);

        // Remove output if it was removed
        auto it = std::find_if(
            self->outputs_.begin(), self->outputs_.end(), [id](const Output& output) { return output.id == id; });
        if (it != self->outputs_.end()) {
            if (it->output)
                wl_output_destroy(it->output);
            self->outputs_.erase(it);

            // Notify about scale change
            if (self->scaleCallback) {
                self->scaleCallback(self->getMaxScale());
            }
        }
    }

    // Output event handlers
    void Display::outputGeometry(
        void*, wl_output*, int32_t, int32_t, int32_t, int32_t, int32_t, const char*, const char*, int32_t) {}

    void Display::outputMode(
        void* data, wl_output* output, uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
        if (flags & WL_OUTPUT_MODE_CURRENT) {
            Display* self = static_cast<Display*>(data);

            // update output dimensions
            for (auto& out : self->outputs_) {
                if (out.output == output) {
                    out.width = width;
                    out.height = height;
                    break;
                }
            }
        }
    }

    void Display::outputDone(void*, wl_output*) {}

    void Display::outputScale(void* data, wl_output* output, int32_t factor) {
        Display* self = static_cast<Display*>(data);

        // Find and update the output scale
        for (auto& out : self->outputs_) {
            if (out.output == output) {
                out.scale = factor;

                // Notify about scale change
                if (self->scaleCallback) {
                    self->scaleCallback(self->getMaxScale());
                }
                break;
            }
        }
    }

    void Display::outputName(void*, wl_output*, const char*) {}
    void Display::outputDescription(void*, wl_output*, const char*) {}
} // namespace wl
