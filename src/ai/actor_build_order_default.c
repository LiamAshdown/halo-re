// actor_build_order_default  (Ghidra: actor_build_order_default, renamed)
// address 0x401090, size 80 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/ai.h actor.swarm (0x06), actor_order layout (0x401090 zeroes all 0x17
//   dwords of it, which is the evidence for the struct's 0x5c size).
// register convention: actor index in EAX, requested order code in CX (low 16 bits of ECX),
//   pointer to the caller's actor_order in EDX, the order's "parameter" word on the stack.
//   // blam-cc: EAX -> actor_index, ECX -> order_code, EDX -> order, stack -> parameter

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// Zeroes the caller's actor_order (all 0x5c bytes) and fills in a default/idle order: no
// target (0xffff), the caller's order_code unless the actor is a swarm (forced to 0), and
// the caller's parameter word. Always reports success.
int32_t actor_build_order_default(uint32_t actor_index, int16_t order_code, actor_order *order, int16_t parameter)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x17; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->swarm != 0) {
        order_code = 0;
    }

    order->order_code = order_code;
    order->unknown_02 = 0;
    order->unknown_0a = 0;
    order->target_index = -1;
    order->parameter = parameter;
    order->valid = 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x401090):

undefined4 FUN_00401090(undefined2 param_1)

{
  int iVar1;
  uint in_EAX;
  undefined2 in_CX;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  puVar3 = in_EDX;
  for (iVar2 = 0x17; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  if (*(char *)((in_EAX & 0xffff) * 0x724 + iVar1 + 6) != '\0') {
    in_CX = 0;
  }
  *(undefined2 *)in_EDX = in_CX;
  *(undefined2 *)((int)in_EDX + 2) = 0;
  *(undefined1 *)((int)in_EDX + 10) = 0;
  *(undefined2 *)((int)in_EDX + 6) = 0xffff;
  *(undefined2 *)(in_EDX + 2) = param_1;
  *(undefined1 *)(in_EDX + 1) = 1;
  return 1;
}
#endif
