// actor_reject_firing_position_by_target_approach  (Ghidra: FUN_00412570, unnamed)
// address 0x412570, size 171 bytes
// name confidence: 0.45  rewrite confidence: 0.85
// REWRITTEN from objdump 0x412570..0x41261b (row 2 of the rejection table 0x006555f8, kinds 0x0008). Only with a
//   target (query +0x5fc): without a candidate the baseline penalty gains 20 and 1 is returned; otherwise the
//   perception result (+0x06) 0 adds 20.0, 1 adds 10.0, and any other result keeps the candidate only when it is
//   within (target distance - 7.5) of the target (distance_squared_to_target <= d * d with d >= 0); a candidate
//   failing that is rejected (rejected = 1, valid cleared unless query.collect_all). Returns the valid byte.
// blam-cc: stack -> actor_index, query, candidate
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

uint8_t actor_reject_firing_position_by_target_approach(datum_index actor_index,
    actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    (void)actor_index;
    if (query->have_target != 0) {
        float value = 0.0f;

        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 20.0f;
            return 1;
        }
        if (candidate->request_result == 0) {
            value = 20.0f;
        } else if (candidate->request_result == 1) {
            value = 10.0f;
        } else {
            float d = query->target_distance - 7.5f;

            // 0x4125c0: fcom 0 / test ah,5 / jnp -> d < 0 rejects; 0x4125d4: d*d vs the squared distance,
            // test ah,0x41 / jne -> kept when distance_squared_to_target <= d*d
            if (!(d >= 0.0f) || !(candidate->distance_squared_to_target <= d * d)) {
                candidate->rejected = 1;
                if (query->collect_all == 0) {
                    candidate->valid = 0;
                }
            }
        }
        candidate->score = value + candidate->score;
    }
    if (candidate == (actor_firing_position_candidate *)0) {
        return 1;
    }
    return candidate->valid;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
