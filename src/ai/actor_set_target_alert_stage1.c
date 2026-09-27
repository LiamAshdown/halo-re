// actor_set_target_alert_stage1  (Ghidra: actor_set_target_alert_stage1; named from out/phase2/results/ai_02.json)
// address 0x41fb00, size 85 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase2/results/ai_02.json -- sets the prop (target-data) record's byte at
//   offset 0xb9 for the given prop index whenever it is valid, then, only if that prop is the
//   watching actor's current target (actor+0x270), refreshes the target combat status /
//   awareness pair; a near-identical sibling exists at +0xba (actor_set_target_alert_stage2)
//   and +0xbb (actor_set_target_alert_stage3, actor_set_target_alert_stage3.c). Matches
//   prop.noticed_a (0xb9) and actor.target_unit_index (0x270) in types/ai.h.
// register convention: ECX -> target_prop_index, ESI -> actor_index (unaff_ESI).
//   // blam-cc: ECX -> target_prop_index, ESI -> actor_index
//
// UNSURE: see actor_target_update_active_flag.c for the same actor+0x270-vs-prop-index
// discrepancy this function's equality test relies on.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// UNSURE signature: real functions take an actor_index (EAX, plus a CX value for the first);
// Ghidra recovers no arguments at either call site here.
extern void actor_update_target_combat_status(datum_index actor_index); // 0x4200d0, UNSURE signature
extern void actor_update_awareness_level(datum_index actor_index);       // 0x420290, UNSURE signature

// blam-cc: ECX -> target_prop_index, ESI -> actor_index
// Marks a per-prop alert/notice flag (noticed_a) and, if that prop is the caller actor's
// current target, refreshes the actor's target combat status.
void actor_set_target_alert_stage1(datum_index target_prop_index, datum_index actor_index)
{
    prop *target;
    actor *self;

    if (target_prop_index != k_datum_index_none) {
        target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));
        target->noticed_a = 1;

        self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
        if (target_prop_index == self->target_unit_index) {
            actor_update_target_combat_status(actor_index);
            actor_update_awareness_level(actor_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x41fb00):

void FUN_0041fb00(void)

{
  int iVar1;
  uint in_ECX;
  uint unaff_ESI;

  if (in_ECX != 0xffffffff) {
    iVar1 = *(int *)(DAT_00880360 + 0x34);
    *(undefined1 *)((in_ECX & 0xffff) * 0x138 + 0xb9 + *(int *)(DAT_008802c0 + 0x34)) = 1;
    if (in_ECX == *(uint *)((unaff_ESI & 0xffff) * 0x724 + iVar1 + 0x270)) {
      actor_update_target_combat_status();
      actor_update_awareness_level();
      return;
    }
  }
  return;
}
#endif
