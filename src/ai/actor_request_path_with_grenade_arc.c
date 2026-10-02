// actor_request_path_with_grenade_arc  (Ghidra: actor_request_path_with_grenade_arc, renamed)
// address 0x408300, size 358 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// REWRITTEN from objdump 0x408300..0x408465: the "uncover" mode's +0x0c process (stack actor index; the draft read
//   it from EAX and called the firing-position helpers with the wrong arguments). While the actor wants a path
//   (+0x4c), has not committed to an order (+0x160) and its mode is not done (+0x9d), it picks a firing position with
//   goal kind 3: with an explicit point (mode +0x08 == 1) aimed at the mode's point (+0x14, object +0x10, surface
//   word +0x0c), otherwise preferring the alternate aim point as the mode says (+0x04). Picked: a point-less search
//   that finds nothing reachable (request result other than 0/1) marks +0x04 (+0xa0); an explicit search whose pick
//   is visible and within the accuracy radius marks +0x20 (+0xbc). A failed claim marks +0x02 (+0x9e). Returns 0.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

extern int16_t actor_select_firing_position(datum_index actor_index, actor_firing_position_query *query,
    actor_firing_position_candidate *out_candidate, uint32_t *out_previous_owner, path_find_context *path_context,
    uint8_t *out_path_ok); // 0x413e50, stack, EBX query, EDI candidate
extern int16_t actor_claim_firing_position(datum_index actor_index, datum_index previous_owner,
    path_find_context *path_context, int16_t firing_position_index, uint8_t path_ok); // 0x414060, stack, CX, AL
extern float actor_compute_accuracy_scale(datum_index actor_index); // 0x429620, EAX

uint8_t actor_request_path_with_grenade_arc(uint32_t actor_index)
{
    uint8_t *actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    static actor_firing_position_query query;
    static path_find_context path_context;
    actor_firing_position_candidate candidate;
    uint32_t previous_owner = 0xffffffff;
    uint8_t path_ok = 0;
    int16_t selected;

    if (actor[0x4c] == 0 || actor[0x160] != 0 || actor[0x9d] != 0) {
        return 0;
    }
    memset(&query, 0, sizeof(query));
    memset(&candidate, 0, sizeof(candidate));
    query.goal_kind = 3;
    if (*(int16_t *)(actor + 0xa4) == 1) {
        query.have_explicit_target = 1;
        query.explicit_target_position = *(real_point3d *)(actor + 0xb0);
        query.explicit_target_object = *(uint32_t *)(actor + 0xac);
        query.explicit_target_unknown_34 = *(int16_t *)(actor + 0xa8);
    } else {
        query.use_last_seen_position = actor[0xa0];
    }
    selected = actor_select_firing_position(actor_index, &query, &candidate, &previous_owner, &path_context, &path_ok);
    actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    if (selected != -1) {
        if (*(int16_t *)(actor + 0xa4) == 0) {
            if (candidate.request_result != 0 && candidate.request_result != 1) {
                actor[0xa0] = 1;
            }
        } else if (candidate.request_result == 0 && actor_compute_accuracy_scale(actor_index) > candidate.distance_from_actor) {
            actor[0xbc] = 1;
        }
    }
    if (actor_claim_firing_position(actor_index, previous_owner, &path_context, selected, path_ok) == -1) {
        actor = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
        actor[0x9e] = 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x408300):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

uint FUN_00408300(uint param_1)

{
  char cVar1;
  short sVar2;
  undefined3 uVar4;
  uint uVar3;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  float10 fVar8;
  undefined1 uStack_10741;
  undefined4 auStack_10740 [2];
  short sStack_10736;
  float fStack_10734;
  undefined4 uStack_10700;
  undefined2 uStack_106fc;
  undefined1 uStack_106e0;
  undefined4 uStack_106dc;
  undefined4 uStack_106d8;
  undefined4 uStack_106d4;
  undefined4 uStack_106d0;
  undefined2 uStack_106cc;
  undefined1 uStack_106bf;
  undefined1 auStack_10098 [65684];

  iVar6 = (param_1 & 0xffff) * 0x724;
  cVar1 = *(char *)(iVar6 + 0x4c + *(int *)(DAT_00880360 + 0x34));
  uVar4 = (undefined3)((uint)DAT_00880360 >> 8);
  uVar3 = CONCAT31(uVar4,cVar1);
  iVar6 = iVar6 + *(int *)(DAT_00880360 + 0x34);
  if (((cVar1 != '\0') &&
      (uVar3 = CONCAT31(uVar4,*(char *)(iVar6 + 0x160)), *(char *)(iVar6 + 0x160) == '\0')) &&
     (uVar3 = CONCAT31(uVar4,*(char *)(iVar6 + 0x9d)), *(char *)(iVar6 + 0x9d) == '\0')) {
    puVar7 = &uStack_10700;
    for (iVar5 = 0x199; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    uStack_106fc = 3;
    if (*(short *)(iVar6 + 0xa4) == 1) {
      uStack_106e0 = 1;
      uStack_106dc = *(undefined4 *)(iVar6 + 0xb0);
      uStack_106d8 = *(undefined4 *)(iVar6 + 0xb4);
      uStack_106d4 = *(undefined4 *)(iVar6 + 0xb8);
      uStack_106d0 = *(undefined4 *)(iVar6 + 0xac);
      uStack_106cc = *(undefined2 *)(iVar6 + 0xa8);
    }
    else {
      uStack_106bf = *(undefined1 *)(iVar6 + 0xa0);
    }
    sVar2 = FUN_00413e50(param_1,auStack_10740,auStack_10098,&uStack_10741);
    if (sVar2 != -1) {
      if (*(short *)(iVar6 + 0xa4) == 0) {
        if ((sStack_10736 != 0) && (sStack_10736 != 1)) {
          *(undefined1 *)(iVar6 + 0xa0) = 1;
        }
      }
      else if (sStack_10736 == 0) {
        fVar8 = (float10)FUN_00429620();
        if ((float10)fStack_10734 < fVar8) {
          *(undefined1 *)(iVar6 + 0xbc) = 1;
        }
      }
    }
    uVar3 = FUN_00414060(param_1,auStack_10740[0],auStack_10098);
    if ((short)uVar3 == -1) {
      *(undefined1 *)(iVar6 + 0x9e) = 1;
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
