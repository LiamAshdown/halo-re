// actor_react_to_registered_danger  (Ghidra: actor_react_to_registered_danger, renamed)
// address 0x422930, size 707 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.danger_type/danger_unknown_284/danger_object_index (0x280/0x284/
//   0x28c), actor.aim_origin (0x120), actor.facing (0x174); calls
//   vector3d_normalize_with_length (0x401990, math module), actor_record_look_at_point
//   (0x421bc0), actor_queue_search_position (0x421af0) and ai_communication_broadcast
//   (0x42d340). Confirmed the point/priority/data values below against bin/halo.exe
//   (0x422930..0x422a56): the local direction vector is normalize(point - aim_origin),
//   falling back to actor.facing when that is degenerate (length below 0.0001), and reused
//   unmodified as both the look-at point and the search-position velocity.
// register convention: EAX -> point (real_point3d*), stack -> actor_index, danger_object_index.
//   // blam-cc: EAX -> point, stack -> actor_index, danger_object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14

extern double fabs(double x);
extern real random_real_range(real min, real max); // 0x401050
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data); // 0x421bc0
extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340

// Variant table paired with the sighted/recognized/directional dialogue families.
extern int16_t actor_dialogue_variant_table_d[]; // 0x00655634

// blam-cc: EAX -> point, stack -> actor_index, danger_object_index
// If the actor already has a matching, still-live danger registered against
// danger_object_index, just re-broadcasts a category-10 "danger" squad event; otherwise
// computes a look direction toward `point` (relative to the actor's aim origin, falling back
// to its current facing when degenerate) and, if alert enough and within surprise range,
// records a look-at point in that direction, then unconditionally queues a matching
// priority-3 search position. Either way, finishes by queuing category-3 dialogue exactly as
// actor_queue_directional_reaction_event does for its direction branch.
void actor_react_to_registered_danger(const real_point3d *point, datum_index actor_index, int32_t danger_object_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    real_vector3d direction;

    if (self->danger_type >= 1 && self->danger_object_index == danger_object_index && self->danger_unknown_284 >= 1) {
        ai_communication_broadcast(10, self->unit_index, (datum_index)k_datum_index_none, (datum_index)k_datum_index_none,
                                   (datum_index)k_datum_index_none, (datum_index)k_datum_index_none, 0);
    } else {
        float length;

        direction.i = point->x - self->aim_origin.x;
        direction.j = point->y - self->aim_origin.y;
        direction.k = point->z - self->aim_origin.z;
        length = vector3d_normalize_with_length(&direction);
        if ((float)fabs((double)length) < 0.0001f) {
            direction = self->facing;
        }

        if (self->awareness_level < 3 && length < actor_tag->surprise_distance) {
            actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 2, 0xffffffff);
        }
        actor_queue_search_position(actor_index, 0, 3, &direction, 0xffffffff, 0, 90, 0xffffffff, 0, 0);
    }

    if (self->awareness_level > 1 && self->vocalization_line < 4 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0) && self->vocalization_unknown_3e8 < 7) {
        float wait_scale = (self->awareness_level < 3 || self->combat_status == 0) ? 1.8f : 0.9f;

        if (actor_tag->event_look_time_modifier[0] != 0.0f || actor_tag->event_look_time_modifier[1] != 0.0f) {
            float min_scale = (actor_tag->event_look_time_modifier[0] <= 0.5f) ? 0.5f : actor_tag->event_look_time_modifier[0];
            float max_scale = (actor_tag->event_look_time_modifier[1] <= 2.0f) ? actor_tag->event_look_time_modifier[1] : 2.0f;
            wait_scale = random_real_range(min_scale, max_scale) * wait_scale;
        }

        {
            int32_t ticks = (int32_t)(wait_scale * 30.0f + 0.5f); // ROUND
            if (ticks > 0x7fff) {
                ticks = 0x7fff;
            }

            self->vocalization_variant = actor_dialogue_variant_table_d[self->combat_status >= 4];
            self->vocalization_state = (int16_t)ticks;
            self->vocalization_line = 3;
            self->vocalization_unknown_54c = 3; // kind = 3 (direction vector)
            self->vocalization_unknown_550 = *(uint32_t *)&point->x;
            self->vocalization_unknown_554 = *(uint32_t *)&point->y;
            self->vocalization_unknown_558 = *(uint32_t *)&point->z;
        }
    }
}

