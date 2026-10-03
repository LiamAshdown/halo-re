#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "physics.h"
#include "units.h"
#include "game.h"

namespace halo::ai {

/**
 * Behaviour group "movement_ops" of the actor AI: 14 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class movement_ops {
public:
    explicit movement_ops(datum_index value) : datum(value) {}

    uint8_t avoid_obstacle_and_project(datum_index vehicle_index, real_point3d *entry, real_point3d *hint, uint8_t *in_out_near_line, real_point3d *out_point, int32_t *out_surface_index);
    static void avoidance_build_direction_tables();
    static uint8_t avoidance_interpolate_sample(const real_vector3d *direction, const real_vector3d *samples, int16_t count, const float *values, float *out_index, float *out_value);
    uint8_t check_step_obstruction(real_vector2d *direction, float step_distance, float step_up, uint8_t *out_flag, void *extra_param);
    uint8_t check_vehicle_mode_timeout();
    void compute_swarm_avoidance_offset(datum_index unit_index, float radius, float *out_offset);
    datum_index create_swarm();
    void delete_swarm();
    uint8_t evaluate_search_node(datum_index vehicle_index, int16_t seat_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint, float *out_score, uint8_t *out_close, uint8_t *out_facing, uint8_t *out_in_front);
    static void fill_unit_position_context(datum_index unit_index, actor_unit_position_context *out_context);
    int16_t find_best_search_node(datum_index vehicle_index, real_point3d *out_entry, real_vector3d *out_direction, real_point3d *out_hint);
    uint8_t gate_jump_traversal(int16_t threshold, char allow_broadcast, int16_t broadcast_threshold);
    uint8_t get_cached_wander_position(real_vector3d *out_position);
    static uint8_t get_requested_velocity(uint8_t skip_clamp, datum_index actor_index, real_vector3d *out_velocity, uint32_t object_index, float speed_limit);

    datum_index datum;
};

}
