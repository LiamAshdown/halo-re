// actor_build_order_look  (Ghidra: actor_build_order_look, renamed)
// address 0x4046c0, size 345 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop against objdump 0x4046c0..0x404818; offsets probed)
// evidence: types/ai.h actor.order_committed/swarm/actor_definition_tag/unknown_3a8;
//   types/tags.h Actor.hide_behind_cover_time/cowering_time (already-named fields, selected
//   by the request's "combat" flag exactly as their names suggest); phase-4 summary
//   "making the actor look in a randomly chosen direction, or at an explicit target point".
// register convention: actor index in EAX, order pointer in ESI, a caller-owned look-request
//   block in EBX.
// blam-cc: EAX -> actor_index, ESI -> order, EBX -> request
// actor_look_request (the EBX block) now lives in types/ai.h.
// actor+0x12c/0x130/0x134 is actor.body_position, used here as a cached source point, the
//   same way actor_select_move_position.c uses it.
//   Order-record byte offsets below were re-derived carefully from the original's mixed
//   dword-index (unaff_ESI + N => byte N*4) and byte-cast ((int)unaff_ESI + N => byte N)
//   pointer arithmetic; each write below notes which the original used.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "fn_ai.h"


extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern real random_real_range(real min, real max);          // 0x401050
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990

// Builds a "look" order (code 0). If the actor has not committed to an order and is not a
// swarm: picks a randomized look-duration from the actor tag's cowering_time (when
// request->force_random is set) or hide_behind_cover_time (otherwise) timing pair, unless
// actor.unknown_3a8 already holds a pending duration and force_random is clear. Then, if the
// request carries no explicit direction (explicit_direction == -1), leaves the order as a
// plain "look randomly" order; otherwise records the explicit direction, and if the request
// also carries a target point, records the direction from the actor's cached position to
// that point (aborting the point if it normalizes to zero length).
int32_t actor_build_order_look(uint32_t actor_index, actor_order *order, actor_look_request *request)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *o = (uint8_t *)order;
    uint32_t *body = (uint32_t *)order;
    int32_t i;
    uint8_t force_random;
    uint8_t need_random_duration;

    for (i = 0x11; i != 0; i--) {
        *body = 0;
        body++;
    }

    if (a->order_committed != 0 || a->swarm != 0) {
        *(int16_t *)(o + 0x24) = 1; // order kind
        *(int32_t *)(o + 0x3c) = -1;
        return 1;
    }

    order->order_code = 0;
    o[0x08] = 1;
    *(int16_t *)(o + 0x0c) = 0;

    force_random = request->force_random > 0;
    o[0x09] = force_random;
    need_random_duration = (a->unknown_3a8 < 1) || force_random;
    o[0x0a] = need_random_duration ? 0 : 1;

    if (need_random_duration) {
        Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
        float min, max;

        if (o[0x09] == 0) {
            min = actor_def->hide_behind_cover_time[0];
            max = actor_def->hide_behind_cover_time[1];
        } else {
            min = actor_def->cowering_time[0];
            max = actor_def->cowering_time[1];
        }
        // 0x40476a..0x40477d: random seconds * 30.0 (0x672ac8) -> ticks, then __ftol. FIXED 2026-09-27: the draft
        // dropped the * 30, so look durations were 1/30 of the original.
        *(int16_t *)(o + 0x0c) = (int16_t)(int32_t)(random_real_range(min, max) * 30.0f);
    }

    if (request->explicit_direction == -1) {
        *(int16_t *)(o + 0x24) = 0;
        o[0x0e] = 1;
        *(int32_t *)(o + 0x3c) = -1;
        return 1;
    }
    o[0x0e] = 0;
    *(int16_t *)(o + 0x24) = 3;
    *(int16_t *)(o + 0x28) = request->explicit_direction;
    if (request->has_target_point != 0) {
        real_vector3d *direction = (real_vector3d *)(o + 0x18);

        o[0x14] = 1;
        o[0x15] = 1;
        direction->i = request->target_point.x - a->body_position.x;
        direction->j = request->target_point.y - a->body_position.y;
        direction->k = request->target_point.z - a->body_position.z;
        // 0x4047c6..0x4047e3: normalized IN PLACE (ECX = order + 0x18). FIXED 2026-09-27: the draft normalized a
        // local copy and left the stored direction unnormalized.
        if (vector3d_normalize_with_length(direction) == 0.0f) {
            o[0x14] = 0;
            *(int32_t *)(o + 0x3c) = -1;
            return 1;
        }
    }
    *(int32_t *)(o + 0x3c) = -1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4046c0):

