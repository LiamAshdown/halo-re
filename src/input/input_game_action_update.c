// input_game_action_update  (Ghidra: FUN_0048cca0; renamed per its behavior)
// address 0x48cca0, size 8112 bytes (0x48cca0..0x48ec4f; 1481 bytes as Ghidra split it); the real body runs on through the
//   region Ghidra mis-split off as "hs_return" at 0x48d270 (0x48d270 is the `test ah,0x5` of
//   the mouse-button forward case, mid-instruction-stream) and ends with the `ret` at
//   0x48ea4f; the jump tables follow at 0x48eb9c..0x48ec4b, so 0x48cca0..0x48ec4f is one
//   function (about 0x1fb0 bytes). 0x48d270 is therefore not written separately.
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: the only caller is input_update_tick 0x48b4b0 (mode_flags == game, after the menu
//   exit deadline). Every global is a field of types/input.h input_abstraction_globals or of
//   the DirectInput block: the keyboard / mouse / gamepad binding tables of settings[0]
//   (0x00710330, 0x0071040a, 0x0071041a, 0x00710426, 0x00710526, 0x00710536, 0x00710736),
//   the float block +0x810.. (0x00710b38..0x00710b81), states[0] (0x00712498..0x007124bc),
//   last_used_bindings (0x007127d4, 12-byte records written inline), idle (0x00712540), and
//   the frame-count arrays 0x0071291c / 0x00712928 / 0x00712a28 listed in the header.
//   Jump tables decoded from the image: keyboard 0x48eb9c and mouse buttons 0x48ebbc (actions
//   0x13..0x1a, everything else to the default max path), mouse axes 0x48ebdc, gamepad
//   buttons 0x48ebfc/0x48ec14 and gamepad axes 0x48ec24/0x48ec3c (byte index table: actions
//   8 and 9 -> "do nothing", 0x13..0x16 -> throttle cases, 0x0a..0x12 -> default max).
//   Constants: 0x00672ba8 = -1.0f, 0x00672ac4 = 1.0f, 0x00672ac0 = 0.0f,
//   0x00672db0 = 0.000244140625f (1/4096, the joystick axis range).
// register conventions of the callees, from the call sites:
//   0x48cdb9: mov ecx,edi ; call 0x490b50        input_get_key_state(CX = key index)
//   0x48e92c: mov eax,0x710328 ; mov ecx,0x712498 ; (edx = action) ; call 0x48ca10
//                                                 input_accumulate_axis_value(EAX, ECX, DX)
//   0x48dabd: push eax(|delta|) ; push ecx(sensitivity) ; call 0x48cb60 ; add esp,8
//   0x48ea0a: lea edx,[esp+0x44](snapshot) ; mov ecx,0x712498 ; call 0x48fce0 ; test al,al
//   0x48ea33: xor eax,eax ; call 0x48fd60 ; test al,al     input_should_invert_look(AX = 0)
// The digital-axis clamp (0x13..0x1a) is inlined at every keyboard, mouse-button and
//   gamepad-button site; it is the exact body of input_accumulate_axis_value 0x48ca10 applied
//   to settings[0] / states[0], so the rewrite calls that function. The 12-byte last-used
//   record stores are likewise the inlined body of input_last_used_binding_set 0x490050
//   (bounds check 0 <= action < 0x1b included), so the rewrite calls that.
// Binary quirks reproduced on purpose (see the inline comments):
//   - with no mouse device the mouse state pointer is NULL and 0x48cd27 still reads [esi]
//     and [esi+4] / [esi+8] from it (the same dead path input_scan_any_bound_input has);
//   - joystick POV frame counts are widened to int before the max against the action byte,
//     so the 256th held frame stores 0 (0x48e8e4 movzx/inc; 0x48ea58 cmp eax,ecx);
//   - the POV diagonal split indexes its 8 "already applied" flags at octant - 1 and
//     octant + 1 without wrapping, so octant 0 reads/writes the high byte of the gamepad axis
//     direction local ([esp+0x37]) and octant 7 the low byte of the saved mouse wheel delta
//     ([esp+0x40]); the frame layout below keeps those bytes where the binary has them;
//   - the three buttons 1 (switch grenade), 3 (switch weapon) and 11 (zoom) are forced back to
//     0 unless their hold count is exactly 1, so they only ever report the first frame.
// UNSURE: the gamepad axis direction local ([esp+0x34]) is never initialized by the binary;
//   it only matters for the octant-0 guard byte above, and is zeroed here.
// reconciled: R19 player_control_settings unknown ranges named from the input.h field map (keyboard, mouse_button/mouse_axis, gamepad_button, gamepad_action_button, gamepad_axis, gamepad_pov, forward_rate..mouse_strafe_scale, mouse_look_x/y_sensitivity, gamepad_axis_scale_x/y, gamepad_rate_80/40, look_inverted/_driving); same offsets and widths

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern input_abstraction_globals input_globals;                  // 0x00710328
extern int16_t keyboard_bindings[k_control_keyboard_key_count];  // 0x00710330
extern int16_t mouse_button_bindings[k_control_mouse_button_count]; // 0x0071040a
extern int16_t mouse_axis_bindings[k_control_mouse_axis_count][2];  // 0x0071041a
extern int16_t gamepad_button_bindings[k_control_gamepad_count][k_control_gamepad_button_count]; // 0x00710426
extern int16_t gamepad_action_buttons[k_control_gamepad_count][2];  // 0x00710526
extern int16_t gamepad_axis_bindings[k_control_gamepad_count][k_control_gamepad_axis_count][2]; // 0x00710536
extern int16_t gamepad_pov_bindings[k_control_gamepad_count][k_control_gamepad_pov_count][k_control_gamepad_pov_direction_count]; // 0x00710736
extern uint8_t mouse_axis_frames[k_input_mouse_axis_count][2];   // 0x0071291c
extern uint8_t joystick_axis_frames[4][0x20][2];                 // 0x00712928
extern uint8_t joystick_pov_frames[4][0x10][8];                  // 0x00712a28
extern void *mouse_device;                                       // 0x006b1804
extern uint8_t input_suppressed;                                 // 0x006b15f9
extern mouse_state live_mouse_state;                             // 0x006b180c
extern mouse_state mouse_neutral_state;                          // 0x006b1828
extern int32_t joystick_slot_devices[4];                         // 0x006b2ce8
extern input_device input_devices[8];                            // 0x006b1868
extern joystick_state joystick_states[4];                        // 0x006b2a68
extern joystick_state joystick_neutral_state;                    // 0x006b2cf8
extern int32_t last_input_device;                                // 0x0087a460
extern real look_yaw_rate_setting[k_maximum_local_players];      // 0x006f1d74, game module
extern real look_pitch_rate_setting[k_maximum_local_players];    // 0x006f1d78, game module

extern uint8_t input_get_key_state(int16_t key_index);           // 0x00490b50, blam-cc: CX
extern void input_accumulate_axis_value(player_control_settings *settings,
    local_player_input_state *state, int16_t action);            // 0x0048ca10, blam-cc: EAX, ECX, DX
extern float input_mouse_acceleration_evaluate(float sensitivity, int32_t magnitude); // 0x0048cb60
extern uint8_t input_accumulator_is_idle(local_player_input_state *current,
    local_player_input_state *previous);                         // 0x0048fce0, blam-cc: ECX, EDX
extern uint8_t input_should_invert_look(int16_t local_player_index); // 0x0048fd60, blam-cc: AX
extern void input_last_used_binding_set(int16_t action, int16_t device_type, int16_t device_index,
    int16_t input_kind, int16_t input_index, int32_t direction); // 0x00490050

// Scales one analog step onto a throttle axis and clamps it to [-1, 1] exactly like the
// inlined x87 sequences (compare against -1 first, then against 1, NaN stores unchanged).
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

// The per-frame game action update. Rebuilds local player 0's action state (19 digital hold
// counts, two throttle axes, two look axes) from every bound keyboard key, mouse button, mouse
// axis and, per controller slot, gamepad button, axis and POV hat; remembers which input last
// drove each action; clears the frame's idle flag when the result changed; and applies look
// inversion.
void input_game_action_update(void)
{
    player_control_settings *settings = &input_globals.settings[0];
    local_player_input_state *state = &input_globals.states[0];
    uint8_t *buttons = (uint8_t *)state->buttons;  // hold counts compare unsigned (cmp/ja/jbe)
    const float *digital_rates = &settings->forward_rate; // +0x810: [0] forward,
                                                   // [1] strafe, [2] look x, [3] look y,
                                                   // [4] mouse forward scale, [5] mouse strafe scale
    const float *gamepad_axis_scales = &settings->gamepad_axis_scale_x; // +0x830 x, +0x834 y
    local_player_input_state previous;             // [esp+0x54]
    joystick_state *slot_states[4];                // [esp+0x44]
    mouse_state *mouse;
    // [esp+0x34]..[esp+0x43] in the binary: the gamepad axis direction, then the saved mouse
    // x / y / wheel. After the mouse section the first 8 bytes of mouse_delta are reused as
    // the per-slot POV "already applied" flags, indexed octant - 1 .. octant + 1 unwrapped.
    struct {
        int32_t axis_direction;                    // [esp+0x34]
        int32_t mouse_delta[3];                    // [esp+0x38] x, y, wheel
    } frame;
    uint8_t *pov_applied = (uint8_t *)frame.mouse_delta;
    int32_t slot;
    int32_t index;
    int16_t action;
    uint8_t frames;
    int32_t mouse_direction = 0;

    memcpy(&previous, state, sizeof(previous));

    mouse = (mouse_state *)0;
    if (mouse_device != (void *)0) {
        mouse = (input_suppressed != 0) ? &mouse_neutral_state : &live_mouse_state;
    }

    look_yaw_rate_setting[0] = settings->look_rate_80;
    look_pitch_rate_setting[0] = settings->look_rate_40;

    for (slot = 0; slot < 4; slot++) {
        slot_states[slot] = (joystick_state *)0;
        if (joystick_slot_devices[slot] != -1) {
            slot_states[slot] = (input_suppressed != 0) ? &joystick_neutral_state : &joystick_states[slot];
        }
    }

    // 0x48cd27: read unconditionally, a NULL read when there is no mouse device.
    frame.axis_direction = 0; // UNSURE: uninitialized stack in the binary
    frame.mouse_delta[0] = mouse->x;
    frame.mouse_delta[1] = mouse->y;
    frame.mouse_delta[2] = mouse->wheel;

    // clear the 0x13 hold counts, the analog-look flag and the four axes (pad_13 is left alone)
    memset(state->buttons, 0, sizeof(state->buttons));
    state->look_is_analog = 0;
    state->look_y = 0.0f;
    state->look_x = 0.0f;
    state->throttle_x = 0.0f;
    state->throttle_y = 0.0f;

    // keyboard: every key index 0 .. 0x6c
    for (index = 0; index < k_control_keyboard_key_count; index++) {
        action = keyboard_bindings[index];
        if (action == k_input_unbound) {
            // nothing
        } else if (action >= _input_action_forward && action <= _input_action_look_right) {
            if (input_get_key_state((int16_t)index) != 0) {
                input_accumulate_axis_value(settings, state, action);
            }
        } else {
            // the binary calls input_get_key_state twice here (compare, then reload); it is
            // side effect free, so this is max(current, key) in unsigned bytes. The action is
            // not range checked: any other value indexes past buttons, as in the binary.
            frames = input_get_key_state((int16_t)index);
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
        }
        if (input_get_key_state((int16_t)index) != 0 && action != k_input_unbound) {
            input_last_used_binding_set(action, _control_device_keyboard, 0, _control_input_button,
                                        (int16_t)index, 0);
            last_input_device = 0;
        }
    }

    // mouse buttons: always the live state, gated on a device and not suppressed
    for (index = 0; index < k_control_mouse_button_count; index++) {
        action = mouse_button_bindings[index];
        frames = 0;
        if (mouse_device != (void *)0 && input_suppressed == 0) {
            frames = live_mouse_state.button_frames[index];
        }
        if (action == k_input_unbound) {
            // nothing
        } else if (action >= _input_action_forward && action <= _input_action_look_right) {
            if (mouse_device == (void *)0) {
                continue; // 0x48d238: straight to the loop increment
            }
            if (frames != 0) {
                input_accumulate_axis_value(settings, state, action);
            }
        } else {
            // 0x48d6d4: max(current, button) unsigned; the binary re-reads the button when
            // it wins, which is the same value
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
        }
        if (mouse_device != (void *)0 && input_suppressed == 0 &&
            live_mouse_state.button_frames[index] != 0 && action != k_input_unbound) {
            input_last_used_binding_set(action, _control_device_mouse, 0, _control_input_button,
                                        (int16_t)index, 0);
            last_input_device = 0;
        }
    }

    // mouse axes x, y, wheel: [axis][0] is bound to a positive delta (direction 1), [axis][1]
    // to a negative one (direction 2); the frame counters count consecutive frames of one sign
    for (index = 0; index < k_input_mouse_axis_count; index++) {
        int32_t delta = frame.mouse_delta[index];
        float step;

        if (delta == 0) {
            mouse_axis_frames[index][0] = 0;
            mouse_axis_frames[index][1] = 0;
            continue;
        }
        if (delta < 0) {
            action = mouse_axis_bindings[index][1];
            frames = (uint8_t)(mouse_axis_frames[index][1] + 1);
            mouse_axis_frames[index][0] = 0;
            mouse_axis_frames[index][1] = frames;
            mouse_direction = 2;
        } else {
            action = mouse_axis_bindings[index][0];
            frames = (uint8_t)(mouse_axis_frames[index][0] + 1);
            mouse_axis_frames[index][0] = frames;
            mouse_axis_frames[index][1] = 0;
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
            state->look_y = input_mouse_acceleration_evaluate(settings->mouse_look_y_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_down:
            state->look_is_analog = 0;
            state->look_y = -input_mouse_acceleration_evaluate(settings->mouse_look_y_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_left:
            state->look_is_analog = 0;
            state->look_x = input_mouse_acceleration_evaluate(settings->mouse_look_x_sensitivity, delta < 0 ? -delta : delta);
            break;
        case _input_action_look_right:
            state->look_is_analog = 0;
            state->look_x = -input_mouse_acceleration_evaluate(settings->mouse_look_x_sensitivity, delta < 0 ? -delta : delta);
            break;
        default:
            if (buttons[action] > frames) {
                frames = buttons[action];
            }
            buttons[action] = frames;
            break;
        }
        if (action != k_input_unbound) {
            input_last_used_binding_set(action, _control_device_mouse, 0, _control_input_axis,
                                        (int16_t)index, (int16_t)mouse_direction);
            last_input_device = 0;
        }
    }

    // gamepads, one controller slot at a time
    for (slot = 0; slot < 4; slot++) {
        int32_t device = joystick_slot_devices[slot];
        joystick_state *pad = slot_states[slot];
        int32_t button_count = 0;
        int32_t axis_count = 0;
        int32_t pov_count = 0;
        int16_t accept_button;
        int16_t back_button;

        if (device != -1) {
            button_count = input_devices[device].button_count;
            axis_count = input_devices[device].axis_count;
            pov_count = input_devices[device].pov_count;
        }

        // accept / back read the per-pad button index table, unless a keyboard key already
        // set them this frame
        accept_button = gamepad_action_buttons[slot][0];
        if (buttons[_input_action_accept] == 0 && accept_button != -1) {
            buttons[_input_action_accept] = (device != -1) ? pad->button_frames[accept_button] : 0;
        }
        back_button = gamepad_action_buttons[slot][1];
        if (buttons[_input_action_back] == 0 && back_button != -1) {
            buttons[_input_action_back] = (device != -1) ? pad->button_frames[back_button] : 0;
        }

        // buttons
        for (index = 0; index < button_count; index++) {
            action = gamepad_button_bindings[slot][index];
            frames = (device != -1) ? pad->button_frames[index] : 0;
            if (action == k_input_unbound || action == _input_action_accept || action == _input_action_back) {
                // nothing: accept / back were handled above
            } else if (action >= _input_action_forward && action <= _input_action_look_right) {
                if (frames != 0) {
                    input_accumulate_axis_value(settings, state, action);
                }
            } else {
                if (buttons[action] > frames) {
                    frames = buttons[action];
                }
                buttons[action] = frames;
            }
            if (device != -1 && pad->button_frames[index] != 0 && action != k_input_unbound) {
                input_last_used_binding_set(action, _control_device_gamepad, (int16_t)slot,
                                            _control_input_button, (int16_t)index, 0);
                last_input_device = slot + 1;
            }
        }

        // axes: [axis][0] positive (direction 1), [axis][1] negative (direction 2); the value
        // range is -0x1000 .. 0x1000
        for (index = 0; index < axis_count; index++) {
            int32_t value = pad->axes[index];
            int32_t magnitude = value < 0 ? -value : value;
            float inverse_scale;

            if (value == 0) {
                joystick_axis_frames[slot][index][0] = 0;
                joystick_axis_frames[slot][index][1] = 0;
                continue;
            }
            if (value < 0) {
                joystick_axis_frames[slot][index][1]++;
                action = gamepad_axis_bindings[slot][index][1];
                joystick_axis_frames[slot][index][0] = 0;
                frames = joystick_axis_frames[slot][index][1];
                frame.axis_direction = 2;
            } else {
                frames = (uint8_t)(joystick_axis_frames[slot][index][0] + 1);
                action = gamepad_axis_bindings[slot][index][0];
                joystick_axis_frames[slot][index][0] = frames;
                joystick_axis_frames[slot][index][1] = 0;
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
                look_pitch_rate_setting[0] = settings->gamepad_rate_40[slot];
                state->look_y = (float)magnitude * 0.000244140625f;
                break;
            case _input_action_look_down:
                state->look_is_analog = 1;
                look_pitch_rate_setting[0] = settings->gamepad_rate_40[slot];
                state->look_y = -((float)magnitude * 0.000244140625f);
                break;
            case _input_action_look_left:
                state->look_is_analog = 1;
                look_yaw_rate_setting[0] = settings->gamepad_rate_80[slot];
                state->look_x = (float)magnitude * 0.000244140625f;
                break;
            case _input_action_look_right:
                state->look_is_analog = 1;
                look_yaw_rate_setting[0] = settings->gamepad_rate_80[slot];
                state->look_x = -((float)magnitude * 0.000244140625f);
                break;
            default:
                if (buttons[action] > frames) {
                    frames = buttons[action];
                }
                buttons[action] = frames;
                break;
            }
            // 0x48e7f4 re-reads the axis; it is still nonzero here
            if (pad->axes[index] != 0 && action != k_input_unbound) {
                input_last_used_binding_set(action, _control_device_gamepad, (int16_t)slot,
                                            _control_input_axis, (int16_t)index,
                                            (int16_t)frame.axis_direction);
                last_input_device = slot + 1;
            }
        }

        // POV hats: one binding per octant (0 north, clockwise). An unbound octant between two
        // bound axis octants (a diagonal) drives both neighbours, each at most once per slot.
        memset(pov_applied, 0, 8);
        for (index = 0; index < pov_count; index++) {
            int32_t octant = pad->povs[index];
            int32_t count;
            int16_t *octant_bindings = gamepad_pov_bindings[slot][index];

            if (octant == -1) {
                memset(joystick_pov_frames[slot][index], 0, 8);
                continue;
            }
            count = joystick_pov_frames[slot][index][octant] + 1; // int, not wrapped to a byte
            memset(joystick_pov_frames[slot][index], 0, 8);
            joystick_pov_frames[slot][index][octant] = (uint8_t)count;
            action = octant_bindings[octant];

            if (action == k_input_unbound) {
                int16_t next_action = (octant == 7) ? octant_bindings[0] : octant_bindings[octant + 1];
                int16_t previous_action = (octant == 0) ? octant_bindings[7] : octant_bindings[octant - 1];

                // 0x48eabc: both in 0x13 .. 0x1b inclusive (0x1b accumulates nothing and is
                // not recorded, but still sets its flag)
                if (next_action >= 0x13 && next_action <= 0x1b &&
                    previous_action >= 0x13 && previous_action <= 0x1b) {
                    // flags and recorded directions are octant + 1 / octant - 1, not wrapped
                    if (pov_applied[octant + 1] == 0) {
                        input_accumulate_axis_value(settings, state, next_action);
                        pov_applied[octant + 1] = 1;
                        input_last_used_binding_set(next_action, _control_device_gamepad, (int16_t)slot,
                                                    _control_input_pov, (int16_t)index, octant + 1);
                    }
                    if (pov_applied[octant - 1] == 0) {
                        input_accumulate_axis_value(settings, state, previous_action);
                        pov_applied[octant - 1] = 1;
                        input_last_used_binding_set(previous_action, _control_device_gamepad, (int16_t)slot,
                                                    _control_input_pov, (int16_t)index, octant - 1);
                    }
                    // the diagonal path does not touch last_input_device
                }
                continue;
            }
            if (action >= _input_action_forward && action <= _input_action_look_right) {
                input_accumulate_axis_value(settings, state, action);
                pov_applied[octant] = 1;
            } else if (action != _input_action_accept && action != _input_action_back) {
                if (count < (int32_t)buttons[action]) {
                    count = buttons[action];
                }
                buttons[action] = (uint8_t)count;
            }
            input_last_used_binding_set(action, _control_device_gamepad, (int16_t)slot,
                                        _control_input_pov, (int16_t)index, octant);
            last_input_device = slot + 1;
        }
    }

    // switch grenade, switch weapon and zoom report only their first frame
    if (buttons[_input_action_zoom] != 1) {
        buttons[_input_action_zoom] = 0;
    }
    if (buttons[_input_action_switch_grenade] != 1) {
        buttons[_input_action_switch_grenade] = 0;
    }
    if (buttons[_input_action_switch_weapon] != 1) {
        buttons[_input_action_switch_weapon] = 0;
    }

    if (!input_accumulator_is_idle(state, &previous)) {
        input_globals.idle = 0;
    }

    // +0x858 look inverted, +0x859 look inverted while piloting (see input_should_invert_look)
    if (settings->look_inverted != 0 ||
        (settings->look_inverted_driving != 0 && input_should_invert_look(0))) {
        state->look_y = -state->look_y;
    }
}

#if 0
Original Ghidra decompilation (0x48cca0; the body already includes the mis-split 0x48d270 region),
from tools/pack.py 0x48cca0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0048cca0(void)

{
  int iVar1;
  float fVar2;
  float fVar3;
  byte bVar4;
  byte bVar5;
  char cVar6;
  undefined4 *puVar7;
  undefined1 uVar8;
  int iVar9;
  uint uVar10;
  undefined2 uVar11;
  undefined4 uVar12;
  short sVar13;
  short sVar14;
  short sVar15;
  short *psVar16;
  short sVar17;
  undefined4 *puVar18;
  int iVar19;
  int iVar20;
  float10 fVar21;
  byte bStack_69;
  int iStack_68;
  int local_64;
  short *psStack_60;
  int iStack_5c;
  int *piStack_58;
  short *psStack_54;
  int iStack_50;
  undefined4 uStack_48;
  undefined4 local_44;
  undefined1 local_40;
  undefined2 uStack_3f;
  undefined1 uStack_3d;
  undefined4 local_3c;
  int aiStack_38 [4];
  undefined4 local_28 [10];
  
  puVar18 = &DAT_00712498;
  puVar7 = local_28;
  for (iVar9 = 10; iVar9 != 0; iVar9 = iVar9 + -1) {
    *puVar7 = *puVar18;
    puVar18 = puVar18 + 1;
    puVar7 = puVar7 + 1;
  }
  puVar18 = (undefined4 *)0x0;
  if ((DAT_006b1804 != 0) && (puVar18 = &DAT_006b1828, DAT_006b15f9 == '\0')) {
    puVar18 = &DAT_006b180c;
  }
  iVar9 = 0;
  _DAT_006f1d74 = DAT_00710328;
  _DAT_006f1d78 = DAT_0071032c;
  do {
    puVar7 = (undefined4 *)0x0;
    if ((&DAT_006b2ce8)[(short)iVar9] != -1) {
      if (DAT_006b15f9 == '\0') {
        puVar7 = &DAT_006b2a68 + (short)iVar9 * 0x28;
      }
      else {
        puVar7 = &DAT_006b2cf8;
      }
    }
    aiStack_38[iVar9] = (int)puVar7;
    iVar9 = iVar9 + 1;
  } while (iVar9 < 4);
  local_44 = *puVar18;
  uVar12 = puVar18[1];
  DAT_00712498 = 0;
  DAT_0071249c = 0;
  _DAT_007124a0 = 0;
  _DAT_007124a4 = 0;
  _DAT_007124a8 = 0;
  DAT_007124aa = 0;
  DAT_007124bc = 0;
  local_40 = (undefined1)uVar12;
  uStack_3f = (undefined2)((uint)uVar12 >> 8);
  uStack_3d = (undefined1)((uint)uVar12 >> 0x18);
  sVar13 = 0;
  psVar16 = &DAT_00710330;
  _DAT_007124b8 = 0.0;
  _DAT_007124b4 = 0.0;
  _DAT_007124ac = 0.0;
  _DAT_007124b0 = 0.0;
  local_3c = puVar18[2];
  do {
    sVar14 = *psVar16;
    iVar9 = (int)sVar14;
    if (iVar9 < 0x8000) {
      if (iVar9 != 0x7fff) {
        switch(iVar9) {
        case 0x13:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_00710b38 + _DAT_007124ac) {
              if (_DAT_00710b38 + _DAT_007124ac <= 1.0) {
                _DAT_007124ac = _DAT_00710b38 + _DAT_007124ac;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
          }
          break;
        case 0x14:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_007124ac - _DAT_00710b38) {
              if (_DAT_007124ac - _DAT_00710b38 <= 1.0) {
                _DAT_007124ac = _DAT_007124ac - _DAT_00710b38;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
          }
          break;
        case 0x15:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_00710b3c + _DAT_007124b0) {
              if (_DAT_00710b3c + _DAT_007124b0 <= 1.0) {
                _DAT_007124b0 = _DAT_00710b3c + _DAT_007124b0;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
          }
          break;
        case 0x16:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_007124b0 - _DAT_00710b3c) {
              if (_DAT_007124b0 - _DAT_00710b3c <= 1.0) {
                _DAT_007124b0 = _DAT_007124b0 - _DAT_00710b3c;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
          }
          break;
        case 0x17:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_00710b44 + _DAT_007124b8) {
              if (_DAT_00710b44 + _DAT_007124b8 <= 1.0) {
                _DAT_007124b8 = _DAT_00710b44 + _DAT_007124b8;
              }
              else {
                _DAT_007124b8 = 1.0;
              }
            }
            else {
              _DAT_007124b8 = -1.0;
            }
          }
          break;
        case 0x18:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_007124b8 - _DAT_00710b44) {
              if (_DAT_007124b8 - _DAT_00710b44 <= 1.0) {
                _DAT_007124b8 = _DAT_007124b8 - _DAT_00710b44;
              }
              else {
                _DAT_007124b8 = 1.0;
              }
            }
            else {
              _DAT_007124b8 = -1.0;
            }
          }
          break;
        case 0x19:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_00710b40 + _DAT_007124b4) {
              if (_DAT_00710b40 + _DAT_007124b4 <= 1.0) {
                _DAT_007124b4 = _DAT_00710b40 + _DAT_007124b4;
              }
              else {
                _DAT_007124b4 = 1.0;
              }
            }
            else {
              _DAT_007124b4 = -1.0;
            }
          }
          break;
        case 0x1a:
          cVar6 = FUN_00490b50();
          if (cVar6 != '\0') {
            if (-1.0 <= _DAT_007124b4 - _DAT_00710b40) {
              if (_DAT_007124b4 - _DAT_00710b40 <= 1.0) {
                _DAT_007124b4 = _DAT_007124b4 - _DAT_00710b40;
              }
              else {
                _DAT_007124b4 = 1.0;
              }
            }
            else {
              _DAT_007124b4 = -1.0;
            }
          }
          break;
        default:
          goto switchD_0048cdb2_default;
        }
      }
    }
    else {
switchD_0048cdb2_default:
      bVar5 = *(byte *)((int)&DAT_00712498 + iVar9);
      bVar4 = FUN_00490b50();
      if (bVar5 <= bVar4) {
        bVar5 = FUN_00490b50();
      }
      *(byte *)((int)&DAT_00712498 + iVar9) = bVar5;
    }
    cVar6 = FUN_00490b50();
    if ((cVar6 != '\0') && (iVar9 != 0x7fff)) {
      if ((-1 < sVar14) && (sVar14 < 0x1b)) {
        iVar9 = sVar14 * 0xc;
        *(undefined2 *)(&DAT_007127d4 + iVar9) = 1;
        *(undefined2 *)(&DAT_007127d6 + iVar9) = 0;
        *(undefined2 *)(&DAT_007127d8 + iVar9) = 0;
        *(short *)(&DAT_007127da + iVar9) = sVar13;
        *(undefined4 *)(&DAT_007127dc + iVar9) = 0;
      }
      DAT_0087a460 = 0;
    }
    psVar16 = psVar16 + 1;
    sVar13 = sVar13 + 1;
  } while ((int)psVar16 < 0x71040a);
  iVar9 = 0;
  do {
    sVar13 = (&DAT_0071040a)[iVar9];
    local_64 = (int)sVar13;
    sVar14 = (short)iVar9;
    if (local_64 < 0x8000) {
      if (local_64 == 0x7fff) goto LAB_0048d71e;
      switch(local_64) {
      case 0x13:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_00710b38 + _DAT_007124ac) {
              if (_DAT_00710b38 + _DAT_007124ac <= 1.0) {
                _DAT_007124ac = _DAT_00710b38 + _DAT_007124ac;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x14:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_007124ac - _DAT_00710b38) {
              if (_DAT_007124ac - _DAT_00710b38 <= 1.0) {
                _DAT_007124ac = _DAT_007124ac - _DAT_00710b38;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x15:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_00710b3c + _DAT_007124b0) {
              if (_DAT_00710b3c + _DAT_007124b0 <= 1.0) {
                _DAT_007124b0 = _DAT_00710b3c + _DAT_007124b0;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x16:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_007124b0 - _DAT_00710b3c) {
              if (_DAT_007124b0 - _DAT_00710b3c <= 1.0) {
                _DAT_007124b0 = _DAT_007124b0 - _DAT_00710b3c;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x17:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_00710b44 + _DAT_007124b8) {
              if (_DAT_00710b44 + _DAT_007124b8 <= 1.0) {
                _DAT_007124b8 = _DAT_00710b44 + _DAT_007124b8;
              }
              else {
                _DAT_007124b8 = 1.0;
              }
            }
            else {
              _DAT_007124b8 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x18:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_007124b8 - _DAT_00710b44) {
              if (_DAT_007124b8 - _DAT_00710b44 <= 1.0) {
                _DAT_007124b8 = _DAT_007124b8 - _DAT_00710b44;
              }
              else {
                _DAT_007124b8 = 1.0;
              }
            }
            else {
              _DAT_007124b8 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x19:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_00710b40 + _DAT_007124b4) {
              if (_DAT_00710b40 + _DAT_007124b4 <= 1.0) {
                _DAT_007124b4 = _DAT_00710b40 + _DAT_007124b4;
              }
              else {
                _DAT_007124b4 = 1.0;
              }
            }
            else {
              _DAT_007124b4 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      case 0x1a:
        if (DAT_006b1804 != 0) {
          if ((DAT_006b15f9 == '\0') && ((&DAT_006b1818)[sVar14] != '\0')) {
            if (-1.0 <= _DAT_007124b4 - _DAT_00710b40) {
              if (_DAT_007124b4 - _DAT_00710b40 <= 1.0) {
                _DAT_007124b4 = _DAT_007124b4 - _DAT_00710b40;
              }
              else {
                _DAT_007124b4 = 1.0;
              }
            }
            else {
              _DAT_007124b4 = -1.0;
            }
          }
          goto LAB_0048d71e;
        }
        break;
      default:
        goto switchD_0048d22b_default;
      }
    }
    else {
switchD_0048d22b_default:
      bVar5 = 0;
      if ((DAT_006b1804 != 0) && (DAT_006b15f9 == '\0')) {
        bVar5 = (&DAT_006b1818)[sVar14];
      }
      bVar4 = *(byte *)((int)&DAT_00712498 + local_64);
      if (((bVar4 <= bVar5) && (bVar4 = 0, DAT_006b1804 != 0)) && (DAT_006b15f9 == '\0')) {
        bVar4 = (&DAT_006b1818)[sVar14];
      }
      *(byte *)((int)&DAT_00712498 + local_64) = bVar4;
LAB_0048d71e:
      if ((((DAT_006b1804 != 0) && (DAT_006b15f9 == '\0')) && ((&DAT_006b1818)[sVar14] != '\0')) &&
         (local_64 != 0x7fff)) {
        if ((-1 < sVar13) && (sVar13 < 0x1b)) {
          iVar20 = sVar13 * 0xc;
          *(undefined2 *)(&DAT_007127d4 + iVar20) = 2;
          *(undefined2 *)(&DAT_007127d6 + iVar20) = 0;
          *(undefined2 *)(&DAT_007127d8 + iVar20) = 0;
          *(short *)(&DAT_007127da + iVar20) = sVar14;
          *(undefined4 *)(&DAT_007127dc + iVar20) = 0;
        }
        DAT_0087a460 = 0;
      }
    }
    iVar9 = iVar9 + 1;
  } while (iVar9 < 8);
  iVar9 = 0;
  uVar12 = uStack_48;
  do {
    iVar20 = (&local_44)[iVar9];
    if (iVar20 == 0) {
      (&DAT_0071291c)[iVar9 * 2] = 0;
      (&DAT_0071291d)[iVar9 * 2] = 0;
    }
    else {
      if (iVar20 < 0) {
        local_64 = (int)(short)(&DAT_0071041c)[iVar9 * 2];
        bStack_69 = (&DAT_0071291d)[iVar9 * 2] + 1;
        (&DAT_0071291c)[iVar9 * 2] = 0;
        (&DAT_0071291d)[iVar9 * 2] = bStack_69;
        uVar12 = 2;
      }
      else if (0 < iVar20) {
        local_64 = (int)(short)(&DAT_0071041a)[iVar9 * 2];
        bStack_69 = (&DAT_0071291c)[iVar9 * 2] + 1;
        (&DAT_0071291c)[iVar9 * 2] = bStack_69;
        (&DAT_0071291d)[iVar9 * 2] = 0;
        uVar12 = 1;
      }
      if (local_64 < 0x8000) {
        if (local_64 != 0x7fff) {
          switch(local_64) {
          case 0x13:
            fVar2 = (float)iVar20 / _DAT_00710b48;
            fVar3 = fVar2;
            if (fVar2 < 0.0) {
              fVar3 = -fVar2;
            }
            if (-1.0 <= _DAT_007124ac + fVar3) {
              fVar3 = fVar2;
              if (fVar2 < 0.0) {
                fVar3 = -fVar2;
              }
              if (_DAT_007124ac + fVar3 <= 1.0) {
                if (fVar2 < 0.0) {
                  fVar2 = -fVar2;
                }
                _DAT_007124ac = fVar2 + _DAT_007124ac;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
            break;
          case 0x14:
            fVar2 = (float)iVar20 / _DAT_00710b48;
            fVar3 = fVar2;
            if (fVar2 < 0.0) {
              fVar3 = -fVar2;
            }
            if (-1.0 <= _DAT_007124ac - fVar3) {
              fVar3 = fVar2;
              if (fVar2 < 0.0) {
                fVar3 = -fVar2;
              }
              if (_DAT_007124ac - fVar3 <= 1.0) {
                if (fVar2 < 0.0) {
                  fVar2 = -fVar2;
                }
                _DAT_007124ac = _DAT_007124ac - fVar2;
              }
              else {
                _DAT_007124ac = 1.0;
              }
            }
            else {
              _DAT_007124ac = -1.0;
            }
            break;
          case 0x15:
            fVar2 = (float)iVar20 / _DAT_00710b4c;
            fVar3 = fVar2;
            if (fVar2 < 0.0) {
              fVar3 = -fVar2;
            }
            if (-1.0 <= _DAT_007124b0 + fVar3) {
              fVar3 = fVar2;
              if (fVar2 < 0.0) {
                fVar3 = -fVar2;
              }
              if (_DAT_007124b0 + fVar3 <= 1.0) {
                if (fVar2 < 0.0) {
                  fVar2 = -fVar2;
                }
                _DAT_007124b0 = fVar2 + _DAT_007124b0;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
            break;
          case 0x16:
            fVar2 = (float)iVar20 / _DAT_00710b4c;
            fVar3 = fVar2;
            if (fVar2 < 0.0) {
              fVar3 = -fVar2;
            }
            if (-1.0 <= _DAT_007124b0 - fVar3) {
              fVar3 = fVar2;
              if (fVar2 < 0.0) {
                fVar3 = -fVar2;
              }
              if (_DAT_007124b0 - fVar3 <= 1.0) {
                if (fVar2 < 0.0) {
                  fVar2 = -fVar2;
                }
                _DAT_007124b0 = _DAT_007124b0 - fVar2;
              }
              else {
                _DAT_007124b0 = 1.0;
              }
            }
            else {
              _DAT_007124b0 = -1.0;
            }
            break;
          case 0x17:
            DAT_007124bc = 0;
            iVar19 = iVar20;
            if (iVar20 < 0) {
              iVar19 = -iVar20;
            }
            fVar21 = (float10)FUN_0048cb60(DAT_00710b54,iVar19);
            _DAT_007124b8 = (float)fVar21;
            break;
          case 0x18:
            DAT_007124bc = 0;
            iVar19 = iVar20;
            if (iVar20 < 0) {
              iVar19 = -iVar20;
            }
            fVar21 = (float10)FUN_0048cb60(DAT_00710b54,iVar19);
            _DAT_007124b8 = (float)-fVar21;
            break;
          case 0x19:
            DAT_007124bc = 0;
            iVar19 = iVar20;
            if (iVar20 < 0) {
              iVar19 = -iVar20;
            }
            fVar21 = (float10)FUN_0048cb60(DAT_00710b50,iVar19);
            _DAT_007124b4 = (float)fVar21;
            break;
          case 0x1a:
            DAT_007124bc = 0;
            iVar19 = iVar20;
            if (iVar20 < 0) {
              iVar19 = -iVar20;
            }
            fVar21 = (float10)FUN_0048cb60(DAT_00710b50,iVar19);
            _DAT_007124b4 = (float)-fVar21;
            break;
          default:
            goto switchD_0048d821_default;
          }
        }
      }
      else {
switchD_0048d821_default:
        bVar5 = *(byte *)((int)&DAT_00712498 + local_64);
        if (*(byte *)((int)&DAT_00712498 + local_64) <= bStack_69) {
          bVar5 = bStack_69;
        }
        *(byte *)((int)&DAT_00712498 + local_64) = bVar5;
      }
      if ((iVar20 != 0) && (local_64 != 0x7fff)) {
        sVar13 = (short)local_64;
        if ((-1 < sVar13) && (sVar13 < 0x1b)) {
          iVar20 = sVar13 * 0xc;
          *(undefined2 *)(&DAT_007127d4 + iVar20) = 2;
          *(undefined2 *)(&DAT_007127d6 + iVar20) = 0;
          *(undefined2 *)(&DAT_007127d8 + iVar20) = 1;
          *(short *)(&DAT_007127da + iVar20) = (short)iVar9;
          *(int *)(&DAT_007127dc + iVar20) = (int)(short)uVar12;
        }
        DAT_0087a460 = 0;
      }
    }
    iVar9 = iVar9 + 1;
  } while (iVar9 < 3);
  iStack_68 = 0;
  do {
    sVar13 = (short)iStack_68;
    iVar20 = (int)sVar13;
    iVar9 = (&DAT_006b2ce8)[iVar20];
    iStack_5c = 0;
    if (iVar9 != -1) {
      iStack_5c = *(int *)(&DAT_006b1aa0 + iVar9 * 0x240);
    }
    psStack_60 = (short *)0x0;
    if (iVar9 != -1) {
      psStack_60 = *(short **)(&DAT_006b1a9c + iVar9 * 0x240);
    }
    iStack_50 = 0;
    if (iVar9 != -1) {
      iStack_50 = *(int *)(&DAT_006b1aa4 + iVar9 * 0x240);
    }
    if ((DAT_007124a0 == '\0') && (sVar14 = (&DAT_00710526)[iStack_68 * 2], sVar14 != -1)) {
      uVar8 = 0;
      if (iVar9 != -1) {
        if (DAT_006b15f9 != '\0') {
          _DAT_007124a0 = CONCAT31(_DAT_007124a1,*(undefined1 *)((int)&DAT_006b2cf8 + (int)sVar14));
          goto LAB_0048dc7c;
        }
        if (&DAT_006b2a68 + iVar20 * 0x28 != (undefined4 *)0x0) {
          uVar8 = *(undefined1 *)((int)sVar14 + (int)(&DAT_006b2a68 + iVar20 * 0x28));
        }
      }
      _DAT_007124a0 = CONCAT31(_DAT_007124a1,uVar8);
    }
LAB_0048dc7c:
    if ((DAT_007124a1 == '\0') && ((&DAT_00710528)[iStack_68 * 2] != -1)) {
      uVar8 = 0;
      if (iVar9 != -1) {
        if (DAT_006b15f9 == '\0') {
          puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
          if (puVar18 == (undefined4 *)0x0) goto LAB_0048dcb8;
        }
        else {
          puVar18 = &DAT_006b2cf8;
        }
        uVar8 = *(undefined1 *)((int)(short)(&DAT_00710528)[iStack_68 * 2] + (int)puVar18);
      }
LAB_0048dcb8:
      _DAT_007124a0 = CONCAT11(uVar8,DAT_007124a0);
    }
    iVar19 = 0;
    if (0 < iStack_5c) {
      cVar6 = DAT_006b15f9;
      do {
        sVar14 = *(short *)((int)&DAT_00710328 + (iStack_68 * 0x20 + 0x7f + iVar19) * 2);
        local_64 = (int)sVar14;
        sVar17 = (short)iVar19;
        if (local_64 < 0x18) {
          if (local_64 == 0x17) {
            if (iVar9 != -1) {
              if (cVar6 == '\0') {
                puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                if (puVar18 == (undefined4 *)0x0) goto switchD_0048dd14_caseD_8;
              }
              else {
                puVar18 = &DAT_006b2cf8;
              }
              if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                if (-1.0 <= _DAT_00710b44 + _DAT_007124b8) {
                  if (_DAT_00710b44 + _DAT_007124b8 <= 1.0) {
                    _DAT_007124b8 = _DAT_00710b44 + _DAT_007124b8;
                  }
                  else {
                    _DAT_007124b8 = 1.0;
                  }
                }
                else {
                  _DAT_007124b8 = -1.0;
                }
              }
            }
          }
          else {
            switch(local_64) {
            case 8:
            case 9:
              break;
            default:
switchD_0048dd14_caseD_a:
              bVar5 = 0;
              if (iVar9 != -1) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) goto LAB_0048e24e;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                bVar5 = *(byte *)((int)sVar17 + (int)puVar18);
              }
LAB_0048e24e:
              bVar4 = *(byte *)((int)&DAT_00712498 + local_64);
              if ((bVar4 <= bVar5) && (bVar4 = 0, iVar9 != -1)) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) goto LAB_0048e285;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                bVar4 = *(byte *)((int)sVar17 + (int)puVar18);
              }
LAB_0048e285:
              *(byte *)((int)&DAT_00712498 + local_64) = bVar4;
              cVar6 = DAT_006b15f9;
              break;
            case 0x13:
              if (iVar9 != -1) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) break;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                  if (-1.0 <= _DAT_00710b38 + _DAT_007124ac) {
                    if (_DAT_00710b38 + _DAT_007124ac <= 1.0) {
                      _DAT_007124ac = _DAT_00710b38 + _DAT_007124ac;
                    }
                    else {
                      _DAT_007124ac = 1.0;
                    }
                  }
                  else {
                    _DAT_007124ac = -1.0;
                  }
                }
              }
              break;
            case 0x14:
              if (iVar9 != -1) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) break;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                  if (-1.0 <= _DAT_007124ac - _DAT_00710b38) {
                    if (_DAT_007124ac - _DAT_00710b38 <= 1.0) {
                      _DAT_007124ac = _DAT_007124ac - _DAT_00710b38;
                    }
                    else {
                      _DAT_007124ac = 1.0;
                    }
                  }
                  else {
                    _DAT_007124ac = -1.0;
                  }
                }
              }
              break;
            case 0x15:
              if (iVar9 != -1) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) break;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                  if (-1.0 <= _DAT_00710b3c + _DAT_007124b0) {
                    if (_DAT_00710b3c + _DAT_007124b0 <= 1.0) {
                      _DAT_007124b0 = _DAT_00710b3c + _DAT_007124b0;
                    }
                    else {
                      _DAT_007124b0 = 1.0;
                    }
                  }
                  else {
                    _DAT_007124b0 = -1.0;
                  }
                }
              }
              break;
            case 0x16:
              if (iVar9 != -1) {
                if (cVar6 == '\0') {
                  puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                  if (puVar18 == (undefined4 *)0x0) break;
                }
                else {
                  puVar18 = &DAT_006b2cf8;
                }
                if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                  if (-1.0 <= _DAT_007124b0 - _DAT_00710b3c) {
                    if (_DAT_007124b0 - _DAT_00710b3c <= 1.0) {
                      _DAT_007124b0 = _DAT_007124b0 - _DAT_00710b3c;
                    }
                    else {
                      _DAT_007124b0 = 1.0;
                    }
                  }
                  else {
                    _DAT_007124b0 = -1.0;
                  }
                }
              }
            }
          }
        }
        else if (local_64 < 0x1b) {
          if (local_64 == 0x1a) {
            if (iVar9 != -1) {
              if (cVar6 == '\0') {
                puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                if (puVar18 == (undefined4 *)0x0) goto switchD_0048dd14_caseD_8;
              }
              else {
                puVar18 = &DAT_006b2cf8;
              }
              if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                if (-1.0 <= _DAT_007124b4 - _DAT_00710b40) {
                  if (_DAT_007124b4 - _DAT_00710b40 <= 1.0) {
                    _DAT_007124b4 = _DAT_007124b4 - _DAT_00710b40;
                  }
                  else {
                    _DAT_007124b4 = 1.0;
                  }
                }
                else {
                  _DAT_007124b4 = -1.0;
                }
              }
            }
          }
          else if (local_64 == 0x18) {
            if (iVar9 != -1) {
              if (cVar6 == '\0') {
                puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                if (puVar18 == (undefined4 *)0x0) goto switchD_0048dd14_caseD_8;
              }
              else {
                puVar18 = &DAT_006b2cf8;
              }
              if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                if (-1.0 <= _DAT_007124b8 - _DAT_00710b44) {
                  if (_DAT_007124b8 - _DAT_00710b44 <= 1.0) {
                    _DAT_007124b8 = _DAT_007124b8 - _DAT_00710b44;
                  }
                  else {
                    _DAT_007124b8 = 1.0;
                  }
                }
                else {
                  _DAT_007124b8 = -1.0;
                }
              }
            }
          }
          else {
            if (local_64 != 0x19) goto switchD_0048dd14_caseD_a;
            if (iVar9 != -1) {
              if (cVar6 == '\0') {
                puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
                if (puVar18 == (undefined4 *)0x0) goto switchD_0048dd14_caseD_8;
              }
              else {
                puVar18 = &DAT_006b2cf8;
              }
              if (*(char *)((int)sVar17 + (int)puVar18) != '\0') {
                if (-1.0 <= _DAT_00710b40 + _DAT_007124b4) {
                  if (_DAT_00710b40 + _DAT_007124b4 <= 1.0) {
                    _DAT_007124b4 = _DAT_00710b40 + _DAT_007124b4;
                  }
                  else {
                    _DAT_007124b4 = 1.0;
                  }
                }
                else {
                  _DAT_007124b4 = -1.0;
                }
              }
            }
          }
        }
        else if (local_64 != 0x7fff) goto switchD_0048dd14_caseD_a;
