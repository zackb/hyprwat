#pragma once

extern "C" {
#include <wayland-client.h>
}

#include "imgui.h"
#include <xkbcommon/xkbcommon.h>

#define XKB_MOD_SHIFT (1 << 0)
#define XKB_MOD_LOCK (1 << 1)
#define XKB_MOD_CTRL (1 << 2)
#define XKB_MOD_ALT (1 << 3)
#define XKB_MOD_LOGO (1 << 6)

namespace wl {

    // handles input from a wl_seat and forwards it to ImGui
    class InputHandler {
    public:
        InputHandler(wl_seat*);
        InputHandler(wl_seat* seat, ImGuiIO* io);
        ~InputHandler();

        void setWindowBounds(int width, int height);
        bool shouldExit() const { return should_exit; }
        void setIO(ImGuiIO* new_io) { io = new_io; }

    private:
        wl_seat* seat;
        wl_pointer* pointer = nullptr;
        wl_keyboard* keyboard = nullptr;
        ImGuiIO* io;
        int width = 0;
        int height = 0;
        bool should_exit = false;

        // xkb state
        struct xkb_context* context = nullptr;
        struct xkb_keymap* keymap = nullptr;
        struct xkb_state* state = nullptr;
        xkb_mod_mask_t control_mask = 0;
        xkb_mod_mask_t alt_mask = 0;
        xkb_mod_mask_t shift_mask = 0;
        xkb_mod_mask_t super_mask = 0;

        // key repeat state
        int32_t repeat_rate = 0;
        int32_t repeat_delay = 0;

        // kb helper functions
        void updateModifiers(xkb_state* state);
        void handleKey(uint32_t key, bool pressed);

        // seat callbacks
        static void seatCapabilities(void* data, wl_seat* seat, uint32_t capabilities);
        static void seatName(void* data, wl_seat* seat, const char* name);

        // listener
        constexpr static const wl_seat_listener seat_listener = {.capabilities = seatCapabilities, .name = seatName};

        // pointer callbacks
        static void pointerEnter(
            void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
        static void pointerLeave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
        static void pointerMotion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
        static void pointerButton(
            void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        static void pointerAxis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
        static void pointerFrame(void* data, wl_pointer* pointer);
        static void pointerAxisSource(void* data, wl_pointer* pointer, uint32_t axis_source);
        static void pointerAxisStop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis);
        static void pointerAxisDiscrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);

        // keyboard callbacks
        static void keyboardKeymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
        static void
            keyboardEnter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
        static void keyboardLeave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
        static void keyboardKey(
            void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
        static void keyboardModifiers(void* data,
                                      wl_keyboard* keyboard,
                                      uint32_t serial,
                                      uint32_t mods_depressed,
                                      uint32_t mods_latched,
                                      uint32_t mods_locked,
                                      uint32_t group);
        static void keyboardRepeatInfo(void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay);
    };
} // namespace wl
