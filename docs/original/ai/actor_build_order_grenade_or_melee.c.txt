// actor_build_order_grenade_or_melee  (Ghidra: actor_build_order_grenade_or_melee, renamed)
// address 0x403630, size 264 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED against objdump 0x403630..0x403737 (bail on actor +0x160; clear 0x30 bytes; base 0xb4 when DL; +8 -1, +0xc code, +4/+5 flags, +0x1c target -> actor_consider_target_candidate(EBX target, stack actor); codes 9..12 roll < 0.4 -> order +2 = 0x2d; else, unless actor +6, melee reachability (EBX order) succeeds when +8 is set); FIXED the failure path clearing byte +0x0e, not +0x07)
// evidence: types/ai.h actor.order_committed (0x160)/swarm (0x06); types/ai.h
//   actor_order_code (grenade-throw range 9..12, matches the random-bias gate here);
//   phase-4 summary "biasing toward a grenade throw for certain order codes and otherwise
//   checking for a valid melee attack".
// register convention: actor index in EAX-turned-stack-param (param_1), candidate order
//   code in param_2, two caller bytes in param_3/param_4, output order pointer in param_5,
//   a resolved target handle in EAX (in_EAX) and a "use alternate base code" flag in DL
//   (in_DL).
//   // blam-cc: EAX -> resolved_target, DL -> use_alt_base, stack -> actor_index/
//   order_code/byte_a/byte_b/order
// TYPES-GAP/UNSURE: this order record's tail does not line up with actor_order at the
// offsets used here (byte 8 behaves like a "resolved" sentinel, not actor_order.parameter);
// kept as raw ushort-indexed offsets on the caller's buffer rather than forced into
// actor_order, consistent with the other order-family builders' varying tails in this
// session.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360
extern uint32_t random_seed_global; // 0x00719cd0

extern uint16_t actor_consider_target_candidate(datum_index actor_index,
                                                datum_index candidate_prop_index); // 0x4208a0, this module;
// stack -> actor_index, EBX -> candidate_prop_index. The EBX value at 0x4036af is the same
// resolved target prop handle the code has just written to the order record at +0x1c.
extern void actor_check_melee_target_reachable(uint32_t actor_index, int16_t *order); // 0x403f00, stack, EBX order

int32_t actor_build_order_grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base, uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    int32_t i;
    uint16_t *body = order;

    if (a->order_committed != 0) {
        return 0;
    }

    for (i = 0xc; i != 0; i--) {
        body[0] = 0;
        body[1] = 0;
        body += 2;
    }

    order[0] = -(uint16_t)(use_alt_base != 0) & 0xb4;
    order[4] = 0xffff;
    order[6] = order_code;
    *(uint8_t *)(order + 2) = byte_a;
    *((uint8_t *)order + 5) = byte_b;
    *(uint32_t *)(order + 0xe) = resolved_target;
    if (resolved_target != 0xffffffff) {
        actor_consider_target_candidate(actor_index, (datum_index)resolved_target);
    }

    if ((int16_t)order_code > 8 && (int16_t)order_code < 0xd) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        if ((float)(random_seed_global >> 0x10) * 1.5259022e-05f < 0.4f) {
            order[1] = 0x2d;
            return 1;
        }
    }
    if (a->swarm == 0) {
        actor_check_melee_target_reachable(actor_index, (int16_t *)order); // 0x403717: EBX = the order
        if (order[4] != 0xffff) {
            return 1;
        }
        *((uint8_t *)order + 0xe) = 0; // 0x403728: byte +0x0e (the draft cleared byte +0x07)
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x403630):

undefined4
FUN_00403630(uint param_1,ushort param_2,undefined1 param_3,undefined1 param_4,ushort *param_5)

{
  int iVar1;
  int in_EAX;
  int iVar2;
  char in_DL;
  int iVar3;
  ushort *puVar4;

  iVar3 = (param_1 & 0xffff) * 0x724;
  iVar1 = *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar3 + 0x160 + iVar1) == '\0') {
    puVar4 = param_5;
    for (iVar2 = 0xc; iVar2 != 0; iVar2 = iVar2 + -1) {
      puVar4[0] = 0;
      puVar4[1] = 0;
      puVar4 = puVar4 + 2;
    }
    *param_5 = -(ushort)(in_DL != '\0') & 0xb4;
    param_5[4] = 0xffff;
    param_5[6] = param_2;
    *(undefined1 *)(param_5 + 2) = param_3;
    *(undefined1 *)((int)param_5 + 5) = param_4;
    *(int *)(param_5 + 0xe) = in_EAX;
    if (in_EAX != -1) {
      actor_consider_target_candidate(param_1);
    }
    if (((8 < (short)param_2) && ((short)param_2 < 0xd)) &&
       (random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f,
       (float)(random_seed_global >> 0x10) * 1.5259022e-05 < 0.4)) {
      param_5[1] = 0x2d;
      return 1;
    }
    if (*(char *)(iVar3 + iVar1 + 6) == '\0') {
      FUN_00403f00(param_1);
      if (param_5[4] != 0xffff) {
        return 1;
      }
      *(undefined1 *)(param_5 + 7) = 0;
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
