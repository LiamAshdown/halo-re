#pragma once

#include "halo/game/game2_types.hpp"

namespace halo::game {

/**
 * Per-player multiplayer state: respawn eligibility, lives, round resets and team resolution.
 */
class EnginePlayers {
public:
    static void player_changed_object(uint32_t param);
    static uint8_t player_has_respawn_priority(uint32_t player_index);
    static uint8_t player_is_eliminated(uint32_t player_index);
    static void player_new_life(uint32_t player_handle);
    static uint8_t player_ready_to_respawn(uint32_t player_index);
    static uint8_t player_respawn_priority_gate(uint32_t player_index);
    static void player_round_reset(int32_t player_handle, int32_t callback_argument);
    static void player_select_random_target(datum_index player_or_all);
    static uint8_t players_ready_for_bsp_switch(void);
    static uint8_t players_ready_for_bsp_switch_strict(void);
    static void reset_all_players(void);
    static void reset_player_look_state(void);
    static void reset_player_profile_stats(void);
    static void reset_respawns_and_cleanup_bipeds(void);
    static void resolve_player_team(uint32_t player_index);
    static void reattach_player_unit_unused(uint32_t player_index, uint32_t target_object, void *local_offset);
    static int32_t pick_random_recent_location(int32_t exclude_value, int32_t fallback);
    static void reset_all_unit_grenade_counts(void);
    static uint8_t team_has_scoring_capacity(int32_t team);
    static uint8_t scores_tracked_individually(void);
};

/**
 * Client/server player update, profile cache and local control paths of the multiplayer engine.
 */
class EnginePlayerSync {
public:
    static void players_update_client(void);
    static void players_update_server(void);
    static void server_update_player_positions(void);
    static void send_player_profile_update(const int32_t *machine_hash, const void *cached_profile, int32_t commit, const void *current_profile, int32_t target);
    static void send_unit_weapon_loadout(uint32_t unit_index, datum_index player_handle, int32_t value, int32_t machine_index);
    static void update_local_player_control(int16_t local_player_index, real delta_time, int32_t ticks_this_frame);
    static void update_local_player_look(int16_t local_player_index, real yaw_delta, real pitch_delta);
    static void player_profile_cache_add(datum_index player_handle);
    static int32_t player_profile_cache_find(datum_index player_handle);
    static void player_profile_cache_sync_all(int32_t commit, void *callback_extra_arg);
    static void spawn_player_starting_loadout(uint32_t starting_equipment_index, int32_t *frag_count, int32_t *plasma_count);

private:
    static int32_t pooled_node_hash_lookup(int32_t key);
    static real look_wrap_angle(real a);
};

}
