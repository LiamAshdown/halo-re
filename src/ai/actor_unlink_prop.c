// actor_unlink_prop  (Ghidra: actor_unlink_prop, renamed)
// address 0x43ea20, size 95 bytes
// name confidence: 0.45  rewrite confidence: 0.4
// evidence: types/ai.h actor.first_prop(+0x50) and prop.next_in_actor(+0x08). phase-4
// summary "removes a firing-position node from its owning object's linked list of nodes"
// (the list is actually the owning actor's prop list, chained through next_in_actor).
// register convention: EAX -> actor_index, EDI -> prop_to_remove (an `unaff_` register
//   Ghidra's decompile shows).
//   // blam-cc: EAX -> actor_index, EDI -> prop_to_remove

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

// blam-cc: EAX -> actor_index, EDI -> prop_to_remove
void actor_unlink_prop(datum_index actor_index, datum_index prop_to_remove)
{
    actor *self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    datum_index cur = self->first_prop;

    if (cur == prop_to_remove) {
        prop *removed = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
        self->first_prop = removed->next_in_actor;
        return;
    }

    for (;;) {
        prop *p = (prop *)((uint8_t *)prop_data->data + (cur & 0xffff) * sizeof(prop));
        if (p->next_in_actor == prop_to_remove) {
            prop *removed = (prop *)((uint8_t *)prop_data->data + (prop_to_remove & 0xffff) * sizeof(prop));
            p->next_in_actor = removed->next_in_actor;
            return;
        }
        cur = p->next_in_actor;
    }
}

#if 0
// ---- original Ghidra decompilation (FUN_0043ea20 @ 0x43ea20) ----
void FUN_0043ea20(void)

{
  uint *puVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  uint unaff_EDI;

  iVar4 = (in_EAX & 0xffff) * 0x724;
  uVar3 = *(uint *)(iVar4 + 0x50 + *(int *)(DAT_00880360 + 0x34));
  iVar5 = (uVar3 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (uVar3 != unaff_EDI) {
    do {
      puVar1 = (uint *)(iVar5 + 8);
      puVar2 = (undefined4 *)(iVar5 + 8);
      iVar5 = (*puVar1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
    } while (*puVar1 != unaff_EDI);
    *puVar2 = *(undefined4 *)(iVar5 + 8);
    return;
  }
  *(undefined4 *)(iVar4 + 0x50 + *(int *)(DAT_00880360 + 0x34)) = *(undefined4 *)(iVar5 + 8);
  return;
}
#endif
