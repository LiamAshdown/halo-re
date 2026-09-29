// actor_react_to_flee_point  (Ghidra: actor_react_to_flee_point, renamed)
// address 0x422c00, size 696 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/ai.h actor.aim_origin (0x120), actor.facing (0x174), actor.unit_index/
//   awareness_level/vocalization_*; types/objects.h object_header (stride 0xc,
//   object_data->data + 8 + index*0xc). Same look-at/search-position tail as
//   actor_react_to_registered_danger @0x422930, with priority 4 for the look-at call instead
//   of 2 (confirmed via objdump, bin/halo.exe 0x422c00..0x422ce6). The optional
//   `flee_source_object` argument, when valid, is checked with teams_are_enemies against the
//   actor's own team and, if hostile, raises a category-2 perception event.
//   UNSURE: object+0xb8 is typed in types/objects.h as `name_index` from a scenario-placement
//   reader elsewhere in the codebase; here it is read as the second (team-like) operand of
//   teams_are_enemies, which does not obviously fit that name. Kept as a raw offset rather
//   than asserting the wrong field name.
// register convention: EBX(masked) -> actor_index (stack), stack -> flee_source_object, point.
//   Ghidra already resolved these three as ordinary stack parameters.
//   // blam-cc: stack -> actor_index, flee_source_object, point
// reconciled: R29 object/object_placement_data.name_index -> owner_team (int16 team at 0xb8 / 0x14)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *object_data;     // 0x008603b0

extern double fabs(double x);
extern real random_real_range(real min, real max); // 0x401050
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void actor_record_look_at_point(datum_index actor_index, const uint32_t *point, int16_t priority, uint32_t data); // 0x421bc0
extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0
extern void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data); // 0x422070
extern int8_t teams_are_enemies(int16_t a, int16_t b); // 0x45bd50, CX/DX; UNSURE, see file header

// Variant table paired with the sighted/recognized/directional/danger dialogue families.
extern int16_t actor_dialogue_variant_table_e[]; // 0x00655640

// blam-cc: stack -> actor_index, flee_source_object, point
// Computes a look direction toward `point` (relative to aim origin, falling back to facing
// when degenerate) and, if alert enough and within surprise range, records a look-at point
// toward it at priority 4, then unconditionally queues a matching priority-3 search position.
// If flee_source_object is valid, raises a category-2 perception event when it belongs to a
// hostile team. Finishes by queuing category-6 dialogue carrying `point` as its payload.
void actor_react_to_flee_point(datum_index actor_index, int32_t flee_source_object, const real_point3d *point)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    real_vector3d direction;
    float length;

    direction.i = point->x - self->aim_origin.x;
    direction.j = point->y - self->aim_origin.y;
    direction.k = point->z - self->aim_origin.z;
    length = vector3d_normalize_with_length(&direction);
    if ((float)fabs((double)length) < 0.0001f) {
        direction = self->facing;
    }

    if (self->awareness_level < 3 && length < actor_tag->surprise_distance) {
        actor_record_look_at_point(actor_index, (const uint32_t *)&direction, 4, 0xffffffff);
    }
    actor_queue_search_position(actor_index, 0, 3, &direction, 0xffffffff, 0, 90, 0xffffffff, 0, 0);

    if (flee_source_object != -1) {
        object *source = ((object_header *)object_data->data)[flee_source_object & 0xffff].data;
        if (teams_are_enemies(source->owner_team /* UNSURE, see file header */, self->team) != 0) {
            actor_record_perception_event(actor_index, 2, 0x384);
        }
    }

    if (self->awareness_level > 1 && self->vocalization_line < 7 &&
        (self->mode != 11 || self->mode_data.raw[3] != 0)) {
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

            self->vocalization_state = (int16_t)ticks;
            self->vocalization_line = 6;
            self->vocalization_variant = actor_dialogue_variant_table_e[self->combat_status >= 4];
            self->vocalization_unknown_54c = 3; // kind = 3 (direction/point)
            self->vocalization_unknown_550 = *(uint32_t *)&point->x;
            self->vocalization_unknown_554 = *(uint32_t *)&point->y;
            self->vocalization_unknown_558 = *(uint32_t *)&point->z;
        }
    }
}

#if 0
Original Ghidra decompilation (0x422c00):

void FUN_00422c00(float param_1,float param_2,float *param_3)

