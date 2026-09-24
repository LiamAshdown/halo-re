// actor_report_firing_position_request  (Ghidra: actor_report_firing_position_request, renamed)
// address 0x4120f0, size 415 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: it is the step actor_firing_position_evaluate @0x412820 and
//   actor_find_best_firing_position @0x412ba0 run between the scoring and the rejection
//   passes, and the only field it writes is candidate.request_result (0x06), which
//   actor_reject_firing_position_by_request_result @0x412620 then turns into a penalty.
//   0x569190 is the unit-side request submitter and 0x42b270 the perception evaluator; both
//   live outside this module.
// register convention: actor_index in EAX, the query in ECX and the candidate in EBX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360

extern real vector2d_normalize_with_length(real_vector2d *v); // 0x4018e0
extern void unit_add_marker_relative_offset(datum_index unit_index, uint32_t mode, void *point, void *direction,
                         void *offset);                        // 0x569190, not yet rewritten (units)
extern int16_t actor_evaluate_engagement_reachability(uint32_t kind, uint32_t enabled, uint32_t object_index,
                            uint32_t in_vehicle);              // 0x42b270, not yet rewritten

// blam-cc: EAX -> actor_index, ECX -> query, EBX -> candidate
// Submits the movement or aim the actor would make if it took this candidate and records
// the perception result code on the candidate. Goal kind 5 (pursuing) short-circuits: a
// candidate more than six units away is written off as result 4 without asking. Otherwise
// the request is mode 2 for goal kinds 1 and 2, mode 3 with the standing gun offset and a
// flattened facing direction when the query has one, and mode 1 as the fallback.
void actor_report_firing_position_request(datum_index actor_index,
                                          actor_firing_position_query *query,
                                          actor_firing_position_candidate *candidate)
{
    actor *self;
    real_vector3d facing;
    real_point3d *point;
    real_vector3d *direction;
    void *offset;
    uint32_t mode;
    uint32_t kind;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (query->goal_kind == 5) {
        if (candidate->distance_from_actor < 6.0f) {
            unit_add_marker_relative_offset(self->unit_index, 1, (void *)candidate->position, (void *)0, (void *)0);
            candidate->request_result = actor_evaluate_engagement_reachability(0, 0, 0xffffffff,
                                                     (uint32_t)(self->active_unit_index !=
                                                                (datum_index)0xffffffff));
            return;
        }
        candidate->request_result = 4;
        return;
    }

    offset = (void *)0;
    direction = (real_vector3d *)0;

    if (query->goal_kind == 1 || query->goal_kind == 2) {
        mode = 2;
    } else if (query->have_standing_gun_offset != 0) {
        point = (real_point3d *)candidate->position;
        mode = 3;
        offset = &query->standing_gun_offset;
        facing.i = query->target_aim_position.x - point->x;
        facing.j = query->target_aim_position.y - point->y;
        facing.k = query->target_aim_position.z - point->z;
        // Only the horizontal part is normalized; a degenerate result falls back on the
        // actor own working position, which the original passes as a vector.
        if (vector2d_normalize_with_length((real_vector2d *)&facing) <= 0.0f) {
            direction = (real_vector3d *)&self->facing;
        } else {
            facing.k = 0.0f;
            direction = &facing;
        }
    } else {
        mode = 1;
    }

    unit_add_marker_relative_offset(self->unit_index, mode, (void *)candidate->position, direction, offset);

    kind = (query->goal_kind >= 1 && query->goal_kind <= 3) ? 1 : 0;
    candidate->request_result = actor_evaluate_engagement_reachability(kind, 1, (uint32_t)query->target_relationship_object,
                                             (uint32_t)(self->active_unit_index !=
                                                        (datum_index)0xffffffff));
}

#if 0
Original Ghidra decompilation (0x4120f0):

void FUN_004120f0(void)

{
  short sVar1;
  undefined2 uVar2;
  uint in_EAX;
  int iVar3;
  float *pfVar4;
  int in_ECX;
  undefined4 *unaff_EBX;
  int iVar5;
  undefined4 uVar6;
  float10 fVar7;
  float local_18;
  float local_14;
  float local_10;

  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  sVar1 = *(short *)(in_ECX + 4);
  if (sVar1 == 5) {
    if ((float)unaff_EBX[2] < 6.0) {
      FUN_00569190(*(undefined4 *)(iVar3 + 0x18),1,*unaff_EBX,0,0);
      uVar2 = FUN_0042b270(0,0,0xffffffff,
                           CONCAT31((int3)((uint)*(int *)(iVar3 + 0x158) >> 8),
                                    *(int *)(iVar3 + 0x158) != -1));
      *(undefined2 *)((int)unaff_EBX + 6) = uVar2;
      return;
    }
    *(undefined2 *)((int)unaff_EBX + 6) = 4;
    return;
  }
  iVar5 = 0;
  if ((sVar1 == 1) || (sVar1 == 2)) {
    uVar6 = 2;
  }
  else {
    if (*(char *)(in_ECX + 0x5dc) != '\0') {
      pfVar4 = (float *)*unaff_EBX;
      local_18 = *(float *)(in_ECX + 0x610) - *pfVar4;
      uVar6 = 3;
      iVar5 = in_ECX + 0x5e0;
      local_14 = *(float *)(in_ECX + 0x614) - pfVar4[1];
      local_10 = *(float *)(in_ECX + 0x618) - pfVar4[2];
      fVar7 = (float10)vector2d_normalize_with_length();
      if (fVar7 <= (float10)0.0) {
        pfVar4 = (float *)(iVar3 + 0x174);
      }
      else {
        local_10 = 0.0;
        pfVar4 = &local_18;
      }
      goto LAB_0041221a;
    }
    uVar6 = 1;
  }
  pfVar4 = (float *)0x0;
LAB_0041221a:
  FUN_00569190(*(undefined4 *)(iVar3 + 0x18),uVar6,*unaff_EBX,pfVar4,iVar5);
  if ((*(short *)(in_ECX + 4) < 1) || (uVar6 = 1, 3 < *(short *)(in_ECX + 4))) {
    uVar6 = 0;
  }
  uVar2 = FUN_0042b270(uVar6,1,*(undefined4 *)(in_ECX + 0x62c),
                       CONCAT31((int3)((uint)*(int *)(iVar3 + 0x158) >> 8),
                                *(int *)(iVar3 + 0x158) != -1));
  *(undefined2 *)((int)unaff_EBX + 6) = uVar2;
  return;
}
#endif
