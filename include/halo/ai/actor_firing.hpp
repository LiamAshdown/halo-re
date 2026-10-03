#pragma once

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cseries.h"
#include "objects.h"
#include "cache.h"
#include "units.h"

namespace halo::ai {

/**
 * Behaviour group "firing_position_ops" of the actor AI: 10 routines recovered from the original engine,
 * grouped around the actor record they operate on. Instance members act on the actor datum the object
 * was built from; static members take their operands explicitly.
 */
class firing_position_ops {
public:
    explicit firing_position_ops(datum_index value) : datum(value) {}

    int16_t claim_firing_position(datum_index previous_owner, path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok);
    uint32_t find_best_firing_position(actor_firing_position_query *query, actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context, uint8_t *out_path_ok);
    static uint8_t firing_position_compare(int32_t element, int32_t other);
    static uint8_t firing_position_evaluate(actor_firing_position_candidate *candidate, actor_firing_position_query *query, datum_index actor_index);
    uint8_t firing_position_near_point(real_point3d *point, int32_t start_surface_index, int16_t kind);
    static uint8_t firing_position_probe_reject_rules(actor_firing_position_query *query, datum_index actor_index);
    uint8_t firing_position_run_reject_rules(actor_firing_position_query *query, actor_firing_position_candidate *candidate);
    void firing_position_run_score_rules(uint16_t count, actor_firing_position_query *query, actor_firing_position_candidate *candidates);
    uint32_t get_firing_position_group_mask(int16_t kind, int16_t search_override);
    void get_firing_positions(actor_firing_positions *out_block, real_point3d *query_point);

    datum_index datum;
};

}
