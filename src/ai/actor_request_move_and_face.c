// actor_request_move_and_face  (Ghidra: actor_request_move_and_face, renamed)
// address 0x4049d0, size 439 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x4049d0..0x404b86: the "guard" mode's +0x0c process (stack actor index). Swarms and
//   actors committed to an order (+0x160, which also asks for a new pick +0x0e) only set the guard state (+0x24) to
//   1. A guard state 3 that lost its firing position falls back to 0 and asks for a new pick. When the actor wants
//   a path (+0x4c) and a pick (+0x0e): the held position goes to the recognition history (type 0), a firing
//   position is found with goal kind 4 (its group mask 0x412880, random fallback allowed) and claimed; state 3 and
//   the position (+0x28) on success, 1 otherwise; the pick and +0x14 flags clear, and the next pick waits
//   random(tag +0x3b8, +0x3bc) seconds (+0x9c, ticks). Returns 0.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include <string.h>
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern real random_real_range(real min, real max); // 0x401050
extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind, int16_t search_override); // 0x412880, EAX, SI, stack
extern uint32_t actor_find_best_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x412ba0
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok); // 0x414060, stack, CX, AL
extern void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type); // 0x4141a0, EAX, CX, DL

uint8_t actor_request_move_and_face(datum_index actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *actor_tag = (uint8_t *)tag_instances[((struct actor *)actor)->actor_definition_tag & 0xffff].data;

    if (actor[6] != 0) {
        *(int16_t *)(actor + 0xc0) = 1;
        return 0;
    }
    if (actor[0x160] != 0) {
        *(int16_t *)(actor + 0xc0) = 1;
        actor[0xaa] = 1;
        return 0;
    }
    if (*(int16_t *)(actor + 0xc0) == 3 && ((struct actor *)actor)->firing_position_index == -1) {
        *(int16_t *)(actor + 0xc0) = 0;
        actor[0xaa] = 1;
    }
    if (actor[0x4c] == 0 || actor[0xaa] == 0) {
        return 0;
    }
    if (*(int16_t *)(actor + 0xc0) == 3 && ((struct actor *)actor)->firing_position_index != -1) {
        actor_push_recognition_entry(actor_index, ((struct actor *)actor)->firing_position_index, 0);
    }
    {
        static actor_firing_position_query query;
        static path_find_context path_context;
        actor_firing_position_candidate candidate;
        uint32_t previous_owner = 0xffffffff;
        uint8_t path_ok = 0;
        int16_t found;
        int16_t claimed;

        memset(&query, 0, sizeof(query));
        memset(&candidate, 0, sizeof(candidate));
        query.goal_kind = 4;
        query.group_mask = actor_get_firing_position_group_mask(actor_index, 4, 0);
        query.allow_random_fallback = 1;
        found = (int16_t)actor_find_best_firing_position(actor_index, &query, &candidate, &previous_owner,
            &path_context, &path_ok);
        claimed = actor_claim_firing_position(actor_index, previous_owner, &path_context, found, path_ok);
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
        actor[0xaa] = 0;
        actor[0xb0] = 0;
        if (claimed == -1) {
            *(int16_t *)(actor + 0xc0) = 1;
        } else {
            *(int16_t *)(actor + 0xc0) = 3;
            *(int16_t *)(actor + 0xc4) = claimed;
        }
    }
    *(int16_t *)&((struct actor *)actor)->mode_data = (int16_t)(int32_t)(random_real_range(*(float *)(actor_tag + 0x3b8),
        *(float *)(actor_tag + 0x3bc)) * 30.0f); // __ftol
    return 0;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
