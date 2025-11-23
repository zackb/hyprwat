#include "input.hpp"

extern "C" {
#include <linux/input-event-codes.h>
}
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

// keycode to ImGuiKey mapping
static ImGuiKey ImGui_ImplWayland_KeycodeToImGuiKey(uint32_t keycode);

// map xkb keycode to ImGuiKey
static const struct {
    xkb_keycode_t xkbKeycode;
    ImGuiKey imguiKey;
} KeyMap[] = {
    {XKB_KEY_Return, ImGuiKey_Enter},
    {XKB_KEY_Tab, ImGuiKey_Tab},
    {XKB_KEY_Left, ImGuiKey_LeftArrow},
    {XKB_KEY_Right, ImGuiKey_RightArrow},
    {XKB_KEY_Up, ImGuiKey_UpArrow},
    {XKB_KEY_Down, ImGuiKey_DownArrow},
    {XKB_KEY_Page_Up, ImGuiKey_PageUp},
    {XKB_KEY_Page_Down, ImGuiKey_PageDown},
    {XKB_KEY_Home, ImGuiKey_Home},
    {XKB_KEY_End, ImGuiKey_End},
    {XKB_KEY_Delete, ImGuiKey_Delete},
    {XKB_KEY_BackSpace, ImGuiKey_Backspace},
    {XKB_KEY_space, ImGuiKey_Space},
    {XKB_KEY_Insert, ImGuiKey_Insert},
    {XKB_KEY_Escape, ImGuiKey_Escape},
    {XKB_KEY_Control_L, ImGuiKey_LeftCtrl},
    {XKB_KEY_Control_R, ImGuiKey_RightCtrl},
    {XKB_KEY_Shift_L, ImGuiKey_LeftShift},
    {XKB_KEY_Shift_R, ImGuiKey_RightShift},
    {XKB_KEY_Alt_L, ImGuiKey_LeftAlt},
    {XKB_KEY_Alt_R, ImGuiKey_RightAlt},
    {XKB_KEY_Super_L, ImGuiKey_LeftSuper},
    {XKB_KEY_Super_R, ImGuiKey_RightSuper},
    {XKB_KEY_Menu, ImGuiKey_Menu},
    {XKB_KEY_0, ImGuiKey_0},
    {XKB_KEY_1, ImGuiKey_1},
    {XKB_KEY_2, ImGuiKey_2},
    {XKB_KEY_3, ImGuiKey_3},
    {XKB_KEY_4, ImGuiKey_4},
    {XKB_KEY_5, ImGuiKey_5},
    {XKB_KEY_6, ImGuiKey_6},
    {XKB_KEY_7, ImGuiKey_7},
    {XKB_KEY_8, ImGuiKey_8},
    {XKB_KEY_9, ImGuiKey_9},
    {XKB_KEY_a, ImGuiKey_A},
    {XKB_KEY_b, ImGuiKey_B},
    {XKB_KEY_c, ImGuiKey_C},
    {XKB_KEY_d, ImGuiKey_D},
    {XKB_KEY_e, ImGuiKey_E},
    {XKB_KEY_f, ImGuiKey_F},
    {XKB_KEY_g, ImGuiKey_G},
    {XKB_KEY_h, ImGuiKey_H},
    {XKB_KEY_i, ImGuiKey_I},
    {XKB_KEY_j, ImGuiKey_J},
    {XKB_KEY_k, ImGuiKey_K},
    {XKB_KEY_l, ImGuiKey_L},
    {XKB_KEY_m, ImGuiKey_M},
    {XKB_KEY_n, ImGuiKey_N},
    {XKB_KEY_o, ImGuiKey_O},
    {XKB_KEY_p, ImGuiKey_P},
    {XKB_KEY_q, ImGuiKey_Q},
    {XKB_KEY_r, ImGuiKey_R},
    {XKB_KEY_s, ImGuiKey_S},
    {XKB_KEY_t, ImGuiKey_T},
    {XKB_KEY_u, ImGuiKey_U},
    {XKB_KEY_v, ImGuiKey_V},
    {XKB_KEY_w, ImGuiKey_W},
    {XKB_KEY_x, ImGuiKey_X},
    {XKB_KEY_y, ImGuiKey_Y},
    {XKB_KEY_z, ImGuiKey_Z},
    {XKB_KEY_F1, ImGuiKey_F1},
    {XKB_KEY_F2, ImGuiKey_F2},
    {XKB_KEY_F3, ImGuiKey_F3},
    {XKB_KEY_F4, ImGuiKey_F4},
    {XKB_KEY_F5, ImGuiKey_F5},
    {XKB_KEY_F6, ImGuiKey_F6},
    {XKB_KEY_F7, ImGuiKey_F7},
    {XKB_KEY_F8, ImGuiKey_F8},
    {XKB_KEY_F9, ImGuiKey_F9},
    {XKB_KEY_F10, ImGuiKey_F10},
    {XKB_KEY_F11, ImGuiKey_F11},
    {XKB_KEY_F12, ImGuiKey_F12},
    {XKB_KEY_semicolon, ImGuiKey_Semicolon},
    {XKB_KEY_equal, ImGuiKey_Equal},
    {XKB_KEY_comma, ImGuiKey_Comma},
    {XKB_KEY_minus, ImGuiKey_Minus},
    {XKB_KEY_period, ImGuiKey_Period},
    {XKB_KEY_slash, ImGuiKey_Slash},
    {XKB_KEY_grave, ImGuiKey_GraveAccent},
    {XKB_KEY_bracketleft, ImGuiKey_LeftBracket},
    {XKB_KEY_backslash, ImGuiKey_Backslash},
    {XKB_KEY_bracketright, ImGuiKey_RightBracket},
    {XKB_KEY_apostrophe, ImGuiKey_Apostrophe},
};

