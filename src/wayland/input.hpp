#pragma once

extern "C" {
#include <wayland-client.h>
}

#include "imgui.h"

namespace wl {

    // Handles input from a wl_seat and forwards it to ImGui
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

        // Seat callbacks
        static void seat_capabilities(void* data, wl_seat* seat, uint32_t capabilities);
        static void seat_name(void* data, wl_seat* seat, const char* name);

        // Listener
        constexpr static const wl_seat_listener seat_listener = {.capabilities = seat_capabilities, .name = seat_name};

        // Pointer callbacks
        static void pointer_enter(
            void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface, wl_fixed_t sx, wl_fixed_t sy);
        static void pointer_leave(void* data, wl_pointer* pointer, uint32_t serial, wl_surface* surface);
        static void pointer_motion(void* data, wl_pointer* pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy);
        static void pointer_button(
            void* data, wl_pointer* pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state);
        static void pointer_axis(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis, wl_fixed_t value);
        static void pointer_frame(void* data, wl_pointer* pointer);
        static void pointer_axis_source(void* data, wl_pointer* pointer, uint32_t axis_source);
        static void pointer_axis_stop(void* data, wl_pointer* pointer, uint32_t time, uint32_t axis);
        static void pointer_axis_discrete(void* data, wl_pointer* pointer, uint32_t axis, int32_t discrete);

        // Keyboard callbacks
        static void keyboard_keymap(void* data, wl_keyboard* keyboard, uint32_t format, int32_t fd, uint32_t size);
        static void
            keyboard_enter(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface, wl_array* keys);
        static void keyboard_leave(void* data, wl_keyboard* keyboard, uint32_t serial, wl_surface* surface);
        static void keyboard_key(
            void* data, wl_keyboard* keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state);
        static void keyboard_modifiers(void* data,
                                       wl_keyboard* keyboard,
                                       uint32_t serial,
                                       uint32_t mods_depressed,
                                       uint32_t mods_latched,
                                       uint32_t mods_locked,
                                       uint32_t group);
        static void keyboard_repeat_info(void* data, wl_keyboard* keyboard, int32_t rate, int32_t delay);
    };
} // namespace wl
