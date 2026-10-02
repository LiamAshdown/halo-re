// ai_reference_set_unknown_1cb  (Ghidra: ai_reference_set_unknown_1cb; named for this rewrite)
// address 0x434d40, size 64 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: sets actor.unknown_1cb (types/ai.h: "0x434d40 sets it on every member of a
// squad") to (flag == 0) for every actor a packed ai reference names.
//   // blam-cc: EAX -> packed_reference, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void ai_reference_actor_iterator_new(uint32_t packed_reference, ai_reference_actor_iterator *out_iterator); // 0x432650, this batch
extern actor *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, this batch

void ai_reference_set_unknown_1cb(uint32_t packed_reference, char flag)
{
    ai_reference_actor_iterator iterator;
    actor *a;
    uint8_t value = (flag == 0);

    ai_reference_actor_iterator_new(packed_reference, &iterator);
    a = ai_reference_actor_iterator_next(&iterator);
    while (a != 0) {
        a->charge_disallowed = value;
        a = ai_reference_actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x434d40):

void FUN_00434d40(char param_1)

{
  int iVar1;

  FUN_00432650();
  iVar1 = FUN_004326d0();
  if (iVar1 != 0) {
    do {
      *(bool *)(iVar1 + 0x1cb) = param_1 == '\0';
      iVar1 = FUN_004326d0();
    } while (iVar1 != 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
