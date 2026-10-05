/**
 * Input system start-up, state reset and the per-frame tick.
 */

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>

#include "halo/input/system.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/input/state.hpp"
#include "halo/input/directinput_constants.hpp"
#include "halo/input/binding_names.hpp"
#include "halo/input/bindings.hpp"
#include "halo/input/devices.hpp"
#include "halo/input/game_actions.hpp"
#include "halo/input/ui_events.hpp"
#include "halo/platform/time.hpp"

namespace halo::input {

/**
 * One-time input subsystem initializer: zeroes the whole input abstraction block (settings,
 * states, binding tables, scan state), reseeds the millisecond time base from
 * QueryPerformanceCounter, clears the cached last-used input device, and marks the subsystem as
 * running in game mode.
 *
 * @address 0x48b3e0
 */
void InputSystem::state_initialize(void)
{
    large_integer counter;

    memset(&input_state().input_globals, 0, sizeof(input_state().input_globals));

    input_state().last_input_device = 0;
    input_state().input_globals.unknown_2214 = 1;

    halo::platform::read_performance_counter(&counter);
    input_state().input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    input_state().input_globals.scan_result.device_type = 0;
    input_state().input_globals.scan_result.device_index = 0;
    input_state().input_globals.scan_result.input_kind = 0;
    input_state().input_globals.scan_result.input_index = 0;
    input_state().input_globals.scan_result.direction = 0;

    input_state().input_globals.mode_flags = input_state().input_globals.mode_flags | _input_mode_game_bit;
    input_state().input_globals.unknown_2219 = 1;
}

}

namespace halo::input {

/**
 * Implements input system initialize.
 *
 * @address 0x491a80
 */
uint32_t InputSystem::system_initialize(void)
{
    int32_t i;

    for (i = 0; i < 0x20; i++) {
        input_state().joystick_objects[i].guid = 0;
        input_state().joystick_objects[i].offset = i * 4;
        input_state().joystick_objects[i].type = k_didft_optional_any_instance | k_didft_axis;
        input_state().joystick_objects[i].flags = 0;
    }
    for (i = 0; i < 0x10; i++) {
        input_state().joystick_objects[0x20 + i].guid = 0;
        input_state().joystick_objects[0x20 + i].offset = 0x80 + i * 4;
        input_state().joystick_objects[0x20 + i].type = k_didft_optional_any_instance | k_didft_pov;
        input_state().joystick_objects[0x20 + i].flags = 0;
    }
    for (i = 0; i < 0x20; i++) {
        input_state().joystick_objects[0x30 + i].guid = 0;
        input_state().joystick_objects[0x30 + i].offset = 0xc0 + i;
        input_state().joystick_objects[0x30 + i].type = k_didft_optional_any_instance | k_didft_button;
        input_state().joystick_objects[0x30 + i].flags = 0;
    }

    for (i = 0; i < 8; i++) {
        memset(&input_state().input_devices[i], 0, sizeof(input_device));
        input_state().input_devices[i].slot = -1;
    }

    if (input_state().nojoystick == 0) {
        halo::input::InputDevices::enumerate_joysticks();
    }

    memset(&input_state().joystick_neutral_state, 0, sizeof(input_state().joystick_neutral_state));
    for (i = 0; i < 0x10; i++) {
        input_state().joystick_neutral_state.povs[i] = -1;
    }

    for (i = 0; i < 4; i++) {
        globals().joystick_slot_devices[i] = -1;
    }

    return 0xffffff01;
}

}

namespace halo::input {

/**
 * Recomputes the millisecond input time base from QueryPerformanceCounter without touching the
 * rest of the input abstraction state.
 *
 * @address 0x48b470
 */
void InputSystem::time_base_resync(void)
{
    large_integer counter;

    halo::platform::read_performance_counter(&counter);
    input_state().input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
}

}

namespace halo::input {

/**
 * Per-frame input tick. Resyncs the millisecond time base, marks the frame idle (cleared later
 * if the game action update finds a change), expires key-block timers, refreshes the three
 * system key hold states (grave, escape, print screen), and dispatches on the current input
 * mode: run the game action update once the post-menu exit delay has passed, generate menu
 * navigation events, clear the local player's action state while the keyboard rebind-capture UI
 * owns the keyboard, or drive the bind-capture scan machine.
 *
 * @address 0x48b4b0
 */
void InputSystem::update_tick(void)
{
    large_integer counter;
    uint32_t now_ms;
    int32_t i;
    uint8_t mode;

    halo::platform::read_performance_counter(&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    input_state().input_globals.idle = 1;
    halo::input::InputDevices::key_block_timers_expire();

    for (i = 0; i < k_input_system_key_count; i++) {
        input_state().input_globals.system_key_states[i] = halo::input::input_get_key_state(input_state().system_keys[i]);
    }

    mode = input_state().input_globals.mode_flags;
    if (mode == _input_mode_game_bit) {
        if (input_state().input_menu_exit_deadline < now_ms) {
            halo::input::GameActions::game_action_update();
        }
    } else if ((mode & _input_mode_bind_scan_bit) != 0) {
        halo::input::input_scan_any_bound_input();
    } else if ((mode & _input_mode_keyboard_capture_bit) != 0) {
        memset(&input_state().input_globals.states[0], 0, sizeof(input_state().input_globals.states[0]));
    } else if ((mode & _input_mode_menu_bit) != 0) {
        input_state().input_menu_exit_deadline = now_ms + k_input_menu_exit_delay_ms;
        halo::input::UiEvents::menu_generate_events();
    }
}

}
