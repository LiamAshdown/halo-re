// ai_dispatch_queued_order  (Ghidra: ai_dispatch_queued_order; named for this rewrite)
// address 0x42f840, size 126 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x42f840..0x42f8bd: the actor comes on the stack; the count is the word at +0xe (the
//   broadcast header's look kind), the target +0x10, the variant +0xc. One target: 0x4302e0 (EAX -1, EBX actor, EDI
//   target, stack line 8 when the target is the prop's own object else 9, variant). Two: 0x4303a0 (EDI actor, BX
//   variant, ESI target, stack line 9).
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
#include "objects.h"
#include "units.h"

extern data_array *prop_data; // 0x008802c0

extern void actor_issue_order_or_vocalize(datum_index prop_index, datum_index actor_index,
    datum_index vehicle_object_index, int16_t line, int16_t variant); // 0x4302e0, EAX, EBX, EDI, stack
extern void actor_issue_multi_target_vocalization(int16_t line, datum_index actor_index, int16_t variant,
    datum_index vehicle_object_index); // 0x4303a0, stack, EDI, BX, ESI

// blam-cc: ECX -> order, EDX -> prop_index, stack -> actor_index
void ai_dispatch_queued_order(ai_queued_order *order, datum_index prop_index, datum_index actor_index)
{
    uint8_t *o = (uint8_t *)order;
    int16_t count = ((struct ai_queued_order *)o)->target_count;
    datum_index target = ((struct ai_queued_order *)o)->object_a;
    int16_t variant = (int16_t)*(uint16_t *)&((struct ai_queued_order *)o)->single_target;
    int16_t line = 9;

    if (count <= 0) {
        return;
    }
    if (count == 1) {
        prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];

        if (target == p->object_index) {
            line = 8;
        }
        actor_issue_order_or_vocalize(k_datum_index_none, actor_index, target, line, variant);
    } else if (count == 2) {
        actor_issue_multi_target_vocalization(line, actor_index, variant, target);
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
