#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include <string.h>
#include "units.h"

namespace halo::ai {

/**
 * Behaviour group "order_builder" of the actor AI: 16 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class order_builder {
public:
    explicit order_builder(datum_index value) : datum(value) {}

    uint8_t build_guard_mode_data(uint8_t *out);
    int32_t default_(int16_t order_code, actor_order *order, int16_t parameter);
    uint32_t face_seat_marker(int16_t firing_position_index, uint32_t *order);
    uint32_t face_seat_marker_committed(int16_t firing_position_index, uint8_t byte_a, uint32_t *order);
    int32_t flee(uint8_t byte_a, uint32_t *order);
    static int32_t grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base, uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order);
    int32_t guard(actor_order *order, int16_t guard_at_current_position);
    static uint8_t investigate_encounter_point(uint32_t vehicle_index, uint32_t actor_index, int16_t seat_index, uint8_t *order);
    int32_t look(actor_order *order, actor_look_request *request);
    int32_t minimal_stop(uint32_t *order);
    int32_t random_wait(uint8_t byte_a, uint32_t *order);
    int32_t return_to_anchor(actor_order *order);
    static uint8_t search_object(uint32_t vehicle_index, uint32_t actor_index, float radius_a, float radius_b, uint8_t *order);
    int32_t search_wait(actor_order *order);
    int32_t wait_byte(uint8_t byte_a, uint32_t *order);
    void build_path_find_request(path_find_request *request);

    datum_index datum;
};

}
