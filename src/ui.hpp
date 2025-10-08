#pragma once

#include "src/config.hpp"
#include "src/vec.hpp"
#include "src/wayland/layer_surface.hpp"
#include "src/wayland/wayland.hpp"

// Forward declaration
struct FrameResult;

class Frame {
public:
    virtual ~Frame() = default;

    // render the frame and return the result of user interaction
    // returns FrameResult indicating continue, submit, or cancel
    virtual FrameResult render() = 0;

    virtual Vec2 getSize() = 0;
    virtual void applyTheme(const Config& config) {};
};

class UI {
public:
    UI(wl::Wayland& wayland) : wayland(wayland) {}
    void init(int x, int y);

    // run a single frame until it returns a result
    FrameResult run(Frame& frame);

    // Run a flow until completion
    void runFlow(class Flow& flow);

    void applyTheme(const Config& config);

    Config* currentConfig = nullptr;

private:
    wl::Wayland& wayland;
    std::unique_ptr<wl::LayerSurface> surface;
    std::unique_ptr<egl::Context> egl;
    int32_t currentScale = 1;
    bool running = true;

    FrameResult renderFrame(Frame& frame);
    void updateScale(int32_t new_scale);
    void setupFont(ImGuiIO& io, const Config& config);
};
