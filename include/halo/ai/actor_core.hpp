#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include <string.h>
#include <stdint.h>

namespace halo::ai {

/**
 * Behaviour group "actor_ref" of the actor AI: 23 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class actor_ref {
public:
    explicit actor_ref(datum_index value) : datum(value) {}

    uint8_t action_has_queued_secondary();
    uint8_t apply_perception_scale(const uint8_t *zone, float *in_out_value);
    void attach_to_unit(datum_index unit_index);
    int32_t classify_communication_object_type();
    uint8_t command_list_permits_escalation();
    void command_list_reset_record(datum_index unit_index, uint16_t extra, void *component_record, int32_t secondary_record, uint32_t callback_extra);
    void delete_(uint32_t flag);
    void delete_or_release_unit(uint8_t is_dead);
    void dispatch_perception_reset();
    static void dispatch_squad_order(datum_index prop_index, const actor_squad_order_header *order, datum_index actor_index);
    void dispatch_type_vtable_0x10();
    void dispatch_type_vtable_0x18();
    void dispatch_type_vtable_0x1c(uint32_t a, uint32_t b, uint32_t c);
    void * get_actor_definition();
    void get_body_axis_vector(uint32_t unit_index, actor_axis_request *request);
    int16_t get_current_mode_combat_grade();
    static uint8_t get_ranged_attack_vector(datum_index target_prop_index, datum_index actor_index, real_vector3d *out_vector);
    uint8_t handle_death(uint8_t param_2, uint8_t param_3);
    void invoke_type_handler();
    static void iterator_new(actor_iterator_state *out_iterator, uint8_t active_only);
    static actor * iterator_next(actor_iterator_state *iterator);
    uint8_t link_to_unit_cluster(datum_index unit_index);
    static void mark_units_and_release(uint8_t use_alternate_flag, datum_index actor_index, uint8_t suppress_release);

    datum_index datum;
};

}
