// actor_target_reset_combat_flags  (Ghidra: actor_target_reset_combat_flags, already named)
// address 0x41baf0, size 55 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase2/results/ai_02.json -- clears target-data flags at +0xb9/+0xba/+0xbb and
//   sets +0x64=1, then calls actor_queue_sighted_target_dialogue; identical tail sequence appears inside
//   actor_target_data_release (0x41b980). +0xb9/+0xba/+0xbb are prop.noticed_a/b/c and +0x64
//   is prop.combat_dirty in types/ai.h.
// register convention: the prop record pointer is only visible as in_ECX (an already-scaled
//   prop index, i.e. the low 16 bits of a datum_index). Mapped to the first parameter (ECX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern void actor_queue_sighted_target_dialogue(void); // 0x421c20, not in this rewrite range, UNSURE signature

// blam-cc: ECX -> target_prop_index (low 16 bits used as the prop slot)
// Resets a prop's (a target-data record's) combat-status flags -- clears the three per-unit
// notice flags and marks it dirty so the next update recomputes its combat status -- e.g.
// after losing track of or re-acquiring a target.
void actor_target_reset_combat_flags(datum_index target_prop_index)
{
    prop *target;

    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
    target->noticed_b = 0;
    target->noticed_a = 0;
    target->noticed_c = 0;
    target->combat_dirty = 1;
    // UNSURE: Ghidra shows actor_queue_sighted_target_dialogue() called with no visible arguments in this function.
    // The sibling call site in actor_target_data_release.c (0x41b980) passes this same tail
    // sequence's actor_queue_sighted_target_dialogue call an explicit actor_index argument, suggesting the real
    // signature needs an actor_index this function never received (it only has the prop
    // index, via ECX) -- called with no arguments here rather than guessing a wrong one.
    actor_queue_sighted_target_dialogue();
}

#if 0
Original Ghidra decompilation (0x41baf0):

void actor_target_reset_combat_flags(void)

{
  int iVar1;
  uint in_ECX;

  iVar1 = (in_ECX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  *(undefined1 *)(iVar1 + 0xba) = 0;
  *(undefined1 *)(iVar1 + 0xb9) = 0;
  *(undefined1 *)(iVar1 + 0xbb) = 0;
  *(undefined1 *)(iVar1 + 100) = 1;
  FUN_00421c20();
  return;
}
#endif
