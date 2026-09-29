// actor_build_order_minimal_stop  (Ghidra: actor_build_order_minimal_stop, renamed)
// address 0x4078f0, size 68 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.swarm (0x06)/unknown_98; phase-4 summary "a minimal 'stop/idle'
//   order, only valid while the actor is inactive" (this session reads the gate as
//   actor.swarm rather than an inactive flag, per the actual byte tested).
// register convention: actor index in EAX, order pointer in ESI.
//   // blam-cc: EAX -> actor_index, ESI -> order

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

int32_t actor_build_order_minimal_stop(uint32_t actor_index, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;

    for (i = 0xb; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->swarm != 0) {
        *(int16_t *)(order + 2) = 2;
        a->unknown_98 = 1;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4078f0):

undefined4 FUN_004078f0(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *unaff_ESI;
  undefined4 *puVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = unaff_ESI;
  for (iVar2 = 0xb; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  if (*(char *)(iVar1 + 6) != '\0') {
    *(undefined2 *)(unaff_ESI + 2) = 2;
    *(undefined1 *)(iVar1 + 0x98) = 1;
    return 1;
  }
  return 0;
}
#endif
