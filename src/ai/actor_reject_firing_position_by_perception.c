// actor_reject_firing_position_by_perception  (Ghidra: FUN_004124c0, unnamed)
// address 0x4124c0, size 152 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x4124c0..0x412557 (row 3 of the rejection table 0x006555f8, kinds 0x0006). Only with a
//   target (query +0x5fc): without a candidate the baseline penalty (+0x660) gains 12 and 1 is returned; otherwise
//   the candidate's perception result (+0x06) picks the desirability through the jump table 0x412558:
//   0 -> rejected (rejected = 1, valid cleared unless query.collect_all), 1 -> 6.0 when query +0x08 is set, else
//   rejected, 2 -> 12.0, 3 -> 4.0, 4 -> 10.0, others -> 0. The value is added to the score. Returns the candidate's
//   valid byte (1 without a candidate).
// blam-cc: stack -> actor_index, query, candidate
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
static void reject(actor_firing_position_query *query, actor_firing_position_candidate *candidate)
{
    candidate->rejected = 1;
    if (query->collect_all == 0) {
        candidate->valid = 0;
    }
}

uint8_t actor_reject_firing_position_by_perception(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *candidate)
{
    (void)actor_index;
    if (query->have_target != 0) {
        float value = 0.0f;

        if (candidate == (actor_firing_position_candidate *)0) {
            query->baseline_penalty = query->baseline_penalty + 12.0f;
            return 1;
        }
        switch (candidate->request_result) {
        case 0:
            reject(query, candidate);
            break;
        case 1:
            if (query->unknown_08[0] != 0) {
                value = 6.0f;
            } else {
                reject(query, candidate);
            }
            break;
        case 2:
            value = 12.0f;
            break;
        case 3:
            value = 4.0f;
            break;
        case 4:
            value = 10.0f;
            break;
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
