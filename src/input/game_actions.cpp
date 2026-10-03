/**
 * Per-frame translation of bound keys, buttons and axes into the local player input state.
 */

#include "halo/core/crt.hpp"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>
#include "cache.h"
#include "objects.h"
#include "units.h"

#include "halo/input/game_actions.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/objects/api.hpp"
#include "halo/input/state.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/core/datum.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/directinput.hpp"
#include "halo/input/system.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/game/api.hpp"
#include "halo/units/records.hpp"
#include "halo/units/api.hpp"

namespace halo::input {

/**
 * Accumulates a digitally-pressed control's contribution into the corresponding movement/look
 * axis of *state (throttle_x for forward/backward, throttle_y for left/right, look_x for
 * look_left/look_right, look_y for look_up/look_down), scaled by the matching per-tick rate in
 * *settings, clamped to [-1, 1]. Does nothing for any other action id.
 *
 * @address 0x48ca10
 */
void GameActions::accumulate_axis_value(player_control_settings *settings, local_player_input_state *state, int16_t action)
{
    const float *rates = &settings->forward_rate;
    float value;
    float *axis;

    switch (action) {
    case _input_action_forward:
        axis = &state->throttle_x;
        value = *axis + rates[0];
        break;
    case _input_action_backward:
        axis = &state->throttle_x;
        value = *axis - rates[0];
        break;
    case _input_action_left:
        axis = &state->throttle_y;
        value = *axis + rates[1];
        break;
    case _input_action_right:
        axis = &state->throttle_y;
        value = *axis - rates[1];
        break;
    case _input_action_look_up:
        axis = &state->look_y;
        value = *axis + rates[3];
        break;
    case _input_action_look_down:
        axis = &state->look_y;
        value = *axis - rates[3];
        break;
    case _input_action_look_left:
        axis = &state->look_x;
        value = *axis + rates[2];
        break;
    case _input_action_look_right:
        axis = &state->look_x;
        value = *axis - rates[2];
        break;
    default:
        return;
    }

    if (value < -1.0f) {
        *axis = -1.0f;
    } else if (1.0f < value) {
        *axis = 1.0f;
    } else {
        *axis = value;
    }
}

}

namespace halo::input {

/**
 * True when current's four axes are all within 0.1 of previous's and none of current's 19
 * digital-button hold counts are nonzero.
 *
 * Original register convention: current in ECX, previous in EDX.
 *
 * @address 0x48fce0
 */
uint8_t GameActions::accumulator_is_idle(local_player_input_state *current, local_player_input_state *previous)
{
    uint8_t idle;
    int32_t i;

    idle = 1;

    if (!(fabs((double)(current->throttle_x - previous->throttle_x)) < (double)0.1f) ||
        !(fabs((double)(current->throttle_y - previous->throttle_y)) < (double)0.1f) ||
        !(fabs((double)(current->look_y - previous->look_y)) < (double)0.1f) ||
        !(fabs((double)(current->look_x - previous->look_x)) < (double)0.1f)) {
        idle = 0;
    }

    for (i = 0; i < k_input_digital_action_count; i++) {
        idle = (idle && current->buttons[i] == 0) ? 1 : 0;
    }
    return idle;
}

}

namespace halo::input {

/**
 * Clamps a floating-point input value to the 0..1 range.
 *
 * @address 0x48c8a0
 */
float GameActions::clamp_unit_float(float value)
{
    if (value < 0.0f) {
        return 0.0f;
    }
    if (1.0f < value) {
        return 1.0f;
    }
    return value;
}

}

static void input_throttle_add_clamped(float *axis, float step)
{
    float value = *axis + step;

    if (value < -1.0f) {
        *axis = -1.0f;
    } else if (1.0f < value) {
        *axis = 1.0f;
    } else {
        *axis = value;
    }
}

