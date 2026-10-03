#pragma once

#include <cstdint>

struct message_delta_context;

namespace halo::networking {

/**
 * Message type ids of the network message delta group: the first field of every game action the server sends, as passed
 * to message_delta_encode_message and dispatched by the client's action_apply switch.
 *
 * Names follow the handler that applies each message (and, for the game engine profile updates that share one handler,
 * the sender that builds it).
 */
enum class delta_message : int32_t {
    object_delete = 0x00,
    object_release_node_1 = 0x01,
    object_release_node_2 = 0x02,
    object_release_node_3 = 0x03,
    object_release_node_4 = 0x04,
    object_release_node_5 = 0x05,
    hud_item_message = 0x06,
    object_value_event = 0x07,
    unit_weapon_loadout = 0x08,
    unit_seat_exit = 0x09,
    player_interaction = 0x0a,
    player_damage_direction = 0x0b,
    unit_control_update = 0x0c,
    kill_streak_update = 0x0e,
    chat = 0x0f,
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
    unit_weapon_script = 0x1b,
    unit_spawn_starting_weapons = 0x1c,
    unit_create_update = 0x1d,
    projectile_create = 0x1e,
    equipment_create = 0x1f,
    weapon_create = 0x20,
    player_set_changed = 0x21,
    parameters_update = 0x22,
    local_player_update = 0x23,
    local_player_vehicle_update = 0x24,
    remote_player_action_update = 0x25,
    remote_player_action_apply = 0x26,
    remote_player_position_delta = 0x27,
    remote_player_vehicle_position_delta = 0x28,
    remote_player_biped_update = 0x29,
    remote_player_vehicle_update = 0x2a,
    weapon_predict_ammo = 0x2b,
    weapon_add_ammunition = 0x2c,
    weapon_ammo_correction = 0x2d,
    weapon_ammo_correction_resync = 0x2e,
    netgame_equipment_spawn = 0x2f,
    projectile_detonation = 0x30,
    object_linked_impulse = 0x31,
    object_shield_charge = 0x32,
    projectile_attach = 0x33,
    ping_field_update = 0x34,
    map_cycle_list = 0x35,
    rcon_request = 0x36,
    server_text = 0x37,
};

/** The typed view of the void ** decode context the delta handlers are called with. */
inline ::message_delta_context *delta_context(void **context) noexcept { return reinterpret_cast<::message_delta_context *>(context); }

/** The integer id the encoder takes for a message type. */
constexpr int32_t message_id(delta_message type) noexcept { return static_cast<int32_t>(type); }

}  // namespace halo::networking