switchD_0048dd14_caseD_8:
        iVar9 = (&DAT_006b2ce8)[iVar20];
        if (iVar9 != -1) {
          if (cVar6 == '\0') {
            puVar18 = &DAT_006b2a68 + iVar20 * 0x28;
            if (puVar18 == (undefined4 *)0x0) goto LAB_0048e303;
          }
          else {
            puVar18 = &DAT_006b2cf8;
          }
          if ((*(char *)((int)sVar17 + (int)puVar18) != '\0') && (local_64 != 0x7fff)) {
            if ((-1 < sVar14) && (sVar14 < 0x1b)) {
              iVar1 = sVar14 * 0xc;
              *(short *)(&DAT_007127d6 + iVar1) = sVar13;
              *(undefined2 *)(&DAT_007127d4 + iVar1) = 3;
              *(undefined2 *)(&DAT_007127d8 + iVar1) = 0;
              *(short *)(&DAT_007127da + iVar1) = sVar17;
              *(undefined4 *)(&DAT_007127dc + iVar1) = 0;
            }
            DAT_0087a460 = iStack_68 + 1;
          }
        }
LAB_0048e303:
        iVar19 = iVar19 + 1;
      } while (iVar19 < iStack_5c);
    }
    iVar9 = 0;
    if (0 < (int)psStack_60) {
      do {
        sVar14 = *(short *)(aiStack_38[iStack_68] + 0x20 + iVar9 * 2);
        iVar20 = (int)sVar14;
        if (iVar20 == 0) {
          iVar20 = (iStack_68 * 0x20 + iVar9) * 2;
          uVar8 = (undefined1)sVar14;
          (&DAT_00712928)[iVar20] = uVar8;
          (&DAT_00712929)[iVar20] = uVar8;
        }
        else {
          if (iVar20 < 0) {
            iVar19 = (iStack_68 * 0x20 + iVar9) * 2;
            (&DAT_00712929)[iVar19] = (&DAT_00712929)[iVar19] + '\x01';
            local_64 = (int)(short)(&DAT_00710538)[(iStack_68 * 0x20 + iVar9) * 2];
            (&DAT_00712928)[iVar19] = 0;
            bStack_69 = (&DAT_00712929)[iVar19];
            uStack_48 = 2;
          }
          else if (0 < iVar20) {
            iVar19 = iStack_68 * 0x20 + iVar9;
            local_64 = (int)(short)(&DAT_00710536)[iVar19 * 2];
            bStack_69 = (&DAT_00712928)[iVar19 * 2] + 1;
            (&DAT_00712928)[iVar19 * 2] = bStack_69;
            (&DAT_00712929)[iVar19 * 2] = 0;
            uStack_48 = 1;
          }
          if (local_64 < 0x18) {
            if (local_64 == 0x17) {
              DAT_007124bc = 1;
              _DAT_006f1d78 = *(undefined4 *)(&DAT_00710b70 + iStack_68 * 4);
              if (iVar20 < 0) {
                iVar20 = -iVar20;
              }
              _DAT_007124b8 = (float)iVar20 * 0.00024414062;
            }
            else {
              psStack_54 = (short *)iVar20;
              switch(local_64) {
              case 8:
              case 9:
                break;
              default:
switchD_0048e3e0_caseD_a:
                bVar5 = *(byte *)((int)&DAT_00712498 + local_64);
                if (*(byte *)((int)&DAT_00712498 + local_64) <= bStack_69) {
                  bVar5 = bStack_69;
                }
                *(byte *)((int)&DAT_00712498 + local_64) = bVar5;
                break;
              case 0x13:
                if (iVar20 < 0) {
                  psStack_54 = (short *)-iVar20;
                }
                fVar2 = 1.0 / _DAT_00710b58;
                if (-1.0 <= (float)(int)psStack_54 * 0.00024414062 * fVar2 + _DAT_007124ac) {
                  psStack_54 = (short *)iVar20;
                  if (iVar20 < 0) {
                    psStack_54 = (short *)-iVar20;
                  }
                  if ((float)(int)psStack_54 * 0.00024414062 * fVar2 + _DAT_007124ac <= 1.0) {
                    if (iVar20 < 0) {
                      iVar20 = -iVar20;
                    }
                    _DAT_007124ac = (float)iVar20 * 0.00024414062 * fVar2 + _DAT_007124ac;
                  }
                  else {
                    _DAT_007124ac = 1.0;
                  }
                }
                else {
                  _DAT_007124ac = -1.0;
                }
                break;
              case 0x14:
                if (iVar20 < 0) {
                  psStack_54 = (short *)-iVar20;
                }
                fVar2 = 1.0 / _DAT_00710b58;
                if (-1.0 <= _DAT_007124ac - (float)(int)psStack_54 * 0.00024414062 * fVar2) {
                  psStack_54 = (short *)iVar20;
                  if (iVar20 < 0) {
                    psStack_54 = (short *)-iVar20;
                  }
                  if (_DAT_007124ac - (float)(int)psStack_54 * 0.00024414062 * fVar2 <= 1.0) {
                    if (iVar20 < 0) {
                      iVar20 = -iVar20;
                    }
                    _DAT_007124ac = _DAT_007124ac - (float)iVar20 * 0.00024414062 * fVar2;
                  }
                  else {
                    _DAT_007124ac = 1.0;
                  }
                }
                else {
                  _DAT_007124ac = -1.0;
                }
                break;
              case 0x15:
                if (iVar20 < 0) {
                  psStack_54 = (short *)-iVar20;
                }
                fVar2 = 1.0 / _DAT_00710b5c;
                if (-1.0 <= (float)(int)psStack_54 * 0.00024414062 * fVar2 + _DAT_007124b0) {
                  psStack_54 = (short *)iVar20;
                  if (iVar20 < 0) {
                    psStack_54 = (short *)-iVar20;
                  }
                  if ((float)(int)psStack_54 * 0.00024414062 * fVar2 + _DAT_007124b0 <= 1.0) {
                    if (iVar20 < 0) {
                      iVar20 = -iVar20;
                    }
                    _DAT_007124b0 = (float)iVar20 * 0.00024414062 * fVar2 + _DAT_007124b0;
                  }
                  else {
                    _DAT_007124b0 = 1.0;
                  }
                }
                else {
                  _DAT_007124b0 = -1.0;
                }
                break;
              case 0x16:
                if (iVar20 < 0) {
                  psStack_54 = (short *)-iVar20;
                }
                fVar2 = 1.0 / _DAT_00710b5c;
                if (-1.0 <= _DAT_007124b0 - (float)(int)psStack_54 * 0.00024414062 * fVar2) {
                  psStack_54 = (short *)iVar20;
                  if (iVar20 < 0) {
                    psStack_54 = (short *)-iVar20;
                  }
                  if (_DAT_007124b0 - (float)(int)psStack_54 * 0.00024414062 * fVar2 <= 1.0) {
                    if (iVar20 < 0) {
                      iVar20 = -iVar20;
                    }
                    _DAT_007124b0 = _DAT_007124b0 - (float)iVar20 * 0.00024414062 * fVar2;
                  }
                  else {
                    _DAT_007124b0 = 1.0;
                  }
                }
                else {
                  _DAT_007124b0 = -1.0;
                }
              }
            }
          }
          else if (local_64 < 0x1b) {
            if (local_64 == 0x1a) {
              DAT_007124bc = 1;
              _DAT_006f1d74 = *(undefined4 *)(&DAT_00710b60 + iStack_68 * 4);
              if (iVar20 < 0) {
                iVar20 = -iVar20;
              }
              _DAT_007124b4 = -((float)iVar20 * 0.00024414062);
            }
            else if (local_64 == 0x18) {
              DAT_007124bc = 1;
              _DAT_006f1d78 = *(undefined4 *)(&DAT_00710b70 + iStack_68 * 4);
              if (iVar20 < 0) {
                iVar20 = -iVar20;
              }
              _DAT_007124b8 = -((float)iVar20 * 0.00024414062);
            }
            else {
              if (local_64 != 0x19) goto switchD_0048e3e0_caseD_a;
              DAT_007124bc = 1;
              _DAT_006f1d74 = *(undefined4 *)(&DAT_00710b60 + iStack_68 * 4);
              if (iVar20 < 0) {
                iVar20 = -iVar20;
              }
              _DAT_007124b4 = (float)iVar20 * 0.00024414062;
            }
          }
          else if (local_64 != 0x7fff) goto switchD_0048e3e0_caseD_a;
          sVar14 = *(short *)(aiStack_38[iStack_68] + 0x20 + iVar9 * 2);
          iVar20 = (int)sVar14;
          if (sVar14 < 0) {
            iVar20 = -iVar20;
          }
          if ((iVar20 != 0) && (local_64 != 0x7fff)) {
            sVar14 = (short)local_64;
            if ((-1 < sVar14) && (sVar14 < 0x1b)) {
              iVar20 = sVar14 * 0xc;
              *(undefined2 *)(&DAT_007127d4 + iVar20) = 3;
              *(short *)(&DAT_007127d6 + iVar20) = sVar13;
              *(undefined2 *)(&DAT_007127d8 + iVar20) = 1;
              *(short *)(&DAT_007127da + iVar20) = (short)iVar9;
              *(int *)(&DAT_007127dc + iVar20) = (int)(short)uStack_48;
            }
            DAT_0087a460 = iStack_68 + 1;
          }
        }
        iVar9 = iVar9 + 1;
      } while (iVar9 < (int)psStack_60);
    }
    local_40 = 0;
    uStack_3f = 0;
    iVar9 = 0;
    local_44 = 0;
    uStack_3d = 0;
    if (0 < iStack_50) {
      iStack_5c = iStack_68 << 7;
      piStack_58 = (int *)(aiStack_38[iStack_68] + 0x60);
      psStack_60 = (short *)(&DAT_00710744 + iStack_68 * 0x100);
      psStack_54 = (short *)(&DAT_00710742 + iStack_68 * 0x100);
      do {
        iVar20 = *piStack_58;
        if (iVar20 == -1) {
          *(undefined4 *)(&DAT_00712a28 + iStack_5c) = 0;
          *(undefined4 *)(&DAT_00712a2c + iStack_5c) = 0;
        }
        else {
          iVar19 = iStack_5c + iVar20;
          bVar5 = (&DAT_00712a28)[iVar19];
          *(undefined4 *)(&DAT_00712a28 + iStack_5c) = 0;
          uVar10 = bVar5 + 1;
          *(undefined4 *)(&DAT_00712a2c + iStack_5c) = 0;
          (&DAT_00712a28)[iVar19] = (char)uVar10;
          sVar14 = (&DAT_00710736)[iVar19];
          local_64 = (int)sVar14;
          uVar11 = (undefined2)iVar9;
          if (local_64 < 0x1b) {
            if (local_64 < 0x13) {
              if ((local_64 < 8) || (9 < local_64)) goto LAB_0048ea58;
            }
            else {
              FUN_0048ca10();
              *(undefined1 *)((int)&local_44 + iVar20) = 1;
            }
          }
          else if (local_64 == 0x7fff) {
            if (iVar20 == 7) {
              sVar17 = psStack_60[-7];
              sVar15 = *psStack_54;
            }
            else if (iVar20 == 0) {
              sVar15 = *psStack_60;
              sVar17 = (&DAT_00710738)[(iStack_68 * 0x10 + iVar9) * 8];
            }
            else {
              sVar15 = *(short *)(&DAT_00710734 + iVar19 * 2);
              sVar17 = (&DAT_00710738)[iVar20 + (iStack_68 * 0x10 + iVar9) * 8];
            }
            if ((((0x12 < sVar17) && (sVar17 < 0x1c)) && (0x12 < sVar15)) && (sVar15 < 0x1c)) {
              if (*(char *)((int)&local_44 + iVar20 + 1) == '\0') {
                FUN_0048ca10();
                *(undefined1 *)((int)&local_44 + iVar20 + 1) = 1;
                if ((-1 < sVar17) && (sVar17 < 0x1b)) {
                  iVar19 = sVar17 * 0xc;
                  *(undefined2 *)(&DAT_007127d4 + iVar19) = 3;
                  *(short *)(&DAT_007127d6 + iVar19) = sVar13;
                  *(undefined2 *)(&DAT_007127d8 + iVar19) = 2;
                  *(undefined2 *)(&DAT_007127da + iVar19) = uVar11;
                  *(int *)(&DAT_007127dc + iVar19) = iVar20 + 1;
                }
              }
              if (*(char *)((int)&uStack_48 + iVar20 + 3) == '\0') {
                FUN_0048ca10();
                *(undefined1 *)((int)&uStack_48 + iVar20 + 3) = 1;
                if ((-1 < sVar15) && (sVar15 < 0x1b)) {
                  iVar19 = sVar15 * 0xc;
                  *(undefined2 *)(&DAT_007127d4 + iVar19) = 3;
                  *(short *)(&DAT_007127d6 + iVar19) = sVar13;
                  *(undefined2 *)(&DAT_007127d8 + iVar19) = 2;
                  *(undefined2 *)(&DAT_007127da + iVar19) = uVar11;
                  *(int *)(&DAT_007127dc + iVar19) = iVar20 + -1;
                }
              }
              goto LAB_0048e988;
            }
          }
          else {
LAB_0048ea58:
            if (uVar10 < *(byte *)((int)&DAT_00712498 + local_64)) {
              uVar10 = (uint)*(byte *)((int)&DAT_00712498 + local_64);
            }
            *(char *)((int)&DAT_00712498 + local_64) = (char)uVar10;
          }
          if (local_64 != 0x7fff) {
            if ((-1 < sVar14) && (sVar14 < 0x1b)) {
              iVar19 = sVar14 * 0xc;
              *(undefined2 *)(&DAT_007127d4 + iVar19) = 3;
              *(short *)(&DAT_007127d6 + iVar19) = sVar13;
              *(undefined2 *)(&DAT_007127d8 + iVar19) = 2;
              *(undefined2 *)(&DAT_007127da + iVar19) = uVar11;
              *(int *)(&DAT_007127dc + iVar19) = iVar20;
            }
            DAT_0087a460 = iStack_68 + 1;
          }
        }
LAB_0048e988:
        psStack_60 = psStack_60 + 8;
        psStack_54 = psStack_54 + 8;
        iVar9 = iVar9 + 1;
        piStack_58 = piStack_58 + 1;
        iStack_5c = iStack_5c + 8;
      } while (iVar9 < iStack_50);
    }
    iStack_68 = iStack_68 + 1;
    if (3 < iStack_68) {
      if (DAT_007124a3 != '\x01') {
        _DAT_007124a0 = _DAT_007124a0 & 0xffffff;
      }
      if (DAT_00712498._1_1_ != '\x01') {
        DAT_00712498._0_2_ = (ushort)(byte)DAT_00712498;
      }
      if (DAT_00712498._3_1_ != '\x01') {
        DAT_00712498 = DAT_00712498 & 0xffffff;
      }
      cVar6 = FUN_0048fce0();
      if (cVar6 == '\0') {
        DAT_00712540 = 0;
      }
      if ((DAT_00710b80 != '\0') ||
         ((DAT_00710b81 != '\0' && (cVar6 = FUN_0048fd60(), cVar6 != '\0')))) {
        _DAT_007124b8 = -_DAT_007124b8;
      }
      return;
    }
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
