p = "C:\\Users\\Liam-\\halo-re\\src\\networking\\network_game_action_apply.c"
t = open(p, encoding="utf-8").read()
cut = t.index("\n#if 0")
new = '''// network_game_action_apply  (Ghidra: FUN_004da320; renamed, no prior name)
// address 0x4da320, size 787 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md summary ("Applies a single queued network-game
// action by type id, calling the specific per-type handler (ammo pickups, player/vehicle
// network updates, etc.)").
// blam-cc: EAX -> context, ECX -> client
// REWRITTEN 2026-09-28 (networking call audit) from the disassembly and both jump tables (client 0x4da634, 0x38
// entries; host index bytes 0x4da734 / cases 0x4da714). The context is the decode context
// network_game_action_queue_drain builds (context[0] the decode state, whose +4 is the action type;
// context[0x11] the decoded record); the client arrives in ECX and is kept in ESI. Every handler takes the context
// in EAX (or EDX / on the stack where noted); the client goes to object_type_override_call_0x70_release_node
// (stack), game_engine_invoke_profile_post_update_callback (ECX) and network_channel_key_send_state (ESI). The
// "applying" flag 0x71c2c0 is set before the dispatch in both modes and cleared after (also for an unknown type).
// When network_game_mode is 1 (client) all types run; when 2 (host) only 6, 0xb, 0xf, 0x1a, 0x21, 0x22 and 0x35. The
// previous C called ~45 handlers with no arguments at all.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int16_t network_game_mode; // 0x00719720
extern uint8_t network_action_apply_active; // 0x0071c2c0, UNSURE name

extern void object_delete_by_pooled_node_id(void **context); // 0x4f5b50, EAX
extern void object_type_override_call_0x70_release_node(void **context, network_client_globals *client); // 0x4f4680, EAX, stack
extern void hud_receive_item_message(void **context); // 0x4ae200, EAX
extern void game_engine_apply_player_join_message(void **context); // 0x4778c0, EAX
extern void game_engine_apply_player_spawn_loadout_message(void **context); // 0x477c70, EAX
extern void unit_dispatch_seat_exit_message(void **context); // 0x56c400, EAX
extern uint8_t game_engine_apply_player_interaction_message(void **context); // 0x478f10, EAX
extern void player_effect_mark_damage_direction_dispatch(void **context); // 0x456ad0, EAX
extern void unit_apply_network_control_update(void **context); // 0x566c90, EAX
extern uint8_t game_engine_apply_kill_streak_message(void **context); // 0x479b40, EAX
extern void chat_dispatch_incoming(void **context); // 0x4aaf70, EAX
extern void game_engine_invoke_profile_post_update_callback(network_client_globals *client, void **context); // 0x466e60, ECX, EDX
extern void game_engine_apply_player_profile_entry(void **context); // 0x466d00, stack
extern void game_engine_dispatch_end_game_notification(void **context); // 0x467230, EAX
extern void game_engine_apply_partial_round_reset_message(void **context); // 0x468320, EAX
extern void game_engine_handle_kill_feed_network_event(void **context); // 0x4609d0, EAX
extern void game_engine_handle_sound_status_event(void **context); // 0x46bca0, EAX
extern void game_engine_client_apply_team_assignment(void **context); // 0x470a10, EAX
extern void unit_scripting_set_or_drop_weapon(void **context); // 0x56ddb0, EAX
extern void unit_spawn_with_starting_weapons(void **context); // 0x572110, EAX (the vehicle creation receiver)
extern void unit_network_create_update_apply(void **context); // 0x55b110, EAX (the biped creation receiver)
extern void projectile_create_from_network(void **context); // 0x4c0ca0, EAX
extern void equipment_create_from_creation_message(void **context); // 0x4bbe20, EAX
extern void weapon_create_from_creation_message(void **context); // 0x4c5c10, EAX
extern int32_t network_channel_key_send_state(network_client_globals *client, void **context); // 0x4de950, ESI, EDX
extern void message_delta_parameters_protocol_receive_update(void **context); // 0x4ec000, EAX
extern void message_delta_definitions_invoke_field_bindings(void); // 0x4ec390
extern void player_update_client_local_player_update_from_network(void **context); // 0x4e5390, EAX
extern void player_update_client_local_player_vehicle_update_from_network(void **context); // 0x4e5490, EAX
extern void player_update_client_remote_player_action_update_from_network(void **context); // 0x4e5620, EDX
extern void player_update_remote_player_action_update_apply(void **context); // 0x4e5720, stack
extern void player_update_client_remote_player_position_delta_from_network(void **context); // 0x4e5c40, EAX
extern void player_update_client_remote_player_vehicle_position_delta_from_network(void **context); // 0x4e5d60, EAX
extern void player_update_client_remote_player_total_biped_update_from_network(void **context); // 0x4e5870, EDX
extern void player_update_client_remote_player_total_vehicle_update_from_network(void **context); // 0x4e5a30, EDX
extern void weapon_predict_ammo(void **context); // 0x4c3530, EAX
extern int32_t weapon_add_ammunition(void **context); // 0x4c25a0, EAX
extern void weapon_apply_ammo_correction(void **context); // 0x4c3870, EAX
extern void weapon_apply_ammo_correction_and_resync(void **context); // 0x4c4ac0, EAX
extern void game_engine_spawn_or_replay_netgame_equipment(void **context); // 0x45f8f0, EAX
extern void projectile_detonation_message_apply(void **context); // 0x4bdb40, EAX
extern void object_apply_linked_impulse(void **context); // 0x4efc80, EAX
extern void object_apply_shield_charge_and_notify(void **context); // 0x4ee4d0, EAX
extern void projectile_attach_apply(void **context); // 0x4bf1c0, EAX
extern void network_player_ping_field_update_and_report(void **context); // 0x4dbaa0, EAX
extern void network_client_handle_server_text_message(void **context); // 0x4e5140, EDX

// the actions a host applies (the rest are client-only)
static void network_game_action_apply_shared(void **context, network_client_globals *client, int32_t type_id)
{
    switch (type_id) {
    case 0x06: hud_receive_item_message(context); break;
    case 0x0b: player_effect_mark_damage_direction_dispatch(context); break;
    case 0x0f: chat_dispatch_incoming(context); break;
    case 0x1a: game_engine_client_apply_team_assignment(context); break;
    case 0x21: network_channel_key_send_state(client, context); break;
    case 0x22:
        message_delta_parameters_protocol_receive_update(context);
        message_delta_definitions_invoke_field_bindings();
        break;
    case 0x35: network_player_ping_field_update_and_report(context); break;
    }
}

void network_game_action_apply(void **context, network_client_globals *client)
{
    int32_t type_id;

    if (network_game_mode != 1 && network_game_mode != 2) {
        return;
    }
    network_action_apply_active = 1;
    type_id = ((int32_t *)context[0])[1];
    if (network_game_mode == 2) {
        network_game_action_apply_shared(context, client, type_id);
        network_action_apply_active = 0;
        return;
    }
    switch (type_id) {
    case 0x00: object_delete_by_pooled_node_id(context); break;
    case 0x01:
    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05: object_type_override_call_0x70_release_node(context, client); break;
    case 0x07: game_engine_apply_player_join_message(context); break;
    case 0x08: game_engine_apply_player_spawn_loadout_message(context); break;
    case 0x09: unit_dispatch_seat_exit_message(context); break;
    case 0x0a: game_engine_apply_player_interaction_message(context); break;
    case 0x0c: unit_apply_network_control_update(context); break;
    case 0x0e: game_engine_apply_kill_streak_message(context); break;
    case 0x10:
    case 0x11:
    case 0x12:
    case 0x13:
    case 0x14: game_engine_invoke_profile_post_update_callback(client, context); break;
    case 0x15: game_engine_apply_player_profile_entry(context); break;
    case 0x16: game_engine_dispatch_end_game_notification(context); break;
    case 0x17: game_engine_apply_partial_round_reset_message(context); break;
    case 0x18: game_engine_handle_kill_feed_network_event(context); break;
    case 0x19: game_engine_handle_sound_status_event(context); break;
    case 0x1b: unit_scripting_set_or_drop_weapon(context); break;
    case 0x1c: unit_spawn_with_starting_weapons(context); break;
    case 0x1d: unit_network_create_update_apply(context); break;
    case 0x1e: projectile_create_from_network(context); break;
    case 0x1f: equipment_create_from_creation_message(context); break;
    case 0x20: weapon_create_from_creation_message(context); break;
    case 0x23: player_update_client_local_player_update_from_network(context); break;
    case 0x24: player_update_client_local_player_vehicle_update_from_network(context); break;
    case 0x25: player_update_client_remote_player_action_update_from_network(context); break;
    case 0x26: player_update_remote_player_action_update_apply(context); break;
    case 0x27: player_update_client_remote_player_position_delta_from_network(context); break;
    case 0x28: player_update_client_remote_player_vehicle_position_delta_from_network(context); break;
    case 0x29: player_update_client_remote_player_total_biped_update_from_network(context); break;
    case 0x2a: player_update_client_remote_player_total_vehicle_update_from_network(context); break;
    case 0x2b: weapon_predict_ammo(context); break;
    case 0x2c: weapon_add_ammunition(context); break;
    case 0x2d: weapon_apply_ammo_correction(context); break;
    case 0x2e: weapon_apply_ammo_correction_and_resync(context); break;
    case 0x2f: game_engine_spawn_or_replay_netgame_equipment(context); break;
    case 0x30: projectile_detonation_message_apply(context); break;
    case 0x31: object_apply_linked_impulse(context); break;
    case 0x32: object_apply_shield_charge_and_notify(context); break;
    case 0x33: projectile_attach_apply(context); break;
    case 0x37: network_client_handle_server_text_message(context); break;
    default: network_game_action_apply_shared(context, client, type_id); break; // 6, 0xb, 0xf, 0x1a, 0x21, 0x22, 0x35
    }
    network_action_apply_active = 0;
}
'''
open(p, "w", encoding="utf-8", newline="\n").write(new + t[cut:])
print("ok")
