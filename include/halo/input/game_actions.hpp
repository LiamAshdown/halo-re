#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::input {

/**
 * Per-frame translation of bound keys, buttons and axes into the local player input state.
 * Stateless service class: the functions are static members, the state they act on lives in the engine globals.
 */
struct GameActions {
    static void accumulate_axis_value(player_control_settings *settings, local_player_input_state *state, int16_t action);
    static uint8_t accumulator_is_idle(local_player_input_state *current, local_player_input_state *previous);
    static float clamp_unit_float(float value);
    static void game_action_update(void);
    static void joystick_set_axis_scale_x(int16_t slot, float value);
    static void joystick_set_axis_scale_y(int16_t slot, float value);
    static float mouse_acceleration_evaluate(float sensitivity, int32_t magnitude);
    static void reset_state_and_axis_configs(void);
    static float sensitivity_to_turn_rate(float sensitivity);
    static uint8_t should_invert_look(int16_t local_player_index);
};

}