namespace wl {
    InputHandler::InputHandler(wl_seat* seat) : seat(seat), io(nullptr) {
        wl_seat_add_listener(seat, &seatListener, this);
        context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    InputHandler::InputHandler(wl_seat* seat, ImGuiIO* io) : seat(seat), io(io) {
        wl_seat_add_listener(seat, &seatListener, this);
        context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    InputHandler::~InputHandler() {
        if (keyboard) {
            wl_keyboard_destroy(keyboard);
            keyboard = nullptr;
        }
        if (pointer) {
            wl_pointer_destroy(pointer);
            pointer = nullptr;
        }
        if (state) {
            xkb_state_unref(state);
            state = nullptr;
        }
        if (keymap) {
            xkb_keymap_unref(keymap);
            keymap = nullptr;
        }
        if (context) {
            xkb_context_unref(context);
            context = nullptr;
        }
    }

    void InputHandler::setWindowBounds(int w, int h) {
        this->width = w;
        this->height = h;
    }

    // calback for the wayland seat capabilities event
    void InputHandler::seatCapabilities(void* data, wl_seat* seat, uint32_t capabilities) {
        InputHandler* self = static_cast<InputHandler*>(data);

        // check if the seat has pointer capabilities
        if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
            self->pointer = wl_seat_get_pointer(seat);
            static const wl_pointer_listener pointerListener = {.enter = pointerEnter,
                                                                .leave = pointerLeave,
                                                                .motion = pointerMotion,
                                                                .button = pointerButton,
                                                                .axis = pointerAxis,
                                                                .frame = pointerFrame,
                                                                .axis_source = pointerAxisSource,
                                                                .axis_stop = pointerAxisStop,
                                                                .axis_discrete = pointerAxisDiscrete};
            wl_pointer_add_listener(self->pointer, &pointerListener, self);
        }

        // check if the seat has keyboard capabilities
        if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
            self->keyboard = wl_seat_get_keyboard(seat);
            static const wl_keyboard_listener keyboardListener = {.keymap = keyboardKeymap,
                                                                  .enter = keyboardEnter,
                                                                  .leave = keyboardLeave,
                                                                  .key = keyboardKey,
                                                                  .modifiers = keyboardModifiers,
                                                                  .repeat_info = keyboardRepeatInfo};
            wl_keyboard_add_listener(self->keyboard, &keyboardListener, self);
        }
    }

    void InputHandler::seatName(void*, wl_seat*, const char*) {}

