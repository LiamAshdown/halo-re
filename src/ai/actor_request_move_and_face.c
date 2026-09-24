// actor_request_move_and_face  (Ghidra: actor_request_move_and_face, renamed)
// address 0x4049d0, size 439 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: types/ai.h actor.swarm/order_committed/needs_new_path/firing_position_index;
//   types/tags.h Actor.guard_position_time (already-named field, used here as a random wait
//   range exactly as its name suggests); phase-4 summary "issues a move-and-face pathfinding
//   request ... and schedules a follow-up random wait once the move completes".
// register convention: actor index in EAX (param_1), the sole real parameter.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor+0xaa/0xb0/0x9c/0xc0/0xc4 all fall inside actor.mode_data (a per-mode union,
//   see types/ai.h); read/written here as a small "move and face" sub-state (state code at
//   0xc0, two flags at 0xaa/0xb0, a waypoint index at 0xc4, a wait-ticks counter at 0x9c),
//   not independently confirmed.
//   UNSURE: actor_get_firing_position_group_mask, actor_find_best_firing_position, actor_claim_firing_position and actor_push_recognition_entry are all outside this
//   session's range and unreviewed; the request/scratch buffer sizes and layout below mirror
//   Ghidra's stack slots as-is (see actor_update_path_if_needed.c for the same caveat about
//   not mapping them onto a named struct).
//   UNSURE: the return value is built as `result & 0xffffff00` in the original, i.e. its low
//   byte is unconditionally zeroed and the early-out paths return an unrelated byte of the
//   actor-tag pointer shifted into the high bits. This function has no observed callers in
//   this module, so the return value's real meaning (if any) could not be cross-checked;
//   modelled here as a plain int32_t exactly matching the bit-for-bit computation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>
#include <stdint.h>

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern real random_real_range(real min, real max); // 0x401050
extern uint32_t actor_get_firing_position_group_mask(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_get_firing_position_group_mask at 0x412880
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_find_best_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_find_best_firing_position at 0x412ba0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern int16_t actor_claim_firing_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_claim_firing_position at 0x414060
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern void actor_push_recognition_entry(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_push_recognition_entry at 0x4141a0
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.

int32_t actor_request_move_and_face(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    uint8_t *mode_data = a->mode_data;
    int16_t *state = (int16_t *)(mode_data + (0xc0 - 0x9c));
    uint8_t *flag_aa = mode_data + (0xaa - 0x9c);
    uint8_t *flag_b0 = mode_data + (0xb0 - 0x9c);
    int16_t *waypoint = (int16_t *)(mode_data + (0xc4 - 0x9c));
    int16_t *wait_ticks = (int16_t *)(mode_data + (0x9c - 0x9c));
    uint32_t garbage_hint = ((uint32_t)(uintptr_t)actor_def >> 8) << 8; // UNSURE, see note above
    uint32_t result;

    if (a->swarm != 0) {
        *state = 1;
        return (int32_t)garbage_hint;
    }
    if (a->order_committed != 0) {
        *state = 1;
        *flag_aa = 1;
        return (int32_t)garbage_hint;
    }

    result = 0;
    if (*state == 3 && a->firing_position_index == -1) {
        *state = 0;
        *flag_aa = 1;
    }

    if (a->needs_new_path != 0 && *flag_aa != 0) {
        uint32_t request_block[16];
        uint8_t buffer_a[60];
        uint32_t out_waypoint;
        uint8_t scratch_context[65684];
        uint8_t out_flag;
        int16_t result_waypoint;

        if (*state == 3 && a->firing_position_index != -1) {
            actor_push_recognition_entry();
        }

        memset(request_block, 0, sizeof(request_block));
        request_block[1] = 4; // kind/formation selector consumed by actor_find_best_firing_position
        request_block[0] = actor_get_firing_position_group_mask(0);
        out_flag = 1; // matches uStack_106eb's pre-set value; likely an "in-progress" flag

        actor_find_best_firing_position(actor_index, request_block, buffer_a, &out_waypoint, scratch_context, &out_flag);
        result_waypoint = actor_claim_firing_position(actor_index, out_waypoint, scratch_context);
        *flag_aa = 0;
        *flag_b0 = 0;
        if (result_waypoint == -1) {
            *state = 1;
        } else {
            *state = 3;
            *waypoint = result_waypoint;
        }

        {
            float delay = random_real_range(actor_def->guard_position_time[0], actor_def->guard_position_time[1]);
            result = (uint32_t)(int32_t)delay; // __ftol
            *wait_ticks = (int16_t)result;
        }
    }
    return (int32_t)(result & 0xffffff00u);
}

#if 0
Original Ghidra decompilation (0x4049d0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_004049d0(uint param_1)

{
  short sVar1;
  uint3 uVar3;
  uint uVar2;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined1 uStack_10745;
  int iStack_10744;
  undefined4 uStack_10740;
  undefined1 auStack_1073c [60];
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 uStack_106eb;
  undefined1 auStack_10098 [65684];

  iVar4 = (param_1 & 0xffff) * 0x724;
  iVar5 = iVar4 + *(int *)(DAT_00880360 + 0x34);
  iStack_10744 = *(int *)((*(uint *)(iVar4 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20
                          + 0x14 + DAT_0087bc14);
  uVar3 = (uint3)((uint)iStack_10744 >> 8);
  if (*(char *)(iVar5 + 6) != '\0') {
    *(undefined2 *)(iVar5 + 0xc0) = 1;
    return (uint)uVar3 << 8;
  }
  if (*(char *)(iVar5 + 0x160) != '\0') {
    *(undefined2 *)(iVar5 + 0xc0) = 1;
    *(undefined1 *)(iVar5 + 0xaa) = 1;
    return (uint)uVar3 << 8;
  }
  uVar2 = 0;
  if ((*(short *)(iVar5 + 0xc0) == 3) && (*(short *)(iVar5 + 0x3b8) == -1)) {
    *(undefined2 *)(iVar5 + 0xc0) = 0;
    *(undefined1 *)(iVar5 + 0xaa) = 1;
  }
  if ((*(char *)(iVar5 + 0x4c) != '\0') && (*(char *)(iVar5 + 0xaa) != '\0')) {
    if ((*(short *)(iVar5 + 0xc0) == 3) && (*(short *)(iVar5 + 0x3b8) != -1)) {
      FUN_004141a0();
    }
    puVar6 = &uStack_10700;
    for (iVar4 = 0x199; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    uStack_106fc = 4;
    uStack_10700 = FUN_00412880(0);
    uStack_106eb = 1;
    FUN_00412ba0(param_1,&uStack_10700,auStack_1073c,&uStack_10740,auStack_10098,&uStack_10745);
    sVar1 = FUN_00414060(param_1,uStack_10740,auStack_10098);
    *(undefined1 *)(iVar5 + 0xaa) = 0;
    *(undefined1 *)(iVar5 + 0xb0) = 0;
    if (sVar1 == -1) {
      *(undefined2 *)(iVar5 + 0xc0) = 1;
    }
    else {
      *(undefined2 *)(iVar5 + 0xc0) = 3;
      *(short *)(iVar5 + 0xc4) = sVar1;
    }
    random_real_range(*(float *)(iStack_10744 + 0x3b8),*(float *)(iStack_10744 + 0x3bc));
    uVar2 = __ftol();
    *(short *)(iVar5 + 0x9c) = (short)uVar2;
  }
  return uVar2 & 0xffffff00;
}
#endif
