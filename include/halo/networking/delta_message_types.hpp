#pragma once

#include <cstdint>

namespace halo::networking {

/**
 * Message type ids of the network message delta group, as passed to message_delta_encode_message.
 *
 * Names follow the sender that builds each message; the generic player_update_part_* entries are the staged
 * pieces of the player full-resync update whose individual roles are not pinned down yet.
 */
enum class delta_message : int32_t {
    object_value_event = 7,
    unit_weapon_loadout = 8,
    player_interaction = 10,
    kill_streak_update = 0x0e,
    slayer_profiles_updated = 0x10,
    ctf_profiles_updated = 0x11,
    koth_team_scores = 0x12,
    koth_hill_times = 0x13,
    ctf_state = 0x14,
    player_profile_update = 0x15,
    end_game = 0x16,
    round_reset = 0x17,
    kill_event = 0x18,
    status_sound = 0x19,
    team_allegiance = 0x1a,
    player_set_changed = 0x21,
    parameters_update = 0x22,
    local_player_position_ack = 0x23,
    local_player_vehicle_ack = 0x24,
    player_update_part_a = 0x25,
    event_feed_flush = 0x26,
    player_update_part_b = 0x27,
    player_update_part_c = 0x28,
    player_update_part_d = 0x29,
    player_update_part_e = 0x2a,
    item_pickup_event = 0x2f,
    ping_field_update = 0x34,
    map_cycle_list = 0x35,
    rcon_request = 0x36,
    rcon_output = 0x37,
};

/** The integer id the encoder takes for a message type. */
constexpr int32_t message_id(delta_message type) noexcept { return static_cast<int32_t>(type); }

}  // namespace halo::networking
