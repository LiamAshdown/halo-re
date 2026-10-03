/**
 * Input system start-up, state reset and the per-frame tick.
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

#include "halo/input/system.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"

extern "C" { extern input_abstraction_globals input_globals; }
extern "C" { extern int32_t last_input_device; }
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

    memset(&input_globals, 0, sizeof(input_globals));

    last_input_device = 0;
    input_globals.unknown_2214 = 1;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    input_globals.scan_result.device_type = 0;
    input_globals.scan_result.device_index = 0;
    input_globals.scan_result.input_kind = 0;
    input_globals.scan_result.input_index = 0;
    input_globals.scan_result.direction = 0;

    input_globals.mode_flags = input_globals.mode_flags | _input_mode_game_bit;
    input_globals.unknown_2219 = 1;
}

}

extern "C" { extern di_object_data_format joystick_objects[k_input_joystick_object_count]; }
extern "C" { extern input_device input_devices[8]; }
extern "C" { extern int32_t nojoystick; }
extern "C" { extern void *direct_input; }
extern "C" { extern joystick_state joystick_neutral_state; }
extern "C" { extern int32_t joystick_slot_devices[4]; }
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
        joystick_objects[i].guid = 0;
        joystick_objects[i].offset = i * 4;
        joystick_objects[i].type = 0x80ffff03;
        joystick_objects[i].flags = 0;
    }
    for (i = 0; i < 0x10; i++) {
        joystick_objects[0x20 + i].guid = 0;
        joystick_objects[0x20 + i].offset = 0x80 + i * 4;
        joystick_objects[0x20 + i].type = 0x80ffff10;
        joystick_objects[0x20 + i].flags = 0;
    }
    for (i = 0; i < 0x20; i++) {
        joystick_objects[0x30 + i].guid = 0;
        joystick_objects[0x30 + i].offset = 0xc0 + i;
        joystick_objects[0x30 + i].type = 0x80ffff0c;
        joystick_objects[0x30 + i].flags = 0;
    }

    for (i = 0; i < 8; i++) {
        memset(&input_devices[i], 0, sizeof(input_device));
        input_devices[i].slot = -1;
    }

    if (nojoystick == 0) {
        ((idirectinput8_enumdevices_proc)(*(void ***)direct_input)[4])(direct_input, 4,
            (void *)halo::input::input_enumerate_gamepad_callback, (void *)0, 1);
    }

    memset(&joystick_neutral_state, 0, sizeof(joystick_neutral_state));
    for (i = 0; i < 0x10; i++) {
        joystick_neutral_state.povs[i] = -1;
    }

    for (i = 0; i < 4; i++) {
        joystick_slot_devices[i] = -1;
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

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
}

}

extern "C" { extern uint32_t input_menu_exit_deadline; }
extern "C" { extern int16_t system_keys[k_input_system_key_count]; }
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

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (uint32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);

    input_globals.idle = 1;
    halo::input::input_key_block_timers_expire();

    for (i = 0; i < k_input_system_key_count; i++) {
        input_globals.system_key_states[i] = halo::input::input_get_key_state(system_keys[i]);
    }

    mode = input_globals.mode_flags;
    if (mode == _input_mode_game_bit) {
        if (input_menu_exit_deadline < now_ms) {
            halo::input::input_game_action_update();
        }
    } else if ((mode & _input_mode_bind_scan_bit) != 0) {
        halo::input::input_scan_any_bound_input();
    } else if ((mode & _input_mode_keyboard_capture_bit) != 0) {
        memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));
    } else if ((mode & _input_mode_menu_bit) != 0) {
        input_menu_exit_deadline = now_ms + k_input_menu_exit_delay_ms;
        halo::input::input_menu_generate_events();
    }
}

}
