// actor_build_order_flee  (Ghidra: actor_build_order_flee, renamed)
// address 0x4077d0, size 77 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h actor.order_committed (0x160)/unknown_98; phase-4 summary "builds a
//   flee-style order for the actor and marks it as having committed to a new order".
// register convention: actor index in EAX, order pointer in EDX, a caller byte in the stack
//   parameter.
//   // blam-cc: EAX -> actor_index, EDX -> order, stack -> byte_a

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

int32_t actor_build_order_flee(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xb; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0) {
        *((uint8_t *)order + 5) = byte_a;
        *(int16_t *)(order + 2) = 0;
        a->search_firing_positions = 1;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4077d0):

undefined4 FUN_004077d0(undefined1 param_1)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = in_EDX;
  for (iVar2 = 0xb; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  if (*(char *)(iVar1 + 0x160) == '\0') {
    *(undefined1 *)((int)in_EDX + 5) = param_1;
    *(undefined2 *)(in_EDX + 2) = 0;
    *(undefined1 *)(iVar1 + 0x98) = 1;
    return 1;
  }
  return 0;
}
#endif
