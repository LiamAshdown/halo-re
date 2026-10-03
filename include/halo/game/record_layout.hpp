#pragma once

#include <cstddef>

#include "halo/game/api.hpp"
#include "halo/networking/api.hpp"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"

/** Compile-time proof that the named record members sit at the offsets the engine code used to hard-code. */
static_assert(offsetof(data_array, actual_count) == 0x30);
static_assert(offsetof(data_array, maximum_count) == 0x20);
static_assert(offsetof(data_array, size) == 0x22);
static_assert(offsetof(network_machine, unknown_52) == 0x52);
static_assert(offsetof(network_machine, unknown_56) == 0x56);
static_assert(offsetof(network_server_globals, join_finalize_pending) == 0x9f8);
static_assert(offsetof(network_server_globals, last_stamp_ms) == 0x9c0);
static_assert(offsetof(network_server_globals, session.unknown_07e) == 0x86);
static_assert(offsetof(network_server_globals, session.unknown_080) == 0x88);
static_assert(offsetof(network_server_globals, state) == 0x4);
static_assert(offsetof(network_server_globals, update_tick) == 0x9b8);
static_assert(offsetof(player, baseline_update_id) == 0xec);
static_assert(offsetof(player, deaths) == 0xae);
static_assert(offsetof(player, last_position_update_id) == 0x160);
static_assert(offsetof(player, last_update_id) == 0xe8);
static_assert(offsetof(player, objective_score) == 0xc8);
static_assert(offsetof(player, position_baseline_x) == 0x164);
static_assert(offsetof(player, position_baseline_y) == 0x168);
static_assert(offsetof(player, position_updates.capacity) == 0x170);
static_assert(offsetof(player, position_updates.read_index) == 0x180);
static_assert(offsetof(player, position_updates.record_size) == 0x174);
static_assert(offsetof(player, position_updates.write_index) == 0x17c);
static_assert(offsetof(player, slayer_target) == 0x88);
static_assert(offsetof(player, team) == 0x20);
static_assert(offsetof(player, unit) == 0x34);
static_assert(offsetof(player, unknown_f0) == 0xf0);
static_assert(offsetof(player, unknown_f4) == 0xf4);
static_assert(offsetof(player, unknown_f8) == 0xf8);
static_assert(offsetof(player, update_history.queue.capacity) == 0x120);
static_assert(offsetof(player, update_history.queue.record_size) == 0x124);
static_assert(offsetof(unit_object, unit.controlling_player) == 0x218);
static_assert(offsetof(unit_object, unit.driver_unit_index) == 0x324);
static_assert(offsetof(unit_object, unit.gunner_unit_index) == 0x328);
static_assert(offsetof(unit_object, unit.last_parent_object_index) == 0x32c);
static_assert(offsetof(unit_object, unit.last_seat_change_tick) == 0x330);
static_assert(offsetof(unit_object, unit.vehicle_seat_index) == 0x2f0);