{
  short sVar1;
  undefined2 uVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  char cVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float fVar11;
  undefined4 local_10;

  iVar8 = ((uint)param_1 & 0xffff) * 0x724;
  iVar9 = *(int *)(DAT_00880360 + 0x34) + iVar8;
  local_10._2_2_ = (undefined2)((uint)(*param_3 - *(float *)(iVar9 + 0x120)) >> 0x10);
  iVar7 = *(int *)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x58 + iVar8) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  fVar10 = (float10)vector3d_normalize_with_length();
  if (ABS(fVar10) < (float10)9.999999747378752e-05) {
    local_10._2_2_ = (undefined2)((uint)*(undefined4 *)(iVar9 + 0x174) >> 0x10);
  }
  if ((*(short *)(iVar9 + 0x6a) < 3) && (fVar10 < (float10)*(float *)(iVar7 + 0x2b0))) {
    FUN_00421bc0(0xffffffff);
  }
  FUN_00421af0(0xffffffff,0,0x5a,0xffffffff,0,0);
  if ((param_2 != -NAN) && (cVar6 = FUN_0045bd50(), cVar6 != '\0')) {
    FUN_00422070();
  }
  iVar7 = *(int *)(DAT_00880360 + 0x34);
  fVar3 = param_3[1];
  sVar1 = *(short *)(iVar7 + 0x6a + iVar8);
  fVar4 = *param_3;
  fVar5 = param_3[2];
  iVar9 = iVar7 + iVar8;
  iVar7 = *(int *)((*(uint *)(iVar7 + 0x58 + iVar8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_10 = CONCAT22(local_10._2_2_,3);
  if (((1 < sVar1) && (*(short *)(iVar9 + 0x544) < 7)) &&
     ((*(short *)(iVar9 + 0x6c) != 0xb || (*(char *)(iVar9 + 0x9f) != '\0')))) {
    param_1 = 0.9;
    if ((sVar1 < 3) || (*(short *)(iVar9 + 0x6e) == 0)) {
      param_1 = 1.8;
    }
    if ((*(float *)(iVar7 + 0xd4) != 0.0) || (*(float *)(iVar7 + 0xd8) != 0.0)) {
      if (*(float *)(iVar7 + 0xd4) <= 0.5) {
        param_2 = 0.5;
      }
      else {
        param_2 = *(float *)(iVar7 + 0xd4);
      }
      if (*(float *)(iVar7 + 0xd8) <= 2.0) {
        param_3 = *(float **)(iVar7 + 0xd8);
      }
      else {
        param_3 = (float *)0x40000000;
      }
      fVar11 = random_real_range(param_2,(float)param_3);
      param_1 = fVar11 * param_1;
    }
    iVar7 = (int)ROUND(param_1 * 30.0);
    if (0x7fff < iVar7) {
      iVar7 = 0x7fff;
    }
    uVar2 = *(undefined2 *)(&DAT_00655640 + (uint)(3 < *(short *)(iVar9 + 0x6e)) * 2);
    *(short *)(iVar9 + 0x548) = (short)iVar7;
    *(undefined2 *)(iVar9 + 0x544) = 6;
    *(undefined2 *)(iVar9 + 0x546) = uVar2;
    *(undefined4 *)(iVar9 + 0x54c) = local_10;
    *(float *)(iVar9 + 0x550) = fVar4;
    *(float *)(iVar9 + 0x554) = fVar3;
    *(float *)(iVar9 + 0x558) = fVar5;
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x422c00..0x422d3f), which resolves the (otherwise
"-NAN sentinel") second parameter as a plain object index and confirms the CX/DX pairing at
the FUN_0045bd50 call:

  422cf2: mov edx, ds:0x8603b0        ; object_data
  422cf8: and eax, 0xffff             ; eax = flee_source_object
  422d03: mov ecx, [eax+ecx*4+0x8]    ; ecx = object_header[flee_source_object].data
  422d07: mov cx, [ecx+0xb8]          ; cx = *(int16*)(object+0xb8)
  422d0e: mov dx, [edi+0x3e]          ; dx = actor.team
  422d12: call teams_are_enemies
  422d1f: mov esi, 0x384              ; data = 0x384
  422d24: mov edx, 2                  ; event = 2
  422d29: call actor_record_perception_event
#endif
