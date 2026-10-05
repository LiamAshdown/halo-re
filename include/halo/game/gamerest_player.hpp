#pragma once

#include <stdint.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "units.h"
#include "networking.h"
#include "items.h"
#include "effects.h"
#include "camera.h"

namespace halo::game {

/**
 * Non-owning handle to a player record: carries only the player datum index and forwards to the
 * behaviour that used to be a free function taking it first.
 */
class PlayerView {
public:
    uint32_t player_index;

    explicit constexpr PlayerView(uint32_t player_index_) : player_index(player_index_) {}

    void apply_pickup_effect(uint32_t pickup_object);
    uint8_t attach_unit_to_parent(uint32_t target_object, void *local_offset);
    void check_assassination_opportunity(uint32_t candidate_object);
    void check_vehicle_boarding_interaction(uint32_t candidate_object);
    void check_vehicle_boarding_interaction_lightweight(uint32_t candidate_object);
    void check_vehicle_interaction(uint32_t candidate_object);
    uint8_t execute_pending_interaction();
    uint8_t execute_weapon_drop_interaction();
    uint8_t find_placement_position(datum_index target_object, real_point3d *point);
    void kill_and_release_unit(int32_t respawn_timer_override);
    void release_unit_and_reset(int32_t previous_unit_override);
    void reset_after_unit_change();
    void respawn();
    void trigger_full_health_effect();
    void trigger_shield_recharge_effect();
    uint8_t swap_to_weapon(datum_index target_weapon);
    void update_nearby_interactions_primary();
    void update_nearby_interactions_secondary();
    void compute_view_forward_vector(real *yaw_pitch, real_vector3d *out_forward);
    int16_t pick_random_starting_location();
    uint8_t unit_has_parent();
    void set_pending_interaction_action(int16_t priority_type, int16_t seat, uint32_t candidate_object);
    uint8_t is_busy_with_interaction(uint32_t candidate_object);
    uint8_t current_weapon_prevents_camo_depower();
    uint8_t has_must_be_readied_weapon();
    void reset_gauge_if_flagged();
    void update_active_camouflage_depower();
};

/**
 * Per-player kill-streak and multikill-medal bookkeeping.
 */
class KillStreak {
public:
    uint32_t player_handle;

    explicit constexpr KillStreak(uint32_t player_handle_) : player_handle(player_handle_) {}

    uint8_t add_kill_streak(int32_t slot, int16_t amount);
    void advance_multikill_medal();
    void begin(int16_t slot);
    void continue_streak(int16_t slot);
    void set_max(int16_t slot, int16_t value);
    void tick();
    void notify_kill_streak_update(int32_t slot, int16_t amount);
    void trigger_kill_streak_effect();
};

/**
 * Handle to a unit that a local player controls: weapon index, zoom level and starting profile.
 */
class LocalPlayerUnit {
public:
    datum_index unit;

    explicit constexpr LocalPlayerUnit(datum_index unit_) : unit(unit_) {}

    int32_t get_local_player_weapon_index();
    void set_local_player_weapon_index(int16_t weapon_index);
    void invalidate_local_player_zoom_level();
    uint8_t get_current_weapon_autoaim_cone(int16_t require_zoomed, real *out);
    void apply_starting_profile(int16_t starting_profile_index, uint8_t reset_stats);
};

/**
 * Non-owning view of an object record for unit position fixups.
 */
class ObjectView {
public:
    object * obj;

    explicit constexpr ObjectView(object * obj_) : obj(obj_) {}

    void snap_position_if_far(real_point3d *new_position);
};

/**
 * Facade over the global player data array: creation, deletion, queries and per-tick catch-up.
 */
class Players {
public:
    Players() = delete;

    static datum_index new_local(datum_index requested_handle, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record);
    static datum_index new_network(datum_index requested_index, uint32_t machine_index, int16_t local_player_index, uint16_t *identifier_record);
    static void delete_player(uint32_t machine_index, datum_index player_handle);
    static void remove_player(datum_index player_handle);
    static datum_index index_from_unit_index(datum_index unit_index);
    static void set_team_by_color(uint8_t new_team, int8_t target_team_index_desired);
    static datum_index spawn_starting_profile_weapon(TagDependency *weapon_dependency, int16_t rounds_loaded, int16_t rounds_reserved, uint32_t role);
    static int32_t active_count();
    static uint8_t any_pending_seat_or_respawn();
    static uint8_t any_with_local_player_index(int16_t local_player_index);
    static uint8_t any_without_unit();
    static void client_catchup_on_server_updates();
    static void dispose();
    static datum_index find_local_owned_unclear();
    static uint32_t get_active_by_index(int32_t index);
    static void handle_deleted_unit(uint32_t object_index);
    static void initialize();
    static void rebind_local_player_after_load();
    static void server_catchup_on_client_updates();
};

/**
 * Structure-bsp switching and the player regrouping that follows it.
 */
class StructureBsp {
public:
    StructureBsp() = delete;

    static void switch_structure_bsp();
    static void switch_regroup();
};

/**
 * Facade over the local-player slots.
 */
class LocalPlayers {
public:
    LocalPlayers() = delete;

    static int32_t find_free_slot_index();
    static int32_t get_zoom_level(int16_t local_player_index);
    static void set_controlled_unit(datum_index new_unit, int16_t local_player_index);
    static datum_index to_player_index(int16_t local_player_index);
    static uint8_t any_within_10_units(const real_point3d *query_point);
};

}  // namespace halo::game
