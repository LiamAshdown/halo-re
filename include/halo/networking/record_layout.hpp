#pragma once

#include <cstddef>

#include "networking.h"

/** Compile-time proof that the named networking record members sit at the offsets the engine code used to hard-code. */
static_assert(offsetof(network_game_session, server_name) == 0x84);
static_assert(offsetof(network_game_session, variant) == 0x104);
static_assert(offsetof(network_game_session, player_count) == 0x1a0);
static_assert(offsetof(network_game_session, salt) == 0x3a4);
static_assert(offsetof(network_game_session, session_counter) == 0x3a8);
static_assert(offsetof(network_game_session, map_loaded) == 0x3ac);
static_assert(offsetof(network_server_globals, session) == 0x08);
static_assert(offsetof(network_server_globals, machines) == 0x3b8);
static_assert(offsetof(network_server_globals, first_join_ms) == 0x9c4);
static_assert(offsetof(network_server_globals, unknown_9d0) == 0x9d0);
static_assert(offsetof(network_machine, unknown_52) == 0x52);
static_assert(offsetof(network_machine, unknown_56) == 0x56);
static_assert(offsetof(network_machine, gcd_user_id) == 0x5c);
static_assert(offsetof(network_client_globals, session) == 0xb14);
static_assert(offsetof(network_client_globals, channel) == 0xadc);
static_assert(offsetof(network_join_request, player) == 0x6e);
static_assert(offsetof(network_join_request, rate_index) == 0x6b);
static_assert(offsetof(network_channel, rate_index) == 0xa88);
static_assert(offsetof(network_channel, outgoing) == 0x10);
static_assert(offsetof(network_channel_stream, empty) == 0x1c);
