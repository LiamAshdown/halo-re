// actor_score_firing_positions_near_target  (Ghidra: FUN_00411840, unnamed)
// address 0x411840, size 305 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x411840..0x411970 (row 5 of the scoring table 0x006555c0, kinds 0x0020). For every valid
//   candidate: +5 inside half the maximum distance (query +0x18), falling linearly to 0 at the maximum
//   ((max - d) / (max / 2) * 5); then, with a target, +(20 - distance_from_target) / 2 when that is under 20
//   (the score is rewritten from the pre-bonus sum); then, with query +0x43 and the vault point flag +0x648,
//   the dot of the candidate's direction_from_target with the query vector at +0x64c gives +10 above
//   cos 45 degrees, else max(0, dot * sqrt2) * 10.
// blam-cc: stack -> actor_index, query, count, candidates
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

void actor_score_firing_positions_near_target(datum_index actor_index, actor_firing_position_query *query,
    uint16_t count, actor_firing_position_candidate *candidates)
{
    int32_t i;

    (void)actor_index;
    if ((int16_t)count <= 0) {
        return;
    }
    for (i = 0; i < (int32_t)count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        float value = 0.0f;
        float base;

        if (c->valid == 0) {
            continue;
        }
        if (query->maximum_distance * 0.5f > c->distance_from_actor) {
            value = 5.0f;
        } else if (!(c->distance_from_actor >= query->maximum_distance)) {
            value = (1.0f / (query->maximum_distance * 0.5f)) * (query->maximum_distance - c->distance_from_actor) * 5.0f;
        }
        base = value + c->score;
        c->score = base;
        if (query->have_target == 0) {
            continue;
        }
        if (!(c->distance_from_target >= 20.0f)) {
            c->score = (20.0f - c->distance_from_target) * 0.5f + base;
        }
        if (query->want_direction_from_target == 0 || query->have_target_vault_point == 0) {
            continue;
        }
        {
            float dot = c->direction_from_target.k * query->target_vault_point.z +
                        c->direction_from_target.i * query->target_vault_point.x +
                        query->target_vault_point.y * c->direction_from_target.j;

            if (dot > 0.70710677f) {
                value = 10.0f;
            } else {
                value = dot * 1.4142135f;
                if (0.0f > value) {
                    value = 0.0f;
                }
                value = value * 10.0f;
            }
            c->score = value + c->score;
        }
    }
}
