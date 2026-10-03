// ai_target_distance_qsort_compare  (Ghidra: ai_target_distance_qsort_compare, already named)
// address 0x41d7a0, size 57 bytes, cdecl
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: qsort comparator passed to qsort inside actor_target_scan_potential_targets
//   (0x41d7e0); orders two candidate-target records ascending by a float at +8. The records
//   are a small local sort structure that actor_target_scan_potential_targets builds (each
//   holding, among other things, a copy of the matching prop's distance), not a prop pointer
//   directly -- types/ai.h's prop.distance (0x11c) is the field this function is the evidence
//   for, one level removed through that copy.
//
// TYPES-GAP: no header defines the local {..., distance} sort record built by
// actor_target_scan_potential_targets; only the +8 float this function reads is named here.

#include "crt.h"
#include "tags.h"
#include "memory.h"

// blam-cc: cdecl(record_a, record_b)
// qsort comparator that orders two candidate-target sort records ascending by the float
// distance field at +8.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int ai_target_distance_qsort_compare(void *record_a, void *record_b)
{
    if (*(float *)((uint8_t *)record_a + 8) < *(float *)((uint8_t *)record_b + 8)) {
        return -1;
    }
    if (*(float *)((uint8_t *)record_b + 8) < *(float *)((uint8_t *)record_a + 8)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x41d7a0):

int __cdecl ai_target_distance_qsort_compare(void *record_a,void *record_b)

{
  if (*(float *)((int)record_a + 8) < *(float *)((int)record_b + 8)) {
    return -1;
  }
  if (*(float *)((int)record_b + 8) < *(float *)((int)record_a + 8)) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
