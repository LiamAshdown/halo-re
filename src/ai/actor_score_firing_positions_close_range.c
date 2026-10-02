// actor_score_firing_positions_close_range  (Ghidra: FUN_00411b60, unnamed)
// address 0x411b60, size 129 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x411b60..0x411be0 (row 3 of the scoring table 0x006555c0, kinds 0x0010). For every
//   valid candidate: +8 inside half the maximum distance (query +0x18), falling linearly to 0 at the maximum
//   ((max - d) / (max / 2) * 8), nothing beyond it.
// blam-cc: stack -> actor_index, query, count, candidates
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void actor_score_firing_positions_close_range(datum_index actor_index, actor_firing_position_query *query,
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

        if (c->valid == 0) {
            continue;
        }
        if (query->maximum_distance * 0.5f > c->distance_from_actor) {
            value = 8.0f;
        } else if (!(c->distance_from_actor >= query->maximum_distance)) {
            value = (1.0f / (query->maximum_distance * 0.5f)) * (query->maximum_distance - c->distance_from_actor) * 8.0f;
        }
        c->score = value + c->score;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
