/**
 * Menu navigation event generation and the four UI input event queues.
 */

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>
#include "crt.h"

#include "halo/input/ui_events.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/input/state.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/system.hpp"
#include "halo/platform/time.hpp"

static void menu_direction_update(menu_repeat_state *state, uint8_t active, int32_t now_ms,
                                   int32_t virtual_key_id, uint8_t *fired)
{
    if (state->key != -1 && virtual_key_id != state->key) {
        return;
    }
    if (active) {
        if ((uint32_t)(now_ms - (int32_t)state->last_event_time) > k_input_menu_repeat_ms) {
            *fired = 1;
            state->last_event_time = now_ms;
            state->key = virtual_key_id;
        }
    } else {
        state->last_event_time = 0;
        state->key = -1;
    }
}

static void menu_direction_dispatch(int16_t action, uint8_t active, int32_t now_ms,
                                     int32_t virtual_key_id, uint8_t fired[4])
{
    switch (action) {
    case _input_action_forward:
    case _input_action_look_up:
        menu_direction_update(&halo::input::input_state().menu_repeat_states[0], active, now_ms, virtual_key_id, &fired[0]);
        break;
    case _input_action_backward:
    case _input_action_look_down:
        menu_direction_update(&halo::input::input_state().menu_repeat_states[1], active, now_ms, virtual_key_id, &fired[1]);
        break;
    case _input_action_left:
    case _input_action_look_left:
        menu_direction_update(&halo::input::input_state().menu_repeat_states[2], active, now_ms, virtual_key_id, &fired[2]);
        break;
    case _input_action_right:
    case _input_action_look_right:
        menu_direction_update(&halo::input::input_state().menu_repeat_states[3], active, now_ms, virtual_key_id, &fired[3]);
        break;
    default:
        break;
    }
}

static void push_menu_event(int16_t kind, uint8_t code, uint8_t pressed)
{
    ui_input_event event;

    memset(&event, 0, sizeof(event));
    event.kind = kind;
    event.code = code;
    event.pressed = pressed;
    halo::input::UiEvents::queue_push_event(0, &event);
}

