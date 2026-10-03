/**
 * @file include/halo/ai/modes.hpp
 * The actor mode numbers: the row index into the 0x38-stride mode definition table and the value of actor.mode.
 */
#pragma once

#include <stdint.h>
#include "halo/ai/api.hpp"

namespace halo::ai {

/**
 * The behaviour mode of an actor. A mode with an actor_mode_<name>_data view in types/ai.h (flee, uncover, guard, search,
 * wait, vehicle, charge, obey, converse) keeps its state in actor.mode_data through that view; the numbers follow the
 * table order of the engine's mode enumeration.
 */
enum class actor_mode : int16_t {
    none = 0,
    sleep = 1,
    alert = 2,
    fight = 3,
    flee = 4,
    uncover = 5,
    guard = 6,
    search = 7,
    wait = 8,
    vehicle = 9,
    charge = 10,
    obey = 11,
    converse = 12,
    avoid = 13,
};

/** Compares the raw mode field of an actor with a mode name. */
inline constexpr bool operator==(int16_t raw, actor_mode mode)
{
    return raw == static_cast<int16_t>(mode);
}

/** Compares the raw mode field of an actor with a mode name. */
inline constexpr bool operator!=(int16_t raw, actor_mode mode)
{
    return raw != static_cast<int16_t>(mode);
}

/** Switches the actor into `mode`, copying the mode's initial `mode_data`. */
inline void actor_set_mode(datum_index actor_index, actor_mode mode, void *mode_data)
{
    actor_set_mode(actor_index, static_cast<int32_t>(mode), mode_data);
}

}  // namespace halo::ai
