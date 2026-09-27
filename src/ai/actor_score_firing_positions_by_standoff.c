// actor_score_firing_positions_by_standoff  (Ghidra: FUN_00411980, unnamed)
// address 0x411980, size 478 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x411980..0x411b5d (row 4 of the scoring table 0x006555c0, kinds 0x0002). For every valid
//   candidate (a rejection sets rejected = 1 and, unless query.collect_all, clears valid and ends that candidate):
//   - closer than 4 to the actor is rejected (with collect_all the distance score is skipped); under 8 it earns
//     (d - 4) * 2, under the maximum distance (max - d) * 8 / (max - 8);
//   - with a target: a squared target distance under 16 is rejected (with collect_all it earns 0), under 49 it earns
//     (sqrt - 4) * 3.33, beyond that 10;
//   - with a finite segment distance and a positive finite target distance: r = 1 - segment / (0.8 * target
//     distance); r > 0.5 is rejected, otherwise the score becomes the post-target sum + (1 - clamp(r, 0, 1)) * 8.
// blam-cc: stack -> actor_index, query, count, candidates
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern double sqrt(double x); // FSQRT

void actor_score_firing_positions_by_standoff(datum_index actor_index, actor_firing_position_query *query,
    uint16_t count, actor_firing_position_candidate *candidates)
{
    int32_t i;

    (void)actor_index;
    if ((int16_t)count <= 0) {
        return;
    }
    for (i = 0; i < (int32_t)count; i++) {
        actor_firing_position_candidate *c = &candidates[i];
        float base;
        float r;

        if (c->valid == 0) {
            continue;
        }
        if (!(c->distance_from_actor >= 4.0f)) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        } else {
            float value = 0.0f;

            if (!(c->distance_from_actor >= 8.0f)) {
                value = c->distance_from_actor - 4.0f;
                value = value + value;
            } else if (!(c->distance_from_actor >= query->maximum_distance)) {
                value = (query->maximum_distance - c->distance_from_actor) * 8.0f / (query->maximum_distance - 8.0f);
            }
            c->score = value + c->score;
        }

        if (query->have_target == 0) {
            continue;
        }
        if (!(c->distance_squared_to_target >= 16.0f)) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        }
        {
            float value;

            if (!(c->distance_squared_to_target >= 16.0f)) {
                value = 0.0f;
            } else if (!(c->distance_squared_to_target >= 49.0f)) {
                value = ((float)sqrt((double)c->distance_squared_to_target) - 4.0f) * 3.3333333f;
            } else {
                value = 10.0f;
            }
            base = value + c->score;
            c->score = base;
        }

        if (c->segment_distance >= 3.4028235e+38f) {
            continue;
        }
        if (query->target_distance <= 0.0f || query->target_distance >= 3.4028235e+38f) {
            continue;
        }
        r = 1.0f - c->segment_distance / (query->target_distance * 0.8f);
        if (r > 0.5f) {
            c->rejected = 1;
            if (query->collect_all == 0) {
                c->valid = 0;
                continue;
            }
        }
        if (!(r >= 0.0f)) {
            r = 0.0f;
        } else if (r > 1.0f) {
            r = 1.0f;
        }
        c->score = (1.0f - r) * 8.0f + base;
    }
}
