#pragma once

#include "halo/ai/airest_types.hpp"

namespace halo::ai {

/**
 * Top-level AI lifecycle: map initialisation and reset, the per-tick dispatcher, actor release and
 * perception bookkeeping, and the scoring helpers shared by the AI subsystems.
 */
class AiSystem {
public:
    static void actors_initialize();
    static void accumulate_repeated_event(int32_t event_type, real_point3d *position, int16_t event_id, int16_t window_ticks);
    static void alert_actors_in_grenade_radius(datum_index source_unit_index, int16_t stimulus, int16_t gate);
    static void build_priority_target_list(ai_priority_target_list *out_list);
    static void category_matches_wildcard(int16_t category, int16_t other_category);
    static int16_t count_actors_in_mode9_group(int32_t group_id);
    static void get_difficulty_request(int16_t request_code, uint8_t *out_flag_a, uint8_t *out_flag_b, float *out_value);
    static int16_t group_bucket_find_or_add(ai_group_bucket_entry *buckets, int32_t key, int16_t *count, int16_t capacity);
    static void initialize_for_new_map();
    static uint8_t insert_scored_candidate_pair(ai_scored_candidate *list, datum_index handle, float score, datum_index payload, datum_index key);
    static void mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status);
    static void notify_actors_of_encounter_state_change(int16_t zone_a, int16_t zone_b, uint8_t status, uint8_t force_update);
    static int16_t pick_weighted_candidate(ai_scored_candidate *table, ai_scored_candidate *out_entry);
    static void process_vehicle_entry_queue();
    static void recompute_all_relationship_flags();
    static void reset_all_actors_perception();
    static void reset_fire_group_assignments();
    static void reset_for_new_map();
    static int32_t scan_for_recent_combat_activity(uint8_t hard_difficulty);
    static int target_distance_qsort_compare(void *record_a, void *record_b);
    static void tick_dispatcher();
    static void unassigned_actors_attach_to_structure_bsp();
    static int32_t weighted_random_index(int16_t weight_offset, void *base, int16_t stride, uint16_t count, uint32_t *exclude_mask);
};

/**
 * Inputs shared by the aiming solvers. Pointers are owned by the caller; optional outputs may be null as in the
 * underlying solver functions.
 */
struct AimRequest {
    real_point3d *target;
    real_point3d *origin;
    real speed;
    real gravity_scale;
    real *max_time;
    uint8_t use_high_arc;
    real_vector3d *out_direction;
    real *max_speed_override;
    real *out_speed;
    real *out_time;
    real *out_range;
};

/**
 * Strategy interface for the two launch solvers: the gravity arc and the straight line. Concrete solvers are
 * stateless objects in static storage.
 */
class AimSolver {
public:
    virtual uint8_t solve(const AimRequest &request) const = 0;
    virtual uint8_t is_straight_line() const = 0;

protected:
    ~AimSolver() = default;
};

/**
 * Ballistic and straight-line aiming solvers used by AI to compute a projectile launch direction.
 */
class ProjectileAim {
public:
    static uint8_t get_aiming_vector(real_point3d *target, real *speed_in, Projectile *tag, real_point3d *origin, void *unused_param_3, real *max_time, real *max_speed_override, uint8_t use_high_arc, real_vector3d *out_direction, real *out_speed, real *out_time_or_fraction, real *out_range_or_length, uint8_t *out_used_straight_line);
    static uint8_t solve_ballistic_arc(real_point3d *target, real_point3d *origin, real speed_limit, real gravity_scale, real *max_time, uint8_t use_high_arc, real_vector3d *out_direction, real *max_speed_override, real *out_speed, real *out_time_of_flight, real *out_range, real *out_half_gravity_term, real *out_horizontal_speed);
    static uint8_t solve_straight_line(real_point3d *target, real_point3d *origin, real speed, real *out_time_of_flight, real_vector3d *out_direction, real *out_speed_echo, real *out_length);
};

}
