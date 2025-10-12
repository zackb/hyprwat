#include "input.hpp"

extern "C" {
#include <linux/input-event-codes.h>
}
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <xkbcommon/xkbcommon-keysyms.h>

// For keycode to ImGuiKey mapping
static ImGuiKey ImGui_ImplWayland_KeycodeToImGuiKey(uint32_t keycode);

// map xkb keycode to ImGuiKey
static const struct {
    xkb_keycode_t xkb_keycode;
    ImGuiKey imgui_key;
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
    {XKB_KEY_A, ImGuiKey_A},
    {XKB_KEY_B, ImGuiKey_B},
    {XKB_KEY_C, ImGuiKey_C},
    {XKB_KEY_D, ImGuiKey_D},
    {XKB_KEY_E, ImGuiKey_E},
    {XKB_KEY_F, ImGuiKey_F},
    {XKB_KEY_G, ImGuiKey_G},
    {XKB_KEY_H, ImGuiKey_H},
    {XKB_KEY_I, ImGuiKey_I},
    {XKB_KEY_J, ImGuiKey_J},
    {XKB_KEY_K, ImGuiKey_K},
    {XKB_KEY_L, ImGuiKey_L},
    {XKB_KEY_M, ImGuiKey_M},
    {XKB_KEY_N, ImGuiKey_N},
    {XKB_KEY_O, ImGuiKey_O},
    {XKB_KEY_P, ImGuiKey_P},
    {XKB_KEY_Q, ImGuiKey_Q},
    {XKB_KEY_R, ImGuiKey_R},
    {XKB_KEY_S, ImGuiKey_S},
    {XKB_KEY_T, ImGuiKey_T},
    {XKB_KEY_U, ImGuiKey_U},
    {XKB_KEY_V, ImGuiKey_V},
    {XKB_KEY_W, ImGuiKey_W},
    {XKB_KEY_X, ImGuiKey_X},
    {XKB_KEY_Y, ImGuiKey_Y},
    {XKB_KEY_Z, ImGuiKey_Z},
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
        wl_seat_add_listener(seat, &seat_listener, this);
        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    }

    InputHandler::InputHandler(wl_seat* seat, ImGuiIO* io) : seat(seat), io(io) {
        wl_seat_add_listener(seat, &seat_listener, this);
        xkb_context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
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
        if (m_xkb_state) {
            xkb_state_unref(m_xkb_state);
            m_xkb_state = nullptr;
        }
        if (xkb_keymap) {
            xkb_keymap_unref(xkb_keymap);
            xkb_keymap = nullptr;
        }
        if (xkb_context) {
            xkb_context_unref(xkb_context);
            xkb_context = nullptr;
        }
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
                self->should_exit = true;
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

    void InputHandler::keyboard_keymap(void* data, wl_keyboard*, uint32_t format, int32_t fd, uint32_t size) {
        InputHandler* self = static_cast<InputHandler*>(data);

        if (!self->xkb_context) {
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

        // Free old keymap if it exists
        if (self->xkb_keymap) {
            xkb_keymap_unref(self->xkb_keymap);
            self->xkb_keymap = nullptr;
        }

        if (self->m_xkb_state) {
            xkb_state_unref(self->m_xkb_state);
            self->m_xkb_state = nullptr;
        }

        // Create new keymap and state
        self->xkb_keymap = xkb_keymap_new_from_string(
            self->xkb_context, map_str, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);

        munmap(map_str, size);
        close(fd);

        if (!self->xkb_keymap) {
            return;
        }

        self->m_xkb_state = xkb_state_new(self->xkb_keymap);
        if (!self->m_xkb_state) {
            xkb_keymap_unref(self->xkb_keymap);
            self->xkb_keymap = nullptr;
            return;
        }

        // Get modifier masks
        xkb_keycode_t min_keycode = xkb_keymap_min_keycode(self->xkb_keymap);
        xkb_keycode_t max_keycode = xkb_keymap_max_keycode(self->xkb_keymap);

        for (xkb_keycode_t keycode = min_keycode; keycode <= max_keycode; ++keycode) {
            xkb_layout_index_t num_layouts = xkb_keymap_num_layouts_for_key(self->xkb_keymap, keycode);
            for (xkb_layout_index_t layout = 0; layout < num_layouts; ++layout) {
                const xkb_keysym_t* syms;
                int nsyms = xkb_keymap_key_get_syms_by_level(self->xkb_keymap, keycode, layout, 0, &syms);

                for (int i = 0; i < nsyms; ++i) {
                    xkb_keysym_t sym = syms[i];
                    if (sym == XKB_KEY_Control_L || sym == XKB_KEY_Control_R) {
                        self->control_mask = 1 << xkb_keymap_mod_get_index(self->xkb_keymap, XKB_MOD_NAME_CTRL);
                    } else if (sym == XKB_KEY_Shift_L || sym == XKB_KEY_Shift_R) {
                        self->shift_mask = 1 << xkb_keymap_mod_get_index(self->xkb_keymap, XKB_MOD_NAME_SHIFT);
                    } else if (sym == XKB_KEY_Alt_L || sym == XKB_KEY_Alt_R) {
                        self->alt_mask = 1 << xkb_keymap_mod_get_index(self->xkb_keymap, XKB_MOD_NAME_ALT);
                    } else if (sym == XKB_KEY_Super_L || sym == XKB_KEY_Super_R) {
                        self->super_mask = 1 << xkb_keymap_mod_get_index(self->xkb_keymap, XKB_MOD_NAME_LOGO);
                    }
                }
            }
        }
    }

    void InputHandler::keyboard_enter(void* data, wl_keyboard*, uint32_t, wl_surface*, wl_array* keys) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;
        self->io->AddFocusEvent(true);
    }

    void InputHandler::keyboard_leave(void* data, wl_keyboard*, uint32_t, wl_surface*) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;
        self->io->AddFocusEvent(false);
    }

    void InputHandler::keyboard_key(void* data, wl_keyboard*, uint32_t, uint32_t, uint32_t key, uint32_t state) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->io)
            return;

        // Key events are passed as evdev codes, which are offset by 8 from XKB codes
        xkb_keycode_t keycode = key + 8;
        bool pressed = (state == WL_KEYBOARD_KEY_STATE_PRESSED);

        // Update the key state in XKB
        if (self->m_xkb_state) {
            xkb_state_update_key(self->m_xkb_state, keycode, pressed ? XKB_KEY_DOWN : XKB_KEY_UP);
        }

        // Handle the key press/release
        self->handleKey(key, pressed);
    }

    void InputHandler::keyboard_modifiers(void* data,
                                          wl_keyboard*,
                                          uint32_t,
                                          uint32_t mods_depressed,
                                          uint32_t mods_latched,
                                          uint32_t mods_locked,
                                          uint32_t group) {
        InputHandler* self = static_cast<InputHandler*>(data);
        if (!self->m_xkb_state || !self->io)
            return;

        // Update the XKB state with the new modifiers
        xkb_state_update_mask(self->m_xkb_state, mods_depressed, mods_latched, mods_locked, 0, 0, group);

        // Update modifier key states
        bool ctrl = (mods_depressed & self->control_mask) || (mods_latched & self->control_mask) ||
                    (mods_locked & self->control_mask);
        bool shift = (mods_depressed & self->shift_mask) || (mods_latched & self->shift_mask) ||
                     (mods_locked & self->shift_mask);
        bool alt =
            (mods_depressed & self->alt_mask) || (mods_latched & self->alt_mask) || (mods_locked & self->alt_mask);
        bool super = (mods_depressed & self->super_mask) || (mods_latched & self->super_mask) ||
                     (mods_locked & self->super_mask);

        // Update modifier keys
        self->io->AddKeyEvent(ImGuiMod_Ctrl, ctrl);
        self->io->AddKeyEvent(ImGuiMod_Shift, shift);
        self->io->AddKeyEvent(ImGuiMod_Alt, alt);
        self->io->AddKeyEvent(ImGuiMod_Super, super);
    }

    void InputHandler::keyboard_repeat_info(void* data, wl_keyboard*, int32_t rate, int32_t delay) {
        InputHandler* self = static_cast<InputHandler*>(data);
        self->repeat_rate = rate;
        self->repeat_delay = delay;
    }

    void InputHandler::handleKey(uint32_t key, bool pressed) {
        if (!io)
            return;

        if (m_xkb_state && xkb_keymap) {
            xkb_keycode_t keycode = key + 8; // Convert to XKB keycode
            xkb_keysym_t sym = xkb_state_key_get_one_sym(m_xkb_state, keycode);

            // Map to ImGuiKey
            ImGuiKey imgui_key = ImGuiKey_None;
            for (const auto& mapping : KeyMap) {
                if (mapping.xkb_keycode == sym) {
                    imgui_key = mapping.imgui_key;
                    break;
                }
            }

            // Update key state
            if (imgui_key != ImGuiKey_None) {
                io->AddKeyEvent(imgui_key, pressed);
            }

            // Get the UTF-8 character for text input
            if (pressed) {
                char buffer[16];
                int size = xkb_state_key_get_utf8(m_xkb_state, keycode, buffer, sizeof(buffer));
                if (size > 0) {
                    buffer[size] = '\0';
                    io->AddInputCharactersUTF8(buffer);
                }
            }
        }
    }
    // Helper function to map XKB keysyms to ImGuiKey
    static ImGuiKey ImGui_ImplWayland_KeycodeToImGuiKey(uint32_t keycode) {
        for (const auto& mapping : KeyMap) {
            if (mapping.xkb_keycode == keycode) {
                return mapping.imgui_key;
            }
        }
        return ImGuiKey_None;
    }

} // namespace wl
