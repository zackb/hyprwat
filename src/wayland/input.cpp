#include "input.hpp"

extern "C" {
#include <linux/input-event-codes.h>
}

namespace wl {
    InputHandler::InputHandler(wl_seat* seat) : seat(seat), io(nullptr) {
        wl_seat_add_listener(seat, &seat_listener, this);
    }

    InputHandler::InputHandler(wl_seat* seat, ImGuiIO* io) : seat(seat), io(io) {
        wl_seat_add_listener(seat, &seat_listener, this);
    }

    InputHandler::~InputHandler() {
        if (keyboard)
            wl_keyboard_destroy(keyboard);
        if (pointer)
            wl_pointer_destroy(pointer);
    }

    void InputHandler::setWindowBounds(int w, int h) {
        this->width = w;
        this->height = h;
    }

    // calback for the wayland seat capabilities event
    void InputHandler::seat_capabilities(void* data, wl_seat* seat, uint32_t capabilities) {
        InputHandler* self = static_cast<InputHandler*>(data);

        // check if the seat has pointer capabilities
        if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
            self->pointer = wl_seat_get_pointer(seat);
            static const wl_pointer_listener pointer_listener = {.enter = pointer_enter,
                                                                 .leave = pointer_leave,
                                                                 .motion = pointer_motion,
                                                                 .button = pointer_button,
                                                                 .axis = pointer_axis,
                                                                 .frame = pointer_frame,
                                                                 .axis_source = pointer_axis_source,
                                                                 .axis_stop = pointer_axis_stop,
                                                                 .axis_discrete = pointer_axis_discrete};
            wl_pointer_add_listener(self->pointer, &pointer_listener, self);
        }

        // check if the seat has keyboard capabilities
        if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
            self->keyboard = wl_seat_get_keyboard(seat);
            static const wl_keyboard_listener keyboard_listener = {.keymap = keyboard_keymap,
                                                                   .enter = keyboard_enter,
                                                                   .leave = keyboard_leave,
                                                                   .key = keyboard_key,
                                                                   .modifiers = keyboard_modifiers,
                                                                   .repeat_info = keyboard_repeat_info};
            wl_keyboard_add_listener(self->keyboard, &keyboard_listener, self);
        }
    }

    void InputHandler::seat_name(void*, wl_seat*, const char*) {}

    void InputHandler::pointer_enter(void* data, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t sx, wl_fixed_t sy) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->io->MousePos = ImVec2((float)wl_fixed_to_int(sx), (float)wl_fixed_to_int(sy));
    }

    void InputHandler::pointer_leave(void*, wl_pointer*, uint32_t, wl_surface*) {}

    void InputHandler::pointer_motion(void* data, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->io->MousePos = ImVec2((float)wl_fixed_to_int(sx), (float)wl_fixed_to_int(sy));
    }

    void InputHandler::pointer_button(void* data, wl_pointer*, uint32_t, uint32_t, uint32_t button, uint32_t state) {
        InputHandler* self = static_cast<InputHandler*>(data);

        if (button == BTN_LEFT) {
            self->io->MouseDown[0] = (state == WL_POINTER_BUTTON_STATE_PRESSED);

            if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
                float x = self->io->MousePos.x;
                float y = self->io->MousePos.y;
                if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
                    self->should_exit = true;
                }
            }
        }
    }

    void InputHandler::pointer_axis(void* data, wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
            self->io->MouseWheel += wl_fixed_to_double(value);
        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
            self->io->MouseWheelH += wl_fixed_to_double(value);
    }

    void InputHandler::pointer_frame(void*, wl_pointer*) {}
    void InputHandler::pointer_axis_source(void*, wl_pointer*, uint32_t) {}
    void InputHandler::pointer_axis_stop(void*, wl_pointer*, uint32_t, uint32_t) {}

    void InputHandler::pointer_axis_discrete(void* data, wl_pointer*, uint32_t axis, int32_t discrete) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
            self->io->MouseWheel += (float)discrete;
        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
            self->io->MouseWheelH += (float)discrete;
    }

    void InputHandler::keyboard_keymap(void*, wl_keyboard*, uint32_t, int32_t, uint32_t) {}
    void InputHandler::keyboard_enter(void*, wl_keyboard*, uint32_t, wl_surface*, wl_array*) {}
    void InputHandler::keyboard_leave(void*, wl_keyboard*, uint32_t, wl_surface*) {}
    void InputHandler::keyboard_key(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t) {}
    void InputHandler::keyboard_modifiers(void*, wl_keyboard*, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t) {}
    void InputHandler::keyboard_repeat_info(void*, wl_keyboard*, int32_t, int32_t) {}
} // namespace wl
