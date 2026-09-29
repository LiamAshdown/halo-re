// actor_build_order_search_wait  (Ghidra: actor_build_order_search_wait, renamed)
// address 0x4045a0, size 260 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4045a0..0x4046a3 and the jump table at 0x4046a4)
// evidence: types/ai.h actor.unknown_15e/order_committed/swarm/unknown_1e8/unknown_1e4;
//   actor_order's opaque tail (order kind at +0x24, seen in the other order builders in
//   this session); phase-4 summary "a search-and-wait order at a scenario search position,
//   scaling the wait duration by target category".
// register convention: actor index in EAX, order pointer in ESI (unaff_ESI).
//   // blam-cc: EAX -> actor_index, ESI -> order
// UNSURE: actor.unknown_1e8 is treated here as a prop_data index (stride 0x138), matching
//   the same target_unit_index-vs-prop-index discrepancy flagged in
//   actor_update_melee_combat_action.c; prop+0xf0/0xf4/0xf8 straddle two of prop's declared
//   fields (unknown_ec.y/.z and unknown_f8) rather than one clean point, which is either a
//   genuine 3-float read spanning that boundary or a sign that the boundary in types/ai.h is
//   slightly off; kept as raw offsets rather than guessing a fix.
//   UNSURE: actor_target_get_relationship_object's return value is discarded by the original
//   (called for a side effect this function does not otherwise use).
//   UNSURE: the returned value's upper 24 bits are decompiler noise from a reused scratch
//   variable in the no-match switch default case; modelled as a plain 0/1 result.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void actor_target_get_relationship_object(datum_index target_prop_index); // 0x41f3a0, this module,
                                                 // blam-cc: EAX -> target_prop_index

// Builds a "search and wait" order (code 0x78) at the actor's pending search position
// (actor.unknown_1e8), unless the actor's turn-bound state (unknown_15e) is 4, in which case
// it returns failure without committing a target. The wait duration category comes from
// actor.unknown_1e4 (6, 7/8, or 9 map to 2.0/1.0/1.5 seconds; anything else aborts).
int32_t actor_build_order_search_wait(uint32_t actor_index, actor_order *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = (uint32_t *)order;
    int32_t i;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    order->order_code = 0x78;
    *(int16_t *)((uint8_t *)order + 0x24) = 1;
    *((uint8_t *)order + 5) = 1;
    *(int32_t *)((uint8_t *)order + 0x3c) = -1;

    if (a->vehicle_driving_type == 4) {
        return 0;
    }

    if (a->order_committed == 0 && a->swarm == 0 && a->post_combat_prop_index != (datum_index)k_datum_index_none) {
        prop *p = &((prop *)prop_data->data)[a->post_combat_prop_index & 0xffff];

        *(int32_t *)((uint8_t *)order + 0x3c) = a->post_combat_prop_index;
        order->unknown_02 = 0x78;
        *((uint8_t *)order + 0x40) = 1;

        switch (a->post_combat_action - 6) {
        case 0:
            *(float *)((uint8_t *)order + 0x38) = 2.0f;
            break;
        case 1:
        case 2:
            *(float *)((uint8_t *)order + 0x38) = 1.0f;
            break;
        case 3:
            *(float *)((uint8_t *)order + 0x38) = 1.5f;
            break;
        default:
            return 1;
        }

        // 0x40466c loads EAX from actor+0x1e8 just before this call.
        actor_target_get_relationship_object(a->post_combat_prop_index);
        *(int16_t *)((uint8_t *)order + 0x24) = 2;
        *(float *)((uint8_t *)order + 0x28) = ((struct prop *)p)->pathfinding_point.x;
        *(float *)((uint8_t *)order + 0x2c) = ((struct prop *)p)->pathfinding_point.y;
        *(float *)((uint8_t *)order + 0x30) = ((struct prop *)p)->pathfinding_point.z;
        *(int32_t *)((uint8_t *)order + 0x34) = ((struct prop *)p)->pathfinding_surface_index;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4045a0):

undefined4 FUN_004045a0(void)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  undefined4 *unaff_ESI;
  undefined4 *puVar4;
  int iVar5;

  iVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  puVar4 = unaff_ESI;
  for (iVar3 = 0x11; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  iVar3 = -1;
  *(undefined2 *)unaff_ESI = 0x78;
  *(undefined2 *)(unaff_ESI + 9) = 1;
  *(undefined1 *)((int)unaff_ESI + 5) = 1;
  unaff_ESI[0xf] = 0xffffffff;
  if (*(short *)(iVar2 + 0x15e) == 4) {
    return 0xffffff00;
  }
  if (((*(char *)(iVar2 + 0x160) == '\0') && (*(char *)(iVar2 + 6) == '\0')) &&
     (uVar1 = *(uint *)(iVar2 + 0x1e8), uVar1 != 0xffffffff)) {
    iVar5 = *(int *)(DAT_008802c0 + 0x34);
    unaff_ESI[0xf] = uVar1;
    *(undefined2 *)((int)unaff_ESI + 2) = 0x78;
    *(undefined1 *)(unaff_ESI + 0x10) = 1;
    iVar5 = (uVar1 & 0xffff) * 0x138 + iVar5;
    iVar3 = *(short *)(iVar2 + 0x1e4) + -6;
    switch(iVar3) {
    case 0:
      unaff_ESI[0xe] = 0x40000000;
      break;
    case 1:
    case 2:
      unaff_ESI[0xe] = 0x3f800000;
      break;
    case 3:
      unaff_ESI[0xe] = 0x3fc00000;
      break;
    default:
      goto switchD_0040464c_default;
    }
    actor_target_get_relationship_object();
    *(undefined2 *)(unaff_ESI + 9) = 2;
    unaff_ESI[10] = *(undefined4 *)(iVar5 + 0xf0);
    unaff_ESI[0xb] = *(undefined4 *)(iVar5 + 0xf4);
    unaff_ESI[0xc] = *(undefined4 *)(iVar5 + 0xf8);
    iVar3 = *(int *)(iVar5 + 0xec);
    unaff_ESI[0xd] = iVar3;
  }
switchD_0040464c_default:
  return CONCAT31((int3)((uint)iVar3 >> 8),1);
}
#endif
