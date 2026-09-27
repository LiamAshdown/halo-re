// actor_build_order_wait_byte  (Ghidra: actor_build_order_wait_byte, renamed)
// address 0x4080c0, size 75 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump.)
// evidence: types/ai.h actor.order_committed (0x160)/swarm (0x06); phase-4 summary "builds a
//   simple wait order carrying a single caller-supplied byte parameter".
// register convention: actor index in EAX, order pointer in ESI, a caller byte in the
//   recognized stack parameter.
//   // blam-cc: EAX -> actor_index, ESI -> order, stack -> byte_a

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

int32_t actor_build_order_wait_byte(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xd; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed == 0 && a->swarm == 0) {
        *(int16_t *)(order + 2) = 0;
        *((uint8_t *)order + 3) = byte_a;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4080c0):

undefined4 FUN_004080c0(undefined1 param_1)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *unaff_ESI;
  undefined4 *puVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = unaff_ESI;
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  if ((*(char *)(iVar1 + 0x160) == '\0') && (*(char *)(iVar1 + 6) == '\0')) {
    *(undefined2 *)(unaff_ESI + 2) = 0;
    *(undefined1 *)((int)unaff_ESI + 3) = param_1;
    return 1;
  }
  return 0;
}
#endif