#if 0
Original Ghidra decompilation (0x422930):

void FUN_00422930(float param_1,int param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  float fVar8;
  float local_18;
  float local_14;
  undefined4 local_10;

  iVar6 = ((uint)param_1 & 0xffff) * 0x724;
  iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar6;
  iVar4 = *(int *)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar6) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  if (((*(short *)(iVar5 + 0x280) < 1) || (*(int *)(iVar5 + 0x28c) != param_2)) ||
     (*(short *)(iVar5 + 0x284) < 1)) {
    local_10._2_2_ = (undefined2)((uint)(*in_EAX - *(float *)(iVar5 + 0x120)) >> 0x10);
    fVar7 = (float10)vector3d_normalize_with_length();
    if (ABS(fVar7) < (float10)9.999999747378752e-05) {
      local_10._2_2_ = (undefined2)((uint)*(undefined4 *)(iVar5 + 0x174) >> 0x10);
    }
    if ((*(short *)(iVar5 + 0x6a) < 3) && (fVar7 < (float10)*(float *)(iVar4 + 0x2b0))) {
      FUN_00421bc0(0xffffffff);
    }
    FUN_00421af0(0xffffffff,0,0x5a,0xffffffff,0,0);
  }
  else {
    ai_communication_broadcast
              (10,*(undefined4 *)(iVar5 + 0x18),0xffffffff,0xffffffff,0xffffffff,0xffffffff,0);
  }
  fVar1 = in_EAX[1];
  fVar2 = *in_EAX;
  fVar3 = in_EAX[2];
  iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar6;
  iVar4 = *(int *)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar6) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  local_10 = CONCAT22(local_10._2_2_,3);
  if ((((1 < *(short *)(iVar5 + 0x6a)) && (*(short *)(iVar5 + 0x544) < 4)) &&
      ((*(short *)(iVar5 + 0x6c) != 0xb || (*(char *)(iVar5 + 0x9f) != '\0')))) &&
     (*(short *)(iVar5 + 1000) < 7)) {
    param_1 = 0.9;
    if ((*(short *)(iVar5 + 0x6a) < 3) || (*(short *)(iVar5 + 0x6e) == 0)) {
      param_1 = 1.8;
    }
    if ((*(float *)(iVar4 + 0xd4) != 0.0) || (*(float *)(iVar4 + 0xd8) != 0.0)) {
      if (*(float *)(iVar4 + 0xd4) <= 0.5) {
        local_14 = 0.5;
      }
      else {
        local_14 = *(float *)(iVar4 + 0xd4);
      }
      if (*(float *)(iVar4 + 0xd8) <= 2.0) {
        local_18 = *(float *)(iVar4 + 0xd8);
      }
      else {
        local_18 = 2.0;
      }
      fVar8 = random_real_range(local_14,local_18);
      param_1 = fVar8 * param_1;
    }
    iVar4 = (int)ROUND(param_1 * 30.0);
    if (0x7fff < iVar4) {
      iVar4 = 0x7fff;
    }
    *(undefined2 *)(iVar5 + 0x546) =
         *(undefined2 *)(&DAT_00655634 + (uint)(3 < *(short *)(iVar5 + 0x6e)) * 2);
    *(short *)(iVar5 + 0x548) = (short)iVar4;
    *(undefined2 *)(iVar5 + 0x544) = 3;
    *(undefined4 *)(iVar5 + 0x54c) = local_10;
    *(float *)(iVar5 + 0x550) = fVar2;
    *(float *)(iVar5 + 0x554) = fVar1;
    *(float *)(iVar5 + 0x558) = fVar3;
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x422930..0x422a56) confirming EAX=point and the
exact look-at/search-position argument setup (see file header).
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
