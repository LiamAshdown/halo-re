#pragma once

#include <stdint.h>

/**
 * Link names of the fields of the main_globals block (types/main.h, 0x719700) that other modules raise or read
 * as separate bytes. The standalone data layer defines them under these names inside the block's address
 * range; the named references of halo::main::fields are what the engine code uses.
 */
extern "C" {
extern uint8_t unknown_00719738;
extern uint8_t unknown_0071973b;
extern uint8_t main_globals_byte_0071974f;
extern uint8_t unknown_00719769;
extern uint8_t unknown_0071976a;
}

namespace halo::main::fields {

/**
 * main_globals.reset_map: reload the current map in place on the next frame. Raised by the map reset script
 * function, the revert when no checkpoint exists and the campaign start; cleared when a session begins.
 *
 * @address 0x719738
 */
inline uint8_t &reset_map = unknown_00719738;

/**
 * main_globals.revert_map_if_allowed: revert to the last checkpoint on the next frame, but only when the
 * revert is not blocked and the game engine allows it. Raised by the cinematic abort script function.
 *
 * @address 0x71973b
 */
inline uint8_t &revert_map_if_allowed = unknown_0071973b;

/**
 * main_globals.lost_map: the player lost the map; the main loop reverts after a short delay. Cleared by the
 * map reset and revert requests.
 *
 * @address 0x71974f
 */
inline uint8_t &lost_map = main_globals_byte_0071974f;

/**
 * main_globals.time_is_running: multiplies the simulation and camera shake delta. A checkpoint write clears it
 * and the frame timer reset sets it again.
 *
 * @address 0x719769
 */
inline uint8_t &time_is_running = unknown_00719769;

/**
 * main_globals.reset_frame_timers: re-baseline both frame counters on the next frame, then set
 * time_is_running. A checkpoint write clears it before queuing and raises it afterwards.
 *
 * @address 0x71976a
 */
inline uint8_t &reset_frame_timers = unknown_0071976a;

}
