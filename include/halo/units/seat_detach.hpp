/**
 * @file include/halo/units/seat_detach.hpp
 * Shared helpers for the unit seat code that used to be copied into every seat, damage and lifecycle source file.
 */
#pragma once

#include <cstdint>
#include "memory.h"

struct unit_object;

namespace halo::units {

/** Drops the local player update history of the player controlling `self` when the game runs as a server. */
void biped_free_local_player_history(unit_object *self);

/** Detaches a biped from its seat in the vehicle (position, orientation, seat bookkeeping and weapon ready), as a unit leaving a vehicle does. */
void biped_detach_from_seat(uint32_t object_index, datum_index vehicle_index);

}  // namespace halo::units