namespace halo::input {

/**
 * The per-frame game action update. Rebuilds local player 0's action state (19 digital hold
 * counts, two throttle axes, two look axes) from every bound keyboard key, mouse button, mouse
 * axis and, per controller slot, gamepad button, axis and POV hat; remembers which input last
 * drove each action; clears the frame's idle flag when the result changed; and applies look
 * inversion.
 *
 * @address 0x48cca0
 */
void GameActions::game_action_update(void)
{
    player_control_settings *settings = &input_state().input_globals.settings[0];
    local_player_input_state *state = &input_state().input_globals.states[0];
    uint8_t *buttons = (uint8_t *)state->buttons;
    const float *digital_rates = &settings->forward_rate;

    const float *gamepad_axis_scales = &settings->gamepad_axis_scale_x;
    local_player_input_state previous;
    joystick_state *slot_states[4];
    mouse_state *mouse;

    struct {
        int32_t axis_direction;
        int32_t mouse_delta[3];
    } frame;
    uint8_t *pov_applied = (uint8_t *)frame.mouse_delta;
    int32_t slot;
    int32_t index;
    int16_t action;
    uint8_t frames;
    int32_t mouse_direction = 0;

    memcpy(&previous, state, sizeof(previous));

    mouse = (mouse_state *)0;
    if (input_state().mouse_device != (void *)0) {
        mouse = (globals().suppressed != 0) ? &input_state().mouse_neutral_state : &input_state().live_mouse_state;
    }

    input_state().look_yaw_rate_setting[0] = settings->look_rate_80;
    input_state().look_pitch_rate_setting[0] = settings->look_rate_40;

    for (slot = 0; slot < 4; slot++) {
        slot_states[slot] = (joystick_state *)0;
        if (globals().joystick_slot_devices[slot] != -1) {
            slot_states[slot] = (globals().suppressed != 0) ? &input_state().joystick_neutral_state : &input_state().joystick_states[slot];
        }
    }

    frame.axis_direction = 0;
    frame.mouse_delta[0] = mouse->x;
    frame.mouse_delta[1] = mouse->y;
    frame.mouse_delta[2] = mouse->wheel;

    memset(state->buttons, 0, sizeof(state->buttons));
    state->look_is_analog = 0;
    state->look_y = 0.0f;
    state->look_x = 0.0f;
    state->throttle_x = 0.0f;
    state->throttle_y = 0.0f;

    for (index = 0; index < k_control_keyboard_key_count; index++) {
        action = input_state().keyboard_bindings[index];
        if (action == k_input_unbound) {
        } else if (action >= _input_action_forward && action <= _input_action_look_right) {
            if (halo::input::input_get_key_state((int16_t)index) != 0) {
                halo::input::GameActions::accumulate_axis_value(settings, state, action);
            }
        } else {
            frames = halo::input::input_get_key_state((int16_t)index);
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
        }
        if (halo::input::input_get_key_state((int16_t)index) != 0 && action != k_input_unbound) {
            halo::input::Bindings::last_used_binding_set(action, _control_device_keyboard, 0, _control_input_button, (int16_t)index, 0);
            input_state().last_input_device = 0;
        }
    }

    for (index = 0; index < k_control_mouse_button_count; index++) {
        action = input_state().mouse_button_bindings[index];
        frames = 0;
        if (input_state().mouse_device != (void *)0 && globals().suppressed == 0) {
            frames = input_state().live_mouse_state.button_frames[index];
        }
        if (action == k_input_unbound) {
        } else if (action >= _input_action_forward && action <= _input_action_look_right) {
            if (input_state().mouse_device == (void *)0) {
                continue;
            }
            if (frames != 0) {
                halo::input::GameActions::accumulate_axis_value(settings, state, action);
            }
        } else {
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
        }
        if (input_state().mouse_device != (void *)0 && globals().suppressed == 0 &&
            input_state().live_mouse_state.button_frames[index] != 0 && action != k_input_unbound) {
            halo::input::Bindings::last_used_binding_set(action, _control_device_mouse, 0, _control_input_button, (int16_t)index, 0);
            input_state().last_input_device = 0;
        }
    }

    for (index = 0; index < k_input_mouse_axis_count; index++) {
        int32_t delta = frame.mouse_delta[index];
        float step;

        if (delta == 0) {
            input_state().mouse_axis_frames[index][0] = 0;
            input_state().mouse_axis_frames[index][1] = 0;
            continue;
        }
        if (delta < 0) {
            action = input_state().mouse_axis_bindings[index][1];
            frames = (uint8_t)(input_state().mouse_axis_frames[index][1] + 1);
            input_state().mouse_axis_frames[index][0] = 0;
            input_state().mouse_axis_frames[index][1] = frames;
            mouse_direction = 2;
        } else {
            action = input_state().mouse_axis_bindings[index][0];
            frames = (uint8_t)(input_state().mouse_axis_frames[index][0] + 1);
            input_state().mouse_axis_frames[index][0] = frames;
            input_state().mouse_axis_frames[index][1] = 0;
            mouse_direction = 1;
        }

        switch (action) {
        case k_input_unbound:
            break;
        case _input_action_forward:
        case _input_action_backward:
            step = (float)delta / digital_rates[4];
            if (step < 0.0f) {
                step = -step;
            }
            input_throttle_add_clamped(&state->throttle_x, action == _input_action_forward ? step : -step);
            break;
        case _input_action_left:
        case _input_action_right:
            step = (float)delta / digital_rates[5];
            if (step < 0.0f) {
                step = -step;
            }
            input_throttle_add_clamped(&state->throttle_y, action == _input_action_left ? step : -step);
            break;
        case _input_action_look_up:
            state->look_is_analog = 0;
            state->look_y = halo::input::GameActions::mouse_acceleration_evaluate(settings->mouse_look_y_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_down:
            state->look_is_analog = 0;
            state->look_y = -halo::input::GameActions::mouse_acceleration_evaluate(settings->mouse_look_y_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_left:
            state->look_is_analog = 0;
            state->look_x = halo::input::GameActions::mouse_acceleration_evaluate(settings->mouse_look_x_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_right:
            state->look_is_analog = 0;
            state->look_x = -halo::input::GameActions::mouse_acceleration_evaluate(settings->mouse_look_x_sensitivity, delta < 0 ? -delta : delta);
            break;
        default:
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
            break;
        }
        if (action != k_input_unbound) {
            halo::input::Bindings::last_used_binding_set(action, _control_device_mouse, 0, _control_input_axis, (int16_t)index, (int16_t)mouse_direction);
            input_state().last_input_device = 0;
        }
    }

    for (slot = 0; slot < 4; slot++) {
        int32_t device = globals().joystick_slot_devices[slot];
        joystick_state *pad = slot_states[slot];
        int32_t button_count = 0;
        int32_t axis_count = 0;
        int32_t pov_count = 0;
        int16_t accept_button;
        int16_t back_button;

        if (device != -1) {
            button_count = input_state().input_devices[device].button_count;
            axis_count = input_state().input_devices[device].axis_count;
            pov_count = input_state().input_devices[device].pov_count;
        }

        accept_button = input_state().gamepad_action_buttons[slot][0];
        if (buttons[_input_action_accept] == 0 && accept_button != -1) {
            buttons[_input_action_accept] = (device != -1) ? pad->button_frames[accept_button] : 0;
        }
        back_button = input_state().gamepad_action_buttons[slot][1];
        if (buttons[_input_action_back] == 0 && back_button != -1) {
            buttons[_input_action_back] = (device != -1) ? pad->button_frames[back_button] : 0;
        }

        for (index = 0; index < button_count; index++) {
            action = input_state().gamepad_button_bindings[slot][index];
            frames = (device != -1) ? pad->button_frames[index] : 0;
            if (action == k_input_unbound || action == _input_action_accept || action == _input_action_back) {
            } else if (action >= _input_action_forward && action <= _input_action_look_right) {
                if (frames != 0) {
                    halo::input::GameActions::accumulate_axis_value(settings, state, action);
                }
            } else {
                if (buttons[action] > frames) {
                    frames = buttons[action];
                }
                buttons[action] = frames;
            }
            if (device != -1 && pad->button_frames[index] != 0 && action != k_input_unbound) {
                halo::input::Bindings::last_used_binding_set(action, _control_device_gamepad, (int16_t)slot, _control_input_button, (int16_t)index, 0);
                input_state().last_input_device = slot + 1;
            }
        }

        for (index = 0; index < axis_count; index++) {
            int32_t value = pad->axes[index];
            int32_t magnitude = value < 0 ? -value : value;
            float inverse_scale;

            if (value == 0) {
                input_state().joystick_axis_frames[slot][index][0] = 0;
                input_state().joystick_axis_frames[slot][index][1] = 0;
                continue;
            }
            if (value < 0) {
                input_state().joystick_axis_frames[slot][index][1]++;
                action = input_state().gamepad_axis_bindings[slot][index][1];
                input_state().joystick_axis_frames[slot][index][0] = 0;
                frames = input_state().joystick_axis_frames[slot][index][1];
                frame.axis_direction = 2;
            } else {
                frames = (uint8_t)(input_state().joystick_axis_frames[slot][index][0] + 1);
                action = input_state().gamepad_axis_bindings[slot][index][0];
                input_state().joystick_axis_frames[slot][index][0] = frames;
                input_state().joystick_axis_frames[slot][index][1] = 0;
                frame.axis_direction = 1;
            }

            switch (action) {
            case k_input_unbound:
            case _input_action_accept:
            case _input_action_back:
                break;
            case _input_action_forward:
            case _input_action_backward:
                inverse_scale = 1.0f / gamepad_axis_scales[0];
                input_throttle_add_clamped(&state->throttle_x,
                    action == _input_action_forward ? (float)magnitude * 0.000244140625f * inverse_scale
                                                    : -((float)magnitude * 0.000244140625f * inverse_scale));
                break;
            case _input_action_left:
            case _input_action_right:
                inverse_scale = 1.0f / gamepad_axis_scales[1];
                input_throttle_add_clamped(&state->throttle_y,
                    action == _input_action_left ? (float)magnitude * 0.000244140625f * inverse_scale
                                                 : -((float)magnitude * 0.000244140625f * inverse_scale));
                break;
            case _input_action_look_up:
                state->look_is_analog = 1;
                input_state().look_pitch_rate_setting[0] = settings->gamepad_rate_40[slot];
                state->look_y = (float)magnitude * 0.000244140625f;
                break;
            case _input_action_look_down:
                state->look_is_analog = 1;
                input_state().look_pitch_rate_setting[0] = settings->gamepad_rate_40[slot];
                state->look_y = -((float)magnitude * 0.000244140625f);
                break;
            case _input_action_look_left:
                state->look_is_analog = 1;
                input_state().look_yaw_rate_setting[0] = settings->gamepad_rate_80[slot];
                state->look_x = (float)magnitude * 0.000244140625f;
                break;
            case _input_action_look_right:
                state->look_is_analog = 1;
                input_state().look_yaw_rate_setting[0] = settings->gamepad_rate_80[slot];
                state->look_x = -((float)magnitude * 0.000244140625f);
                break;
            default:
                if (buttons[action] > frames) {
                    frames = buttons[action];
                }
                buttons[action] = frames;
                break;
            }

            if (pad->axes[index] != 0 && action != k_input_unbound) {
                halo::input::Bindings::last_used_binding_set(action, _control_device_gamepad, (int16_t)slot, _control_input_axis, (int16_t)index, (int16_t)frame.axis_direction);
                input_state().last_input_device = slot + 1;
            }
        }

        memset(pov_applied, 0, 8);
        for (index = 0; index < pov_count; index++) {
            int32_t octant = pad->povs[index];
            int32_t count;
            int16_t *octant_bindings = input_state().gamepad_pov_bindings[slot][index];

            if (octant == -1) {
                memset(input_state().joystick_pov_frames[slot][index], 0, 8);
                continue;
            }
            count = input_state().joystick_pov_frames[slot][index][octant] + 1;
            memset(input_state().joystick_pov_frames[slot][index], 0, 8);
            input_state().joystick_pov_frames[slot][index][octant] = (uint8_t)count;
            action = octant_bindings[octant];

            if (action == k_input_unbound) {
                int16_t next_action = (octant == 7) ? octant_bindings[0] : octant_bindings[octant + 1];
                int16_t previous_action = (octant == 0) ? octant_bindings[7] : octant_bindings[octant - 1];

                if (next_action >= 0x13 && next_action <= 0x1b &&
                    previous_action >= 0x13 && previous_action <= 0x1b) {
                    if (pov_applied[octant + 1] == 0) {
                        halo::input::GameActions::accumulate_axis_value(settings, state, next_action);
                        pov_applied[octant + 1] = 1;
                        halo::input::Bindings::last_used_binding_set(next_action, _control_device_gamepad, (int16_t)slot, _control_input_pov, (int16_t)index, octant + 1);
                    }
                    if (pov_applied[octant - 1] == 0) {
                        halo::input::GameActions::accumulate_axis_value(settings, state, previous_action);
                        pov_applied[octant - 1] = 1;
                        halo::input::Bindings::last_used_binding_set(previous_action, _control_device_gamepad, (int16_t)slot, _control_input_pov, (int16_t)index, octant - 1);
                    }
                }
                continue;
            }
            if (action >= _input_action_forward && action <= _input_action_look_right) {
                halo::input::GameActions::accumulate_axis_value(settings, state, action);
                pov_applied[octant] = 1;
            } else if (action != _input_action_accept && action != _input_action_back) {
                if (count < (int32_t)buttons[action]) {
                    count = buttons[action];
                }
                buttons[action] = (uint8_t)count;
            }
            halo::input::Bindings::last_used_binding_set(action, _control_device_gamepad, (int16_t)slot, _control_input_pov, (int16_t)index, octant);
            input_state().last_input_device = slot + 1;
        }
    }

    if (buttons[_input_action_zoom] != 1) {
        buttons[_input_action_zoom] = 0;
    }
    if (buttons[_input_action_switch_grenade] != 1) {
        buttons[_input_action_switch_grenade] = 0;
    }
    if (buttons[_input_action_switch_weapon] != 1) {
        buttons[_input_action_switch_weapon] = 0;
    }

    if (!halo::input::GameActions::accumulator_is_idle(state, &previous)) {
        input_state().input_globals.idle = 0;
    }

    if (settings->look_inverted != 0 ||
        (settings->look_inverted_driving != 0 && halo::input::GameActions::should_invert_look(0))) {
        state->look_y = -state->look_y;
    }
}

}

namespace halo::input {

/**
 * Sets input_abstraction_globals.settings[slot]'s gamepad X-axis sensitivity/deadzone scale
 * (gamepad_axis_scale_x, 0x00710b58 + slot*0x85c), clamped to 0..1. Does nothing if slot is outside
 * 0..k_control_gamepad_count-1.
 *
 * @address 0x48c930
 */
void GameActions::joystick_set_axis_scale_x(int16_t slot, float value)
{
    float *scale;

    if (slot >= 0 && slot < k_control_gamepad_count) {
        scale = &input_state().input_globals.settings[slot].gamepad_axis_scale_x;
        if (value < 0.0f) {
            *scale = 0.0f;
        } else if (1.0f < value) {
            *scale = 1.0f;
        } else {
            *scale = value;
        }
    }
}

}

namespace halo::input {

/**
 * Sets input_abstraction_globals.settings[slot]'s gamepad Y-axis sensitivity/deadzone scale
 * (gamepad_axis_scale_y, 0x00710b5c + slot*0x85c), clamped to 0..1. Does nothing if slot is outside
 * 0..k_control_gamepad_count-1.
 *
 * @address 0x48c9a0
 */
void GameActions::joystick_set_axis_scale_y(int16_t slot, float value)
{
    float *scale;

    if (slot >= 0 && slot < k_control_gamepad_count) {
        scale = &input_state().input_globals.settings[slot].gamepad_axis_scale_y;
        if (value < 0.0f) {
            *scale = 0.0f;
        } else if (1.0f < value) {
            *scale = 1.0f;
        } else {
            *scale = value;
        }
    }
}

}

namespace halo::input {

/**
 * Evaluates the mouse-sensitivity/acceleration response curve for raw movement magnitude
 * `magnitude`, given `sensitivity` (typically settings.mouse_look_x/y_sensitivity). Rebuilds
 * mouse_acceleration_points from mouse_acceleration_defaults whenever the (0..1 clamped, and
 * clamped in place) mouse_acceleration setting has changed since the last call. Returns 0 for a
 * zero magnitude or one at or beyond the last point's magnitude (in practice unreachable, since
 * the last default point's magnitude is a 10000 sentinel).
 *
 * @address 0x48cb60
 */
float GameActions::mouse_acceleration_evaluate(float sensitivity, int32_t magnitude)
{
    int32_t i;
    float fraction;
    float rate;

    if (magnitude == 0) {
        return 0.0f;
    }

    if (input_state().mouse_acceleration != input_state().mouse_acceleration_cached) {
        if (input_state().mouse_acceleration < 0.0f) {
            input_state().mouse_acceleration = 0.0f;
        } else if (1.0f < input_state().mouse_acceleration) {
            input_state().mouse_acceleration = 1.0f;
        }
        input_state().mouse_acceleration_cached = input_state().mouse_acceleration;

        memcpy(input_state().mouse_acceleration_points, input_state().mouse_acceleration_defaults, sizeof(input_state().mouse_acceleration_points));
        for (i = 0; i < k_input_mouse_acceleration_point_count; i++) {
            input_state().mouse_acceleration_points[i].magnitude = (int32_t)(
                (float)input_state().mouse_acceleration_defaults[i].magnitude_slow -
                (float)(input_state().mouse_acceleration_defaults[i].magnitude_slow - input_state().mouse_acceleration_defaults[i].magnitude) *
                    input_state().mouse_acceleration_cached);
        }
    }

    for (i = 1; i < k_input_mouse_acceleration_point_count; i++) {
        if (magnitude <= input_state().mouse_acceleration_points[i].magnitude) {
            break;
        }
    }
    if (i >= k_input_mouse_acceleration_point_count) {
        return 0.0f;
    }

    fraction = (float)(magnitude - input_state().mouse_acceleration_points[i - 1].magnitude) /
               (float)(input_state().mouse_acceleration_points[i].magnitude - input_state().mouse_acceleration_points[i - 1].magnitude);
    rate = input_state().mouse_acceleration_points[i - 1].rate +
           (input_state().mouse_acceleration_points[i].rate - input_state().mouse_acceleration_points[i - 1].rate) * fraction;

    return (sensitivity * input_state().mouse_acceleration_points[i].boost + 1.0f) * rate * (float)magnitude;
}

}

namespace halo::input {

/**
 * Zeroes the keyboard key-frame and release-pending arrays, the first 16 slots of the key event
 * ring (and its read index / count), the live mouse state, and reseeds every joystick slot's
 * state from the (all-zero) joystick_neutral_state.
 *
 * @address 0x490aa0
 */
void GameActions::reset_state_and_axis_configs(void)
{
    int32_t i;

    memset(input_state().key_frames, 0, sizeof(input_state().key_frames));
    input_state().key_event_read_index = 0;
    input_state().key_event_count = 0;
    memset(input_state().key_events, 0, sizeof(ui_key_event) * 0x10);
    memset(input_state().key_release_pending, 0, sizeof(input_state().key_release_pending));
    memset(&input_state().live_mouse_state, 0, sizeof(input_state().live_mouse_state));

    for (i = 0; i < 4; i++) {
        input_state().joystick_states[i] = input_state().joystick_neutral_state;
    }
}

}

namespace halo::input {

/**
 * Converts a raw sensitivity value (clamped to 0.001 .. 100.0) into a radians-per-unit turn-rate
 * scale factor by multiplying it by 2*pi/1000.
 *
 * @address 0x48c8e0
 */
float GameActions::sensitivity_to_turn_rate(float sensitivity)
{
    if (sensitivity < 0.001f) {
        return 0.001f * 0.006283186f;
    }
    if (100.0f < sensitivity) {
        return 100.0f * 0.006283186f;
    }
    return sensitivity * 0.006283186f;
}

}

namespace halo::input {

/**
 * True when local_player_index's controlled unit is seated (as a driver) in a parent vehicle
 * whose type is vehicletype_human_plane or vehicletype_alien_fighter.
 *
 * Original register convention: local_player_index in EAX.
 *
 * @address 0x48fd60
 */
uint8_t GameActions::should_invert_look(int16_t local_player_index)
{
    datum_index player_handle;
    void *player_record;
    object *unit_object;
    unit_data *unit;
    object_header *parent_header;
    object *parent_object;
    Vehicle *parent_tag;
    UnitSeat *seat;

    if (local_player_index == -1 || local_player_index >= 1) {
        return 0;
    }
    player_handle = input_state().local_player_globals->local_players[local_player_index];
    if (player_handle == halo::k_dword_none) {
        return 0;
    }
    player_record = halo::memory::datum_get(player_handle, input_state().player_data);
    if (player_record == (void *)0) {
        return 0;
    }

    unit_object = halo::objects::object_try_and_get(((player *)player_record)->unit, 3);
    if (unit_object == (object *)0) {
        return 0;
    }
    if (unit_object->parent_object == halo::k_dword_none) {
        return 0;
    }
    unit = halo::units::unit_data_of(unit_object);
    if (unit->vehicle_seat_index == -1) {
        return 0;
    }

    parent_header = &((object_header *)halo::objects::globals().object_data->data)[(uint16_t)unit_object->parent_object];
    parent_object = parent_header->data;
    parent_tag = (Vehicle *)halo::cache::globals().tag_instances[(uint16_t)parent_object->definition_tag].data;

    if (parent_tag->vehicle_type == 3 || parent_tag->vehicle_type == 5) {
        seat = (UnitSeat *)((uint8_t *)parent_tag->base.seats.pointer +
                             (uint32_t)unit->vehicle_seat_index * 0x11c);
        if ((seat->flags & 4) != 0) {
            return 1;
        }
    }
    return 0;
}

}
