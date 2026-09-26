// actor_build_order_guard  (Ghidra: actor_build_order_guard, renamed)
// address 0x404510, size 143 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.order_committed (0x160), actor.swarm (0x06), actor.position
//   (0x174); actor_order's opaque tail (unknown_0b), the same 0x44-byte prefix and "kind"
//   field at +0x24 seen in actor_build_order_return_to_anchor. phase-4 summary: "a
//   stationary guard order, either at the actor's current position or at its designated
//   anchor point".
// register convention: actor index in EAX, order pointer in EDX, a guard-code selector in BX
//   (unaff_BX: 0 selects the anchor, nonzero selects the actor's current position).
//   // blam-cc: EAX -> actor_index, EDX -> order, EBX -> guard_at_current_position
// UNSURE: the individual tail fields are named descriptively, not authoritatively; see
// actor_build_order_return_to_anchor.c for the shared caveat.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

// Builds a guard order. If the actor has already committed to an order this tick or is a
// swarm, builds a minimal fallback guard order instead (kind 1, no point). Otherwise, if
// guard_at_current_position is zero, builds a "guard the anchor" order (kind 0); if nonzero,
// builds an explicit-point guard order at the actor's current position (kind 1).
// FIXED: every ret of the original is preceded by mov eax,1 and callers test it
int32_t actor_build_order_guard(uint32_t actor_index, actor_order *order, int16_t guard_at_current_position)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    int16_t *body = (int16_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        body[0] = 0;
        body[1] = 0;
        body += 2;
    }

    if (a->order_committed == 0 && a->swarm == 0) {
        order->order_code = guard_at_current_position;
        if (guard_at_current_position == 0) {
            *((uint8_t *)order + 0xe) = 1;
            *(int16_t *)((uint8_t *)order + 0x24) = 0;
            *(int16_t *)((uint8_t *)order + 0x3c) = -1;
            *(int16_t *)((uint8_t *)order + 0x3e) = -1;
            return 1;
        }
        *(int16_t *)((uint8_t *)order + 0x24) = 1;
        *((uint8_t *)order + 0x14) = 1;
        // The original copies actor+0x174..0x17c, which types/ai.h now calls actor.facing
        // (a unit forward vector, not a point). The order slot is a 12-byte field the
        // rest of the module treats as a point, so the copy is kept byte for byte.
        *(real_vector3d *)((uint8_t *)order + 0x18) = a->facing;
        *(int16_t *)((uint8_t *)order + 0x3c) = -1;
        *(int16_t *)((uint8_t *)order + 0x3e) = -1;
        return 1;
    }
    *(int16_t *)((uint8_t *)order + 0x24) = 1;
    *(int16_t *)((uint8_t *)order + 0x3c) = -1;
    *(int16_t *)((uint8_t *)order + 0x3e) = -1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x404510):

void FUN_00404510(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  short *in_EDX;
  short unaff_BX;
  short *psVar3;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  psVar3 = in_EDX;
  for (iVar2 = 0x11; iVar2 != 0; iVar2 = iVar2 + -1) {
    psVar3[0] = 0;
    psVar3[1] = 0;
    psVar3 = psVar3 + 2;
  }
  if ((*(char *)(iVar1 + 0x160) == '\0') && (*(char *)(iVar1 + 6) == '\0')) {
    *in_EDX = unaff_BX;
    if (unaff_BX == 0) {
      *(undefined1 *)(in_EDX + 7) = 1;
      in_EDX[0x12] = 0;
      in_EDX[0x1e] = -1;
      in_EDX[0x1f] = -1;
      return;
    }
    in_EDX[0x12] = 1;
    *(undefined1 *)(in_EDX + 10) = 1;
    *(undefined4 *)(in_EDX + 0xc) = *(undefined4 *)(iVar1 + 0x174);
    *(undefined4 *)(in_EDX + 0xe) = *(undefined4 *)(iVar1 + 0x178);
    *(undefined4 *)(in_EDX + 0x10) = *(undefined4 *)(iVar1 + 0x17c);
    in_EDX[0x1e] = -1;
    in_EDX[0x1f] = -1;
    return;
  }
  in_EDX[0x12] = 1;
  in_EDX[0x1e] = -1;
  in_EDX[0x1f] = -1;
  return;
}
#endif