undefined4 FUN_004046c0(void)

{
  int iVar1;
  bool bVar2;
  char cVar3;
  undefined2 uVar4;
  uint in_EAX;
  int iVar5;
  int iVar6;
  int unaff_EBX;
  undefined4 *unaff_ESI;
  undefined4 *puVar7;
  float10 fVar8;
  float min;
  float max;

  iVar5 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = *(int *)((*(uint *)(iVar5 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  puVar7 = unaff_ESI;
  for (iVar6 = 0x11; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar7 = 0;
    puVar7 = puVar7 + 1;
  }
  if ((*(char *)(iVar5 + 0x160) == '\0') && (*(char *)(iVar5 + 6) == '\0')) {
    *(undefined2 *)unaff_ESI = 0;
    *(undefined1 *)(unaff_ESI + 2) = 1;
    *(undefined2 *)(unaff_ESI + 3) = 0;
    bVar2 = 0 < *(short *)(unaff_EBX + 0xc);
    *(bool *)((int)unaff_ESI + 9) = bVar2;
    if ((*(short *)(iVar5 + 0x3a8) < 1) || (bVar2)) {
      cVar3 = '\0';
    }
    else {
      cVar3 = '\x01';
    }
    *(char *)((int)unaff_ESI + 10) = cVar3;
    if (cVar3 == '\0') {
      if (*(char *)((int)unaff_ESI + 9) == '\0') {
        max = *(float *)(iVar1 + 0x2d4);
        min = *(float *)(iVar1 + 0x2d0);
      }
      else {
        max = *(float *)(iVar1 + 0x29c);
        min = *(float *)(iVar1 + 0x298);
      }
      random_real_range(min,max);
      uVar4 = __ftol();
      *(undefined2 *)(unaff_ESI + 3) = uVar4;
    }
    if (*(short *)(unaff_EBX + 8) == -1) {
      *(undefined2 *)(unaff_ESI + 9) = 0;
      *(undefined1 *)((int)unaff_ESI + 0xe) = 1;
      unaff_ESI[0xf] = 0xffffffff;
      return 1;
    }
    *(undefined1 *)((int)unaff_ESI + 0xe) = 0;
    *(undefined2 *)(unaff_ESI + 9) = 3;
    *(undefined2 *)(unaff_ESI + 10) = *(undefined2 *)(unaff_EBX + 8);
    if (*(char *)(unaff_EBX + 0x20) != '\0') {
      *(undefined1 *)(unaff_ESI + 5) = 1;
      *(undefined1 *)((int)unaff_ESI + 0x15) = 1;
      unaff_ESI[6] = *(float *)(unaff_EBX + 0x24) - *(float *)(iVar5 + 300);
      unaff_ESI[7] = *(float *)(unaff_EBX + 0x28) - *(float *)(iVar5 + 0x130);
      unaff_ESI[8] = *(float *)(unaff_EBX + 0x2c) - *(float *)(iVar5 + 0x134);
      fVar8 = (float10)vector3d_normalize_with_length();
      if ((float10)0.0 == fVar8) {
        *(undefined1 *)(unaff_ESI + 5) = 0;
        unaff_ESI[0xf] = 0xffffffff;
        return 1;
      }
    }
  }
  else {
    *(undefined2 *)(unaff_ESI + 9) = 1;
  }
  unaff_ESI[0xf] = 0xffffffff;
  return 1;
}
#endif
