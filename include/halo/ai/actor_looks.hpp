#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include <string.h>

namespace halo::ai {

/**
 * Behaviour group "look_ops" of the actor AI: 11 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class look_ops {
public:
    explicit look_ops(datum_index value) : datum(value) {}

    void apply_queued_look_to_unit();
    uint8_t begin_vocalization(int16_t line, int16_t variant, actor_vocalization_context *context);
    void clear_vocalization();
    static int16_t dispatch_look_handler_by_posture(int16_t posture, uint32_t actor_index, void *origin, void *target, uint8_t stance_a, uint8_t check_facing, uint16_t range_class);
    uint32_t flee_look_away();
    float * get_idle_facing_range();
    static void issue_order_or_vocalize(datum_index prop_index, datum_index actor_index, datum_index vehicle_object_index, int16_t line, int16_t variant);
    int32_t look_get_wait_ticks(int16_t mode, uint32_t flags, float *deviation_table);
    static uint8_t look_pick_random_point_in_cone(void *origin, float yaw_min, float yaw_max, float pitch_min, float pitch_max, real_vector3d *base_direction, uint8_t check_obstruction, real_point3d *out);
    void look_randomize_direction(float *deviation_table, real_vector3d *base_direction);
    static int32_t lookup_small_table_entry(int16_t index);

    datum_index datum;
};

}
