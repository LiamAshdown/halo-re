#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "projectiles.h"
#include <stdint.h>

namespace halo::ai {

/**
 * Behaviour group "grenade_ops" of the actor AI: 18 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class grenade_ops {
public:
    explicit grenade_ops(datum_index value) : datum(value) {}

    void attempt_grenade_throw();
    uint8_t can_throw_grenade_at_target();
    uint8_t check_grenade_facing_and_commit(uint8_t force_commit);
    uint32_t commit_grenade_toss(real_point3d *point, uint32_t object_handle, uint32_t exclude_object_index);
    uint32_t compute_grenade_aim_direction(real_point3d *target_point, real_vector3d *out_direction, float *out_698);
    uint8_t consider_grenade_throw();
    uint8_t evaluate_grenade_target_position();
    uint8_t find_grenade_landing_spot(real_point3d *out_point, datum_index *out_target_handle, int32_t *out_relationship);
    int32_t find_nearest_grenade_ally(uint8_t widen_search);
    static int16_t gather_nearby_grenade_targets(datum_index source_actor_index, int16_t maximum_count, ai_grenade_avoidance_entry *out_entries);
    static uint8_t get_grenade_launch_velocity(int16_t grenade_type, real_vector3d *direction, void *origin, float range, real_point3d *point, int32_t max_time, float *speed, void *out_time_or_fraction, real_vector3d *out_velocity, float *out_gravity);
    static void avoidance_entry_init(ai_grenade_avoidance_entry *entry, datum_index object_index, datum_index prop_index);
    uint8_t behavior_kind_allowed(int16_t kind);
    static uint8_t parabolic_path_clear(real_vector3d *initial_velocity, datum_index source_actor_index, real_point3d *start_position, real total_time, real vertical_acceleration, datum_index exclude_object_index, uint8_t wide_mask);
    int32_t trace_from_source(real_point3d *target_point);
    static uint8_t trajectory_blocked(real_vector3d *trajectory_direction, datum_index source_actor_index, datum_index exclude_object_index, real_point3d *landing_position, int32_t *out_blocking_prop);
    uint32_t compute_grenade_throw_vector(real_point3d *grenade_position, real_vector3d *out_vector);
    static void died_unit_grenade_count_mod(object *unit_object, const ActorVariant *actor_tag_data, datum_index weapon_object_index, datum_index actor_index, datum_index encounter_index);

    datum_index datum;
};

}
