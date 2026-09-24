// ai_dispatch_queued_order  (Ghidra: ai_dispatch_queued_order; named for this rewrite)
// address 0x42f840, size 126 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: phase-4 summary ("dispatches a queued AI order record to either the single-target
// or multi-target order-issuing routine based on its target count").
// register convention: ECX -> order, EDX -> prop_index (both unresolved registers in
// Ghidra's own decompile).
// blam-cc: ECX -> order, EDX -> prop_index
//
// UNSURE: actor_issue_multi_target_vocalization (this batch) is called here with only one visible argument, but its
// own phase-4 summary implies it also needs the order record; passed order through as well.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern void actor_issue_order_or_vocalize(uint32_t reason, int16_t single_target); // 0x4302e0, this batch
extern void actor_issue_multi_target_vocalization(uint32_t reason, ai_queued_order *order); // 0x4303a0, this batch; UNSURE second arg

// blam-cc: ECX -> order, EDX -> prop_index
// If order has any targets, issues it: a single-target order goes to actor_issue_order_or_vocalize (reason 8
// if its stored object handle matches prop_index's own tracked object, else 9), a
// multi-target order (count == 2) goes to actor_issue_multi_target_vocalization with reason 9.
void ai_dispatch_queued_order(ai_queued_order *order, datum_index prop_index)
{
    uint32_t reason;
    prop *p;

    if (0 < order->target_count) {
        reason = 9;
        if (order->target_count == 1) {
            p = &((prop *)prop_data->data)[prop_index & 0xffff];
            if (order->object_a == p->object_index) {
                reason = 8;
            }
        }
        if (order->target_count == 1) {
            actor_issue_order_or_vocalize(reason, order->single_target);
        } else if (order->target_count == 2) {
            actor_issue_multi_target_vocalization(reason, order);
        }
    }
}

#if 0
Original Ghidra decompilation (0x42f840):

void FUN_0042f840(void)

{
  short sVar1;
  int in_ECX;
  uint in_EDX;
  undefined4 uVar2;

  sVar1 = *(short *)(in_ECX + 0xe);
  if (0 < sVar1) {
    uVar2 = 9;
    if ((sVar1 == 1) &&
       (*(int *)(in_ECX + 0x10) ==
        *(int *)((in_EDX & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34)))) {
      uVar2 = 8;
    }
    if (sVar1 == 1) {
      FUN_004302e0(uVar2,*(undefined2 *)(in_ECX + 0xc));
    }
    else if (sVar1 == 2) {
      FUN_004303a0(uVar2);
      return;
    }
  }
  return;
}
#endif
