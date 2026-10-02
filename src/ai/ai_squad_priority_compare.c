// ai_squad_priority_compare  (Ghidra: ai_squad_priority_compare, already named)
// address 0x42ac90, size 54 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: sole caller ai_build_priority_target_list @0x42acd0 (this rewrite), whose
// 12-byte records are {uint8_t tiebreak; uint8_t pad[3]; uint32_t handle; int32_t priority;}
// -- confirmed by matching this comparator's two offsets (0 and 8) against exactly where
// that function writes each field.
// register convention: plain __cdecl, matching qsort's comparator signature.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// qsort comparator that orders candidate entries by priority (descending) with a tie-break
// on the leading tiebreak byte (ascending).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int __cdecl ai_squad_priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b)
{
    if (record_b->priority < record_a->priority) {
        return 1;
    }
    if (record_a->priority < record_b->priority) {
        return -1;
    }
    if (record_b->tiebreak < record_a->tiebreak) {
        return -1;
    }
    return record_a->tiebreak < record_b->tiebreak;
}

#if 0
Original Ghidra decompilation (0x42ac90):

int __cdecl ai_squad_priority_compare(uchar *record_a,uchar *record_b)

{
  if (*(int *)(record_b + 8) < *(int *)(record_a + 8)) {
    return 1;
  }
  if (*(int *)(record_a + 8) < *(int *)(record_b + 8)) {
    return -1;
  }
  if (*record_b < *record_a) {
    return -1;
  }
  return (uint)(*record_a < *record_b);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
