// actor_firing_position_compare  (not a Ghidra function; the qsort_dword_array comparator of actor_find_best_firing_position)
// address 0x4127b0, size 109 bytes
// name confidence: 0.5  rewrite confidence: 0.95
// evidence: pushed as the comparator at 0x413cfd; the candidates are qsort_candidate_base (0x006f0c94).
// objdump 0x4127b0..0x41281c: returns whether candidate `element` sorts after `other` (0x3c each):
//   valid ones (+0x30) first, then the not rejected (+0x31), then by descending score (+0x38).
// blam-cc: stack -> element, other

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cseries.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern actor_firing_position_candidate *qsort_candidate_base; // 0x006f0c94

uint8_t actor_firing_position_compare(int32_t element, int32_t other)
{
    actor_firing_position_candidate *a = &qsort_candidate_base[element];
    actor_firing_position_candidate *b = &qsort_candidate_base[other];

    if (a->valid != b->valid) {
        return a->valid == 0;
    }
    if (a->rejected != b->rejected) {
        return a->rejected != 0;
    }
    return a->score < b->score;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
