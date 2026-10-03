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
static_assert(offsetof(network_bandwidth_graph, border[0].color) == 0x50);
static_assert(offsetof(network_bandwidth_graph, border[0].x) == 0x44);
static_assert(offsetof(network_bandwidth_graph, border[0].y) == 0x48);
static_assert(offsetof(network_bandwidth_graph, border[1].color) == 0x68);
static_assert(offsetof(network_bandwidth_graph, border[1].x) == 0x5c);
static_assert(offsetof(network_bandwidth_graph, border[1].y) == 0x60);
static_assert(offsetof(network_bandwidth_graph, border[2].color) == 0x80);
static_assert(offsetof(network_bandwidth_graph, border[2].x) == 0x74);
static_assert(offsetof(network_bandwidth_graph, border[2].y) == 0x78);
static_assert(offsetof(network_bandwidth_graph, border[3].color) == 0x98);
static_assert(offsetof(network_bandwidth_graph, border[3].x) == 0x8c);
static_assert(offsetof(network_bandwidth_graph, border[3].y) == 0x90);
static_assert(offsetof(network_bandwidth_graph, border[4].color) == 0xb0);
static_assert(offsetof(network_bandwidth_graph, border[4].x) == 0xa4);
static_assert(offsetof(network_bandwidth_graph, border[4].y) == 0xa8);
static_assert(offsetof(network_bandwidth_graph, height) == 0x18);
static_assert(offsetof(network_bandwidth_graph, layout[0]) == 0x2c);
static_assert(offsetof(network_bandwidth_graph, layout[10]) == 0x40);
static_assert(offsetof(network_bandwidth_graph, layout[11]) == 0x42);
static_assert(offsetof(network_bandwidth_graph, layout[1]) == 0x2e);
static_assert(offsetof(network_bandwidth_graph, layout[2]) == 0x30);
static_assert(offsetof(network_bandwidth_graph, layout[3]) == 0x32);
static_assert(offsetof(network_bandwidth_graph, layout[4]) == 0x34);
static_assert(offsetof(network_bandwidth_graph, layout[5]) == 0x36);
static_assert(offsetof(network_bandwidth_graph, layout[6]) == 0x38);
static_assert(offsetof(network_bandwidth_graph, layout[7]) == 0x3a);
static_assert(offsetof(network_bandwidth_graph, layout[8]) == 0x3c);
static_assert(offsetof(network_bandwidth_graph, layout[9]) == 0x3e);
static_assert(offsetof(network_bandwidth_graph, width) == 0x14);
static_assert(offsetof(network_bandwidth_graph, window_left) == 0x24);
static_assert(offsetof(network_bandwidth_graph, x_scale) == 0x1c);
static_assert(offsetof(network_bandwidth_graph, y_scale) == 0x20);
static_assert(offsetof(network_client_globals, connect_attempt.session_info[5]) == 0xb02);
static_assert(offsetof(network_client_globals, connect_attempt.session_info[6]) == 0xb06);
static_assert(offsetof(network_client_globals, connect_attempt.session_info[7]) == 0xb0a);
static_assert(offsetof(network_client_globals, connect_attempt.session_info[8]) == 0xb0e);
static_assert(offsetof(network_game_session, variant.game_engine_index) == 0x134);
static_assert(offsetof(network_machine, gcd_user_id) == 0x5c);
static_assert(offsetof(network_machine, unknown_52) == 0x52);
static_assert(offsetof(network_machine, unknown_56) == 0x56);
static_assert(offsetof(network_server_globals, first_join_ms) == 0x9c4);
static_assert(offsetof(network_server_globals, handshake_blocked) == 0x9d5);
static_assert(offsetof(network_server_globals, handshake_flag) == 0x9d6);
static_assert(offsetof(network_server_globals, handshake_state) == 0x9d4);
static_assert(offsetof(network_server_globals, join_finalize_pending) == 0x9f8);
static_assert(offsetof(network_server_globals, last_challenge_sent_ms) == 0x9bc);
static_assert(offsetof(network_server_globals, last_stamp_ms) == 0x9c0);
static_assert(offsetof(network_server_globals, new_server_pending) == 0x9fa);
static_assert(offsetof(network_server_globals, scenario_announced) == 0x9f9);
static_assert(offsetof(network_server_globals, session.unknown_07e) == 0x86);
static_assert(offsetof(network_server_globals, session.unknown_080) == 0x88);
static_assert(offsetof(network_server_globals, state) == 0x4);
static_assert(offsetof(network_server_globals, unknown_9d0) == 0x9d0);
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
static_assert(offsetof(s_network_address, ipv6_1) == 0x4);
static_assert(offsetof(unit_object, unit.controlling_player) == 0x218);
static_assert(offsetof(unit_object, unit.driver_unit_index) == 0x324);
static_assert(offsetof(unit_object, unit.gunner_unit_index) == 0x328);
static_assert(offsetof(unit_object, unit.last_parent_object_index) == 0x32c);
static_assert(offsetof(unit_object, unit.last_seat_change_tick) == 0x330);
static_assert(offsetof(unit_object, unit.vehicle_seat_index) == 0x2f0);
static_assert(offsetof(network_game_announcement, info) == 0xd0);
static_assert(offsetof(network_game_announcement, game_engine_index) == 0x154);
static_assert(offsetof(network_game_announcement, flags) == 0x15e);
static_assert(sizeof(network_game_announcement) == 0x160);
static_assert(offsetof(network_bandwidth_graph, left) == 0x26);
static_assert(offsetof(network_bandwidth_graph, layout) == 0x2c);
static_assert(offsetof(network_bandwidth_graph, border) == 0x44);
static_assert(offsetof(network_bandwidth_graph, bits_sent) == 0xbc);
static_assert(sizeof(network_bandwidth_graph) == 0x23e0);
static_assert(sizeof(unit_state_snapshot) == 0xd0);
static_assert(sizeof(vehicle_state_snapshot) == 0x314);
static_assert(offsetof(player_update_history_node, unit_state) == 0x30);
static_assert(offsetof(player_update_history_node, vehicle_state) == 0x100);
static_assert(offsetof(vehicle_state_snapshot, forward) == 0x94);
static_assert(offsetof(vehicle_state_snapshot, driver_seat_power) == 0x214);
static_assert(offsetof(unit_state_snapshot, ground_surface_index) == 0xcc);
static_assert(offsetof(unit_state_snapshot, biped_flags) == 0xa8);