namespace halo::input {

/**
 * Implements input menu generate events.
 *
 * @address 0x48ec50
 */
void UiEvents::menu_generate_events(void)
{
    large_integer counter;
    int32_t now_ms;
    uint8_t fired[4];
    uint8_t accept_fired;
    uint8_t back_fired;
    int32_t virtual_id;
    int32_t key_index;
    int32_t slot;
    int32_t dev;
    int32_t i;
    joystick_state *source;
    int16_t axis_value;
    int16_t axis_action;
    uint8_t axis_active;
    int32_t octant;
    uint8_t held;

    fired[0] = 0;
    fired[1] = 0;
    fired[2] = 0;
    fired[3] = 0;
    accept_fired = 0;
    back_fired = 0;

    halo::platform::read_performance_counter(&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    memset(&input_state().input_globals.states[0], 0, sizeof(input_state().input_globals.states[0]));

    virtual_id = 0;
    for (key_index = 0; key_index < k_control_keyboard_key_count; key_index++) {
        held = halo::input::input_get_key_state((int16_t)key_index);
        virtual_id++;
        switch (key_index) {
        case _input_key_escape:
            if (held == 1) {
                back_fired = 1;
            }
            break;
        case _input_key_enter:
        case _input_key_numpad_enter:
            if (held == 1) {
                accept_fired = 1;
            }
            break;
        case _input_key_up:
        case _input_key_numpad_8:
            menu_direction_update(&input_state().menu_repeat_states[0], held != 0, now_ms, virtual_id, &fired[0]);
            break;
        case _input_key_down:
        case _input_key_numpad_2:
            menu_direction_update(&input_state().menu_repeat_states[1], held != 0, now_ms, virtual_id, &fired[1]);
            break;
        case _input_key_left:
        case _input_key_numpad_4:
            menu_direction_update(&input_state().menu_repeat_states[2], held != 0, now_ms, virtual_id, &fired[2]);
            break;
        case _input_key_right:
        case _input_key_numpad_6:
            menu_direction_update(&input_state().menu_repeat_states[3], held != 0, now_ms, virtual_id, &fired[3]);
            break;
        default:
            if (input_state().keyboard_bindings[key_index] == _input_action_accept) {
                if (held == 1) {
                    accept_fired = 1;
                }
            } else if (input_state().keyboard_bindings[key_index] == _input_action_back) {
                if (held == 1) {
                    back_fired = 1;
                }
            }
            break;
        }
    }

    for (slot = 0; slot < 4; slot++) {
        dev = globals().joystick_slot_devices[slot];
        source = (dev == -1) ? (joystick_state *)0
                 : ((globals().suppressed == 0) ? &input_state().joystick_states[slot] : &input_state().joystick_neutral_state);

        if (dev != -1 && input_state().gamepad_action_buttons[slot][0] != -1 &&
            source->button_frames[input_state().gamepad_action_buttons[slot][0]] == 1) {
            accept_fired = 1;
        }
        if (dev != -1 && input_state().gamepad_action_buttons[slot][1] != -1 &&
            source->button_frames[input_state().gamepad_action_buttons[slot][1]] == 1) {
            back_fired = 1;
        }

        for (i = 0; dev != -1 && i < input_state().input_devices[dev].button_count; i++) {
            held = source->button_frames[i];
            virtual_id++;
            menu_direction_dispatch(input_state().gamepad_button_bindings[slot][i], held != 0, now_ms, virtual_id, fired);
        }

        for (i = 0; dev != -1 && i < input_state().input_devices[dev].axis_count; i++) {
            axis_value = source->axes[i];
            virtual_id++;
            if (axis_value < 0) {
                axis_action = input_state().gamepad_axis_bindings[slot][i][1];
                axis_active = axis_value < -k_input_menu_axis_threshold;
            } else if (axis_value > 0) {
                axis_action = input_state().gamepad_axis_bindings[slot][i][0];
                axis_active = axis_value > k_input_menu_axis_threshold;
            } else {
                continue;
            }
            if (axis_action != k_control_binding_unbound) {
                menu_direction_dispatch(axis_action, axis_active, now_ms, virtual_id, fired);
            }
        }

        for (i = 0; dev != -1 && i < input_state().input_devices[dev].pov_count; i++) {
            virtual_id++;
            for (octant = 0; octant < 8; octant++) {
                int16_t pov_action = input_state().gamepad_pov_bindings[slot][i][octant];
                if (pov_action != k_control_binding_unbound) {
                    menu_direction_dispatch(pov_action, octant == source->povs[i], now_ms, virtual_id, fired);
                }
            }
        }
    }

    if (fired[0] && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 8, 1);
    }
    if (fired[1] && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 9, 1);
    }
    if (fired[2] && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 0xa, 1);
    }
    if (fired[3] && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 0xb, 1);
    }
    if (accept_fired && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 0, 1);
    }
    if (back_fired && input_state().input_event_queue_active.enabled) {
        push_menu_event(3, 0xd, 1);
    }

    held = halo::input::input_get_key_state(_input_key_insert);
    if (held != 0 && input_state().input_event_queue_active.enabled) {
        held = halo::input::input_get_key_state(_input_key_insert);
        push_menu_event(3, 3, held);
    }

    held = halo::input::input_get_key_state(_input_key_delete);
    if (held != 0 && input_state().input_event_queue_active.enabled) {
        held = halo::input::input_get_key_state(_input_key_delete);
        push_menu_event(3, 2, held);
    }

    if (input_state().mouse_device != 0 && globals().suppressed == 0 && input_state().live_mouse_state.button_frames[0] != 0 &&
        input_state().input_event_queue_active.enabled) {
        push_menu_event(4, 0, input_state().live_mouse_state.button_frames[0]);
    }

    {
        uint32_t double_click_ms = GetDoubleClickTime();
        halo::platform::read_performance_counter(&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

        if (input_state().mouse_double_click_time == 0) {
            if (input_state().live_mouse_state.button_pressed[0] != 0) {
                input_state().mouse_double_click_time = now_ms;
            }
        } else if (input_state().live_mouse_state.button_pressed[0] == 0) {
            if ((uint32_t)(now_ms - input_state().mouse_double_click_time) >= double_click_ms) {
                input_state().mouse_double_click_time = 0;
            }
        } else {
            if (input_state().input_event_queue_active.enabled) {
                push_menu_event(4, 3, 1);
            }
            input_state().mouse_double_click_time = 0;
        }
    }

    if (input_state().mouse_device != 0 && globals().suppressed == 0 && input_state().live_mouse_state.button_frames[2] != 0 &&
        input_state().input_event_queue_active.enabled) {
        push_menu_event(4, 2, input_state().live_mouse_state.button_frames[2]);
    }
}

}

