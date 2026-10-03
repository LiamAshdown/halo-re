#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Binding display names and the name to index parsers used by the bind console commands and the controls menu.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct BindingNames {
    static void chimera__axis_text(int16_t axis_index, uint8_t direction, uint16_t *out_text);
    static void chimera__button_text(int16_t button_index, uint16_t *out_text);
    static void chimera__pov_text(int16_t pov_index, int16_t direction_index, uint16_t *out_text);
    static int16_t action_name_to_index(char *name);
    static int16_t axis_direction_name_to_index(char *name);
    static void get_axis_direction_name(int16_t direction_index, uint16_t *out_name);
    static void get_binding_display_name(control_binding_descriptor *binding, uint16_t *out_text);
    static void get_keyboard_key_name(int16_t key_index, uint16_t *out_name);
    static void get_mouse_axis_name(int16_t axis_index, uint8_t direction, uint16_t *out_name);
    static void get_mouse_button_name(int16_t button_index, uint16_t *out_name);
    static int16_t joystick_pov_direction_name_to_index(char *name);
    static uint32_t keyboard_key_name_to_index(char *name);
    static uint32_t mouse_axis_name_to_index(char *name, uint8_t *out_direction);
    static uint32_t mouse_button_name_to_index(char *name);
    static int16_t joystick_axis_name_to_index(char *name, uint8_t *out_direction);
    static int16_t joystick_button_name_to_index(char *name);
    static int16_t joystick_pov_name_to_index(char *name, int16_t *out_direction);
    static void print_bound_controls(void);
};

}
