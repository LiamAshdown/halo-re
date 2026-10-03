#pragma once

/* Include after the engine type headers (types/*.h carry no include guards). */

namespace halo::game::engine1 {

/**
 * Network message handlers and gameplay event notifications of the game engine. Stateless service class: every
 * function is a static member and the state it acts on lives in the engine globals.
 */
class Notifications {
public:
    static void apply_partial_round_reset_message(void *event);
    static void apply_player_grenade_counts(uint32_t player_index);
    static uint8_t apply_player_interaction_message(void **envelope);
    static void apply_player_join_message(void **envelope);
    static void apply_player_spawn_loadout_message(void **envelope);
    static void client_apply_team_assignment(void **envelope);
    static void dispatch_end_game_notification(void *event);
    static void dispatch_item_pickup_event(int32_t machine_id, int32_t picked_tag, int32_t param_2);
    static int32_t get_multiplayer_sound_duration_ticks(int32_t sound_index);
    static void handle_sound_status_event(void *event);
    static void multiplayer_sound_queue_tick(void);
    static void notify_item_expired(datum_index object_index);
    static void notify_object_value_event(uint8_t value_byte, int32_t hash_key, int32_t machine_index, void *subject);
    static void notify_player_interaction(uint32_t primary_key, uint32_t edi_key, uint32_t mode, int32_t interaction_type, int32_t interaction_seat, int32_t secondary_key);
    static uint8_t notify_weapon_ready_state_change(datum_index unit_index, datum_index weapon_index);
};

}
