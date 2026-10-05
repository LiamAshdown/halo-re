#pragma once

#include <stdint.h>
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/main/vars.hpp"
#include "halo/core/shared_links.hpp"

/**
 * Link names of the fields of the main_globals block (types/main.h, 0x719700) that other modules raise or read
 * as separate bytes. The standalone data layer defines them under these names inside the block's address
 * range; the named references of halo::main::fields are what the engine code uses.
 */
static auto &revert_map = halo::link::ref<uint8_t>(halo::main::vars().revert_map);
static auto &revert_map_if_allowed = halo::link::ref<uint8_t>(halo::main::vars().revert_map_if_allowed);
static auto &lost_map = halo::link::ref<uint8_t>(halo::main::vars().lost_map);
static auto &time_is_running = halo::link::ref<uint8_t>(halo::main::vars().time_is_running);
static auto &reset_frame_timers = halo::link::ref<uint8_t>(halo::main::vars().reset_frame_timers);

namespace halo::main::fields {

/**
 * main_globals.reset_map: reload the current map in place on the next frame. Raised by the map reset script
 * function, the revert when no checkpoint exists and the campaign start; cleared when a session begins.
 *
 * @address 0x719738
 */
static uint8_t &reset_map = ::reset_map;

/**
 * main_globals.revert_map: revert to the last checkpoint on the next frame. Raised by the revert script function and
 * the revert menu action.
 *
 * @address 0x71973a
 */
static uint8_t &revert_map = ::revert_map;

/**
 * main_globals.revert_map_if_allowed: revert to the last checkpoint on the next frame, but only when the
 * revert is not blocked and the game engine allows it. Raised by the cinematic abort script function.
 *
 * @address 0x71973b
 */
static uint8_t &revert_map_if_allowed = ::revert_map_if_allowed;

/**
 * main_globals.lost_map: the player lost the map; the main loop reverts after a short delay. Cleared by the
 * map reset and revert requests.
 *
 * @address 0x71974f
 */
static uint8_t &lost_map = ::lost_map;

/**
 * main_globals.time_is_running: multiplies the simulation and camera shake delta. A checkpoint write clears it
 * and the frame timer reset sets it again.
 *
 * @address 0x719769
 */
static uint8_t &time_is_running = ::time_is_running;

/**
 * main_globals.reset_frame_timers: re-baseline both frame counters on the next frame, then set
 * time_is_running. A checkpoint write clears it before queuing and raises it afterwards.
 *
 * @address 0x71976a
 */
static uint8_t &reset_frame_timers = ::reset_frame_timers;

}
