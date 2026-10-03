// actor_is_within_alert_range  (Ghidra: actor_is_within_alert_range, already named)
// address 0x408f30, size 319 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (checked against objdump 0x408f30..0x40906e)
// evidence: types/ai.h actor.position-cache offsets (0x12c/0x130/0x134, the same "cached
//   position" pattern used throughout this session); types/objects.h object.vitality_flags
//   (_object_health_frozen_bit)/velocity/up; phase-4 summary "returns whether a point or
//   object lies within a given alert radius of the actor, subject to the actor's perception
//   and vitality state".
// register convention: actor index in EAX, an object index in ECX, plus four recognized
//   stack parameters (a "range covers everything" flag, two candidate radii, a "vitality
//   check only" flag, and a "use the vehicle radius" selector).
//   // blam-cc: EAX -> actor_index, ECX -> object_index,
//   //          stack -> always_in_range/radius_a/radius_b/vitality_only/use_radius_b
// UNSURE: actor_build_order_search_object.c and actor_build_order_investigate_encounter_
//   point.c both call this passing 0 for always_in_range/vitality_only/use_radius_b, so
//   which of radius_a/radius_b is semantically "the" radius in those call sites cannot be
//   distinguished from this function alone; both are forwarded through unchanged there.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0

extern void object_get_position(real_point3d *out_position, datum_index object_index); // 0x4f6900, objects module

uint8_t actor_is_within_alert_range(uint8_t always_in_range, float radius_a, float radius_b, uint8_t vitality_only, uint8_t use_radius_b, uint32_t actor_index, uint32_t object_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t result = 0;

    if ((obj->vitality_flags & _object_health_frozen_bit) == 0) {
        if (always_in_range == 0) {
            float radius = (use_radius_b == 0) ? radius_b : radius_a;
            real_point3d obj_position;

            object_get_position(&obj_position, object_index);

            if (vitality_only != 0 ||
                // 0x408fd4..0x40900a: (dz*dz + dx*dx) + dy*dy against radius*radius
                ((obj_position.z - a->body_position.z) * (obj_position.z - a->body_position.z) +
                 (obj_position.x - a->body_position.x) * (obj_position.x - a->body_position.x) +
                 (obj_position.y - a->body_position.y) * (obj_position.y - a->body_position.y) < radius * radius)) {
                result = 1;
                if (vitality_only != 0) {
                    return 1;
                }
                if (use_radius_b == 0 &&
                    obj->velocity.i * obj->velocity.i + obj->velocity.j * obj->velocity.j + obj->velocity.k * obj->velocity.k > 0.00027777778f) {
                    result = 0;
                }
            }
            goto check_upright;
        }
        result = 1;
    }
    if (vitality_only != 0) {
        return result;
    }
check_upright:
    if (obj->up.k < 0.5f) { // 0x408f7d: a tipped vehicle (up.k below 0.5) never qualifies; NaN keeps the result
        return 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x408f30):

undefined1
actor_is_within_alert_range(char param_1,float param_2,float param_3,char param_4,char param_5)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint in_EAX;
  int iVar5;
  uint in_ECX;
  undefined1 uVar6;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  iVar5 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  uVar6 = 0;
  if ((*(byte *)(iVar1 + 0x106) & 4) == 0) {
    if (param_1 == '\0') {
      if (param_5 == '\0') {
        local_10 = param_3;
      }
      else {
        local_10 = param_2;
      }
      object_get_position();
      if ((param_4 != '\0') ||
         (fVar2 = local_c - *(float *)(iVar5 + 300), fVar3 = local_8 - *(float *)(iVar5 + 0x130),
         fVar4 = local_4 - *(float *)(iVar5 + 0x134),
         fVar3 * fVar3 + fVar2 * fVar2 + fVar4 * fVar4 < local_10 * local_10)) {
        uVar6 = 1;
        if (param_4 != '\0') {
          return 1;
        }
        if ((param_5 == '\0') &&
           (0.00027777778 <
            *(float *)(iVar1 + 0x70) * *(float *)(iVar1 + 0x70) +
            *(float *)(iVar1 + 0x6c) * *(float *)(iVar1 + 0x6c) +
            *(float *)(iVar1 + 0x68) * *(float *)(iVar1 + 0x68))) {
          uVar6 = 0;
        }
      }
      goto LAB_00408f7d;
    }
    uVar6 = 1;
  }
  if (param_4 != '\0') {
    return uVar6;
  }
LAB_00408f7d:
  if (0.5 <= *(float *)(iVar1 + 0x88)) {
    return uVar6;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