    void InputHandler::pointerEnter(void* data, wl_pointer*, uint32_t, wl_surface*, wl_fixed_t sx, wl_fixed_t sy) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->io->MousePos = ImVec2((float)wl_fixed_to_int(sx), (float)wl_fixed_to_int(sy));
    }

    void InputHandler::pointerLeave(void*, wl_pointer*, uint32_t, wl_surface*) {}

    void InputHandler::pointerMotion(void* data, wl_pointer*, uint32_t, wl_fixed_t sx, wl_fixed_t sy) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->io->MousePos = ImVec2((float)wl_fixed_to_int(sx), (float)wl_fixed_to_int(sy));
    }

    void InputHandler::pointerButton(void* data, wl_pointer*, uint32_t, uint32_t, uint32_t button, uint32_t state) {
        InputHandler* self = static_cast<InputHandler*>(data);

        // handle mouse state for ImGui
        bool pressed = (state == WL_POINTER_BUTTON_STATE_PRESSED);
        if (button == BTN_LEFT)
            self->io->MouseDown[0] = pressed;
        else if (button == BTN_RIGHT)
            self->io->MouseDown[1] = pressed;

        // if clicked outside the window with either, request exit
        if (pressed && (button == BTN_LEFT || button == BTN_RIGHT)) {
            float x = self->io->MousePos.x;
            float y = self->io->MousePos.y;
            if (x < 0 || x >= self->width || y < 0 || y >= self->height) {
                self->shouldExit_ = true;
            }
        }
    }

    void InputHandler::pointerAxis(void* data, wl_pointer*, uint32_t, uint32_t axis, wl_fixed_t value) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
            self->io->MouseWheel += wl_fixed_to_double(value);
        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
            self->io->MouseWheelH += wl_fixed_to_double(value);
    }

    void InputHandler::pointerFrame(void*, wl_pointer*) {}
    void InputHandler::pointerAxisSource(void*, wl_pointer*, uint32_t) {}
    void InputHandler::pointerAxisStop(void*, wl_pointer*, uint32_t, uint32_t) {}

    void InputHandler::pointerAxisDiscrete(void* data, wl_pointer*, uint32_t axis, int32_t discrete) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
            self->io->MouseWheel += (float)discrete;
        if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
            self->io->MouseWheelH += (float)discrete;
    }

    void InputHandler::keyboardKeymap(void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
        InputHandler* self = static_cast<InputHandler*>(data);

        if (!self->context) {
            close(fd);
            return;
        }

        if (format != WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1) {
            close(fd);
            return;
        }

        char* map_str = static_cast<char*>(mmap(nullptr, size, PROT_READ, MAP_SHARED, fd, 0));
        if (map_str == MAP_FAILED) {
            close(fd);
            return;
        }

        // free old keymap if it exists
        if (self->keymap) {
            xkb_keymap_unref(self->keymap);
            self->keymap = nullptr;
        }

        if (self->state) {
            xkb_state_unref(self->state);
            self->state = nullptr;
        }

        // create new keymap and state
        self->keymap =
            xkb_keymap_new_from_string(self->context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

        munmap(map_str, size);
        close(fd);

        if (!self->keymap) {
            return;
        }

        self->state = xkb_state_new(self->keymap);
        if (!self->state) {
            xkb_keymap_unref(self->keymap);
            self->keymap = nullptr;
            return;
        }

        // get modifier masks
        xkb_keycode_t minKeycode = xkb_keymap_min_keycode(self->keymap);
        xkb_keycode_t maxKeycode = xkb_keymap_max_keycode(self->keymap);

        for (xkb_keycode_t keycode = minKeycode; keycode <= maxKeycode; ++keycode) {
            xkb_layout_index_t num_layouts = xkb_keymap_num_layouts_for_key(self->keymap, keycode);
            for (xkb_layout_index_t layout = 0; layout < num_layouts; ++layout) {
                const xkb_keysym_t* syms;
                int nsyms = xkb_keymap_key_get_syms_by_level(self->keymap, keycode, layout, 0, &syms);

                for (int i = 0; i < nsyms; ++i) {
                    xkb_keysym_t sym = syms[i];
                    if (sym == XKB_KEY_Control_L || sym == XKB_KEY_Control_R) {
                        self->controlMask = 1 << xkb_keymap_mod_get_index(self->keymap, XKB_MOD_NAME_CTRL);
                    } else if (sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R) {
                        self->shiftMask = 1 << xkb_keymap_mod_get_index(self->keymap, XKB_MOD_NAME_SHIFT);
                    } else if (sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R) {
                        self->altMask = 1 << xkb_keymap_mod_get_index(self->keymap, XKB_MOD_NAME_ALT);
                    } else if (sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R) {
                        self->superMask = 1 << xkb_keymap_mod_get_index(self->keymap, XKB_MOD_NAME_LOGO);
                    }
                }
            }
        }
    }

    void InputHandler::keyboardEnter(void* data, wl_keyboard*, uint32_t, wl_surface*, wl_array* keys) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;
        self->io->AddFocusEvent(true);
    }

    void InputHandler::keyboardLeave(void* data, wl_keyboard*, uint32_t, wl_surface*) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;
        self->io->AddFocusEvent(false);
    }

    void InputHandler::keyboardKey(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;

        // key events are passed as evdev codes, which are offset by 8 from xkb codes
        xkb_keycode_t keycode = key + 8;
        bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

        // update the key state in xkb
        if (self->state) {
            xkb_state_update_key(self->state, keycode, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
        }

        // handle the key press/release
        self->handleKey(key, pressed);
    }

    void InputHandler::keyboardModifiers(void* data,
                                         wl_keyboard*,
                                         uint32_t,
                                         uint32_t modsDepressed,
                                         uint32_t modsLatched,
                                         uint32_t modsLocked,
                                         uint32_t group) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->state || !self->io)
            return;

        // update the xkb state with the new modifiers
        xkb_state_update_mask(self->state, modsDepressed, modsLatched, modsLocked, 0, 0, group);

        // update modifier key states
        bool ctrl = (modsDepressed & self->controlMask) || (modsLatched & self->controlMask) ||
                    (modsLocked & self->controlMask);
        bool shift =
            (modsDepressed & self->shiftMask) || (modsLatched & self->shiftMask) || (modsLocked & self->shiftMask);
        bool alt = (modsDepressed & self->altMask) || (modsLatched & self->altMask) || (modsLocked & self->altMask);
        bool super =
            (modsDepressed & self->superMask) || (modsLatched & self->superMask) || (modsLocked & self->superMask);

        // update modifier keys
        self->io->AddKeyEvent(ImGuiMod_Ctrl, ctrl);
        self->io->AddKeyEvent(ImGuiMod_Shift, shift);
        self->io->AddKeyEvent(ImGuiMod_Alt, alt);
        self->io->AddKeyEvent(ImGuiMod_Super, super);
    }

    void InputHandler::keyboardRepeatInfo(void* data, wl_keyboard*, int32_t rate, int32_t delay) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->repeatRate = rate;
        self->repeatDelay = delay;
    }

    void InputHandler::handleKey(uint32_t key, bool pressed) {
        if (!io)
            return;

        if (state && keymap) {
            xkb_keycode_t keycode = key + 8; // convert to xkb keycode
            xkb_keysym_t sym = xkb_state_key_get_one_sym(state, keycode);

            // map to ImGuiKey
            ImGuiKey imguiKey = ImGuiKey_None;
            for (const auto& mapping : KeyMap) {
                if (mapping.xkbKeycode == sym) {
                    imguiKey = mapping.imguiKey;
                    break;
                }
            }

            // update key state
            if (imguiKey != ImGuiKey_None) {
                io->AddKeyEvent(imguiKey, pressed);
            }

            // get the utf-8 character for text input
            if (pressed) {
                char buffer[16];
                int size = xkb_state_key_get_utf8(state, keycode, buffer, sizeof(buffer));
                if (size > 0) {
                    buffer[size] = '\0';
                    io->AddInputCharactersUTF8(buffer);
                }
            }
        }
    }

    // map xkb keysyms to ImGuiKey
    static ImGuiKey ImGui_ImplWayland_KeycodeToImGuiKey(uint32_t keycode) {
        for (const auto& mapping : KeyMap) {
            if (mapping.xkbKeycode == keycode) {
                return mapping.imguiKey;
            }
        }
        return ImGuiKey_None;
    }

} // namespace wl
