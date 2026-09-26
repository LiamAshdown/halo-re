// actor_build_order_return_to_anchor  (Ghidra: actor_build_order_return_to_anchor, renamed)
// address 0x4044b0, size 82 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.position (0x174) and actor_order (the builder zeroes a prefix
//   of the same 0x5c-byte record 0x401090 fully zeroes; every offset this function touches,
//   0x14..0x3f, falls inside actor_order.unknown_0b, the 81-byte opaque tail no single
//   builder fully explains). phase-4 summary: "sends the actor back to its anchor/home
//   position".
// register convention: actor index in EAX, pointer to the caller's actor_order in EDX.
//   // blam-cc: EAX -> actor_index, EDX -> order
// UNSURE: the individual tail fields (kind selector at +0x24, "valid" byte at +0x14, the -1
// sentinel at +0x3c) are named descriptively rather than authoritatively; actor_order's tail
// layout is only cross-checked against a few of this session's other order builders, not
// against its consumer (actor_process_order_request, 0x409ea0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// Zeroes the leading 0x44 bytes of the caller's actor_order and fills it with a "go to a
// fixed point" order targeting the actor's own current position (i.e. its anchor), with the
// point-order kind selector set and no explicit end-of-list sentinel cleared.
// FIXED: every ret of the original is preceded by mov eax,1 and callers test it
int32_t actor_build_order_return_to_anchor(uint32_t actor_index, actor_order *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    *(int16_t *)((uint8_t *)order + 0x24) = 1;       // order kind: explicit point
    *((uint8_t *)order + 0x14) = 1;                  // valid-ish tail flag
    // The original copies actor+0x174..0x17c, which types/ai.h now calls actor.facing
        // (a unit forward vector, not a point). The order slot is a 12-byte field the
        // rest of the module treats as a point, so the copy is kept byte for byte.
        *(real_vector3d *)((uint8_t *)order + 0x18) = a->facing;
    *(int32_t *)((uint8_t *)order + 0x3c) = -1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4044b0):

void FUN_004044b0(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  undefined4 *in_EDX;
  undefined4 *puVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar3 = in_EDX;
  for (iVar2 = 0x11; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = 0;
    puVar3 = puVar3 + 1;
  }
  *(undefined2 *)(in_EDX + 9) = 1;
  *(undefined1 *)(in_EDX + 5) = 1;
  in_EDX[6] = *(undefined4 *)(iVar1 + 0x174);
  in_EDX[7] = *(undefined4 *)(iVar1 + 0x178);
  in_EDX[8] = *(undefined4 *)(iVar1 + 0x17c);
  in_EDX[0xf] = 0xffffffff;
  return;
}
#endif
