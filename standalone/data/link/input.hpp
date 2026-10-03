/**
 * @file standalone/data/link/input.hpp
 * Link names of the engine variables the input module binds in halo::input::Globals (src/input/globals.cpp). The variables are
 * defined in standalone/data under these C names; this header is included by that one file only.
 */
#pragma once

extern "C" {
extern int32_t joystick_slot_devices[4];
extern uint8_t input_suppressed;
extern uint8_t g_control_binding_state;
extern uint8_t g_control_binding_secondary_active;
}
