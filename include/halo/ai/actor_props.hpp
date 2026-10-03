#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "cache.h"
#include "units.h"
#include "game.h"
#include <string.h>

namespace halo::ai {

/**
 * Behaviour group "prop_ops" of the actor AI: 16 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class prop_ops {
public:
    explicit prop_ops(datum_index value) : datum(value) {}

    datum_index allocate_paired_prop(datum_index existing_prop);
    datum_index allocate_paired_prop_with_kind(datum_index existing_prop, datum_index reference_prop);
    static void apply_unit_definition_properties(datum_index actor_variant_tag, datum_index unit_index);
    void clear_perceived_props();
    void clear_recognition_history(uint8_t keep_when_typed);
    static void copy_prop_and_reset(datum_index dest_prop, datum_index src_prop);
    uint8_t danger_register_point(datum_index source_object_index, float radius, float distance, char accept_flag, uint8_t unknown_byte);
    static uint8_t danger_register_stationary_object(const actor_firing_positions *reference, datum_index actor_index, datum_index object_index, uint8_t unknown_byte);
    uint8_t find_danger_escape(int16_t *out_kind, float *out_step, real_vector3d *path_delta, uint8_t *in_danger);
    datum_index find_or_allocate_prop(uint32_t object_index, char kind);
    static datum_index find_or_create_shared_prop(datum_index object_index, datum_index actor_index, char create_if_missing, uint32_t flag);
    static datum_index find_prop_for_object(datum_index object_index, datum_index actor_index);
    datum_index get_target_prop_object_index();
    static void init_prop_from_object(datum_index object_index, datum_index actor_index, datum_index prop_index);
    static void mark_prop_seen_with_delta(datum_index object_index, datum_index actor_index, float delta, const real_vector3d *direction);
    void danger_update_reaction();

    datum_index datum;
};

}
