#include "ui.hpp"
#include "imgui_impl_opengl3.h"
#include "src/font/font.hpp"
#include <GL/gl.h>

void UI::init(int x, int y) {

    // Create wl layer surface with small but reasonable initial size
    // Small enough to avoid flash, large enough for ImGui to work properly
    int initialWidth = 50;
    int initialHeight = 50;

    surface = std::make_unique<wl::LayerSurface>(wayland.display().compositor(), wayland.display().layerShell());
    surface->create(x, y, initialWidth, initialHeight);
    while (!surface->isConfigured()) {
        wayland.display().dispatch();
    }

    // Get the current maximum scale from all outputs
    currentScale = wayland.display().getMaxScale();

    // Set up callback for dynamic scale changes
    wayland.display().setScaleChangeCallback([this](int32_t newScale) { updateScale(newScale); });

    // Apply buffer scale for hidpi. Keep logical size for layer surface sizing,
    // but render buffers (EGL window) in pixel size.
    surface->bufferScale(currentScale);

    // Initialize EGL
    egl = std::make_unique<egl::Context>(wayland.display().display());

    // Create EGL window with buffer pixel size (logical * buffer_scale)
    const int buf_w = surface->width() * currentScale;
    const int buf_h = surface->height() * currentScale;
    if (!egl->createWindowSurface(surface->surface(), buf_w, buf_h)) {
        throw std::runtime_error("Failed to create EGL window surface");
    }

    // enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplOpenGL3_Init("#version 100");
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    // Logical size in points
    io.DisplaySize = ImVec2((float)surface->width(), (float)surface->height());

    // Framebuffer scale = buffer pixels / logical points
    io.DisplayFramebufferScale = ImVec2((float)currentScale, (float)currentScale);

    // load user font if available
    auto fontPath = font::defaultFontPath();
    if (!fontPath.empty()) {
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 14.0f);
        io.FontDefault = font;
    }

    // hidpi handled by DisplayFramebufferScale
    io.FontGlobalScale = 1.0f;

    // Set up our ImGui style
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    // style.ScaleAllSizes(scale);

    style.ItemSpacing = ImVec2(10, 6);
    // style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); -- no transparent
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.95f);

    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.WindowPadding = ImVec2(10, 10);
    style.FramePadding = ImVec2(8, 4);

    // Set up input handling wayland -> imgui
    wayland.input().setIO(&io);
    // Input bounds in logical units
    wayland.input().setWindowBounds(initialWidth, initialHeight);
}

// main loop
void UI::run(Frame& frame) {
    while (running && !surface->shouldExit()) {
        // Process Wayland events
        wayland.display().prepareRead();
        wayland.display().flush();
        wayland.display().readEvents();
        wayland.display().dispatchPending();

        // Check if user clicked outside
        if (wayland.input().shouldExit()) {
            running = false;
            break;
        }

        // Render ImGui frame
        renderFrame(frame);
    }
}

void UI::renderFrame(Frame& frame) {

    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = 1.0f / 60.0f;
    // DisplaySize will be updated after potential resize
    io.DisplayFramebufferScale = ImVec2((float)currentScale, (float)currentScale);

    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui::NewFrame();
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    static Vec2 lastWindowSize;
    static int resizeStabilityCounter = 0;
    static int frameCount = 0;
    frameCount++;

    running = frame.render();
    Vec2 desiredSize = frame.getSize();

    // Render (but don't swap yet)
    ImGui::Render();

    const int RESIZE_STABILITY_FRAMES = (frameCount < 20) ? 0 : 3; // Resize immediately for first 20 frames

    // Check if size changed
    if (desiredSize != lastWindowSize) {
        resizeStabilityCounter = 0; // Reset counter on size change
        lastWindowSize = desiredSize;
    } else {
        resizeStabilityCounter++;
    }

    // Resize conditions: immediately for first frames, or after stability for later frames
    bool shouldResize = (resizeStabilityCounter >= RESIZE_STABILITY_FRAMES) &&
                        (desiredSize.x != surface->width() || desiredSize.y != surface->height()) &&
                        (desiredSize.x > 0 && desiredSize.y > 0); // Ensure valid size

    if (shouldResize) {
        // Clamp to reasonable bounds
        int newWidth = std::max(100, (int)desiredSize.x);
        int newHeight = std::max(50, (int)desiredSize.y);

        surface->resize(newWidth, newHeight, *egl);
        wayland.input().setWindowBounds(newWidth, newHeight);

        // Update display size immediately for current frame
        io.DisplaySize = ImVec2((float)newWidth, (float)newHeight);
    } else {
        // Ensure DisplaySize is always current
        io.DisplaySize = ImVec2((float)surface->width(), (float)surface->height());
    }

    // Use buffer pixel size for viewport (after any resize)
    Vec2 bufSize = egl->getBufferSize();
    glViewport(0, 0, (int)bufSize.x, (int)bufSize.y);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // EGL buffer swap
    egl->swapBuffers();
}

void UI::updateScale(int32_t newScale) {

    if (newScale == currentScale || !surface || !egl) {
        return;
    }

    currentScale = newScale;

    // Update buffer scale
    surface->bufferScale(currentScale);

    // Resize EGL window with new buffer size
    const int buf_w = surface->width() * currentScale;
    const int buf_h = surface->height() * currentScale;
    if (egl->window()) {
        wl_egl_window_resize(egl->window(), buf_w, buf_h, 0, 0);
    }

    // ImGui will be updated in the next renderFrame() call
}
