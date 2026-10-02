// ai_reference_max_activity_stage  (Ghidra: ai_reference_max_activity_stage; named for this rewrite)
// address 0x435700, size 69 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: returns the highest ai_actor_get_activity_stage (0x435680, this batch) among
// every actor a packed ai reference names. Matches the phase-4 summary.
//   // blam-cc: EAX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch
extern int32_t ai_actor_get_activity_stage(datum_index actor_index); // 0x435680, this batch

int16_t ai_reference_max_activity_stage(uint32_t packed_reference)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    int16_t best = 0;

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        int16_t stage = (int16_t)ai_actor_get_activity_stage(iterator.actor_index);
        if (best <= stage) {
            best = stage;
        }
        a = ai_reference_actor_iterator_next(&iterator);
    }
    return best;
}

#if 0
Original Ghidra decompilation (0x435700):

short FUN_00435700(void)

{
  short sVar1;
  int iVar2;
  short sVar3;

  sVar3 = 0;
  FUN_00432650();
  iVar2 = FUN_004326d0();
  while (iVar2 != 0) {
    sVar1 = FUN_00435680();
    if (sVar3 <= sVar1) {
      sVar3 = sVar1;
    }
    iVar2 = FUN_004326d0();
  }
  return sVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