namespace halo::input {

/**
 * Zeroes the whole input event queue block, seeds its start_time and last_event_time from
 * QueryPerformanceCounter, then enables it.
 *
 * @address 0x492250
 */
void UiEvents::queue_initialize(void)
{
    large_integer counter;
    uint32_t *cursor;
    int32_t count;

    cursor = (uint32_t *)&input_state().input_event_queue_active;
    for (count = 0x43; count != 0; count--) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    halo::platform::read_performance_counter(&counter);
    input_state().input_event_queue_active.last_event_time = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
    input_state().input_event_queue_active.start_time = input_state().input_event_queue_active.last_event_time;
    input_state().input_event_queue_active.enabled = 1;
}

}

namespace halo::input {

/**
 * Pops the newest queued event of queue_index (0..3), or the newest across all four queues when
 * queue_index is -1. Returns 1 and fills *out_event on success, 0 (queue disabled, or nothing
 * queued) otherwise; the popped slot's kind is cleared.
 *
 * @address 0x4922b0
 */
uint8_t UiEvents::queue_pop_event(ui_input_event *out_event, int16_t queue_index)
{
    int32_t slot;
    ui_input_event *record;

    if (input_state().input_event_queue_active.enabled == 0) {
        return 0;
    }
    if (queue_index != -1) {
        for (slot = 7; slot >= 0; slot--) {
            record = &input_state().input_event_queue_active.events[queue_index][slot];
            if (record->kind != 0) {
                *out_event = *record;
                record->kind = 0;
                return 1;
            }
        }
        return 0;
    }
    for (queue_index = 0; queue_index < 4; queue_index++) {
        if (halo::input::UiEvents::queue_pop_event(out_event, queue_index) != 0) {
            return 1;
        }
    }
    return 0;
}

}

namespace halo::input {

/**
 * Pushes record onto queue queue_index (0..3): stamps record->controller_index with the queue
 * index, shifts the existing 8 slots down by one (dropping the oldest, slot 7), stores record
 * into slot 0, and refreshes last_event_time when record->kind is nonzero. A no-op while
 * push_disabled is set.
 *
 * Original register convention: queue index in EAX, record pointer in EDI.
 *
 * @address 0x492340
 */
void UiEvents::queue_push_event(int16_t queue_index, ui_input_event *record)
{
    large_integer counter;
    uint32_t now;
    ui_input_event *slots;

    if (input_state().input_event_queue_active.push_disabled == 0) {
        halo::platform::read_performance_counter(&counter);
        now = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

        record->controller_index = queue_index;
        slots = input_state().input_event_queue_active.events[queue_index];
        memmove(&slots[0], &slots[1], sizeof(ui_input_event) * 7);
        slots[0] = *record;
        if (record->kind != 0) {
            input_state().input_event_queue_active.last_event_time = now;
        }
    }
}

}

namespace halo::input {

/**
 * Refreshes input_queue_sample_time from QueryPerformanceCounter. Nothing in this image reads
 * the value back.
 *
 * @address 0x492210
 */
void UiEvents::queue_sample_time_update(void)
{
    large_integer counter;

    halo::platform::read_performance_counter(&counter);
    input_state().input_queue_sample_time = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
}

}
