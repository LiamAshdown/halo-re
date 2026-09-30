// unit_predict_aim_target_position  (Ghidra: FUN_00571de0; renamed from the phase2 proposal)
// address 0x571de0, size 296 bytes
// name confidence: 0.3 (phase2 proposal at 0.3, matches functions.md summary)
// rewrite confidence: 0.35
// evidence: types/tags.h Vehicle.vehicle_type (0x2f4); callees object_get_position (0x4f6900),
//   FUN_00502060 (established signature and usage pattern in
//   src/units/unit_test_placement_candidate.c, this module); math.h global_up3d_pointer
//   (0x00696720) and the g_0069672c constant (established in
//   src/objects/damage_effect_new_at_location.c).
// register convention: unit object index in ESI (unaff_ESI); an output position pointer in EBX
//   (unaff_EBX).
//   // blam-cc: ESI -> unit_index, EBX -> out_position
// UNSURE: FUN_00502060's two fixed-zero parameters and the fraction/reference outputs
//   (local_418/local_410) are not confirmed; mirrored from the sibling rewrite.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "physics.h"
#include "fn_units.h"

extern data_array *object_data;         // 0x008603b0
extern tag_instance *tag_instances;     // 0x0087bc14
extern void *global_structure_collision_bsp;              // UNSURE global, passed straight through to FUN_00502060
extern real_vector3d *global_up3d_pointer;  // 0x00696720
extern const real_vector3d *global_down3d_pointer; // 0x0069672c, a POINTER (-> 0x65c25c {0,0,-1})

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern uint8_t collision_bsp_query_segment_init(uint32_t flags, collision_bsp_segment_result *result,
    ModelCollisionGeometryBSP *bsp, int16_t breakable_surface_count, uint32_t *breakable_surfaces, real_point3d *origin,
    real_vector3d *delta, float max_fraction); // 0x502060, EAX flags, ECX result

// Attempts to compute a projected/predicted aim position in front of the unit for certain
// vehicle sub-types (0, 1, 4, 6), validating line-of-clearance via a collision test. Writes the
// result through out_position and returns a non-negative value on success, or -1 on failure or
// for unsupported sub-types.
int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    real_point3d base_position;
    real_vector3d delta;
    char hit;
    float hit_fraction = 0.0f;
    int32_t hit_result = 0;
    collision_bsp_segment_result segment_result; // [esp+0x20], 0x418 bytes (the 0x430 frame)

    object_get_position(&base_position, unit_index);

    switch (tag->vehicle_type) {
    case 0: case 1: case 4: case 6:
        object_get_position(&base_position, unit_index); // redundant re-fetch in the original
        base_position.x += global_up3d_pointer->i * 0.4f;
        base_position.y += global_up3d_pointer->j * 0.4f;
        base_position.z += global_up3d_pointer->k * 0.4f;
        delta.i = global_down3d_pointer->i * 2.0f; // 0x571e89: EAX = [0x69672c], then [eax]
        delta.j = global_down3d_pointer->j * 2.0f;
        delta.k = global_down3d_pointer->k * 2.0f;

        // 0x571e50..0x571ec1: EAX = 1, ECX = &result, push bsp [0x746f98], 0, 0, &base, &delta, FLT_MAX (0x7f7fffff)
        {
            uint32_t flt_max_bits = 0x7f7fffff;
            hit = collision_bsp_query_segment_init(1, &segment_result, (ModelCollisionGeometryBSP *)global_structure_collision_bsp, 0,
                (uint32_t *)0, &base_position, &delta, *(float *)&flt_max_bits);
        }
        hit_fraction = segment_result.t;                              // [esp+0x20] = result +0x00
        hit_result = *(int32_t *)((uint8_t *)&segment_result + 0x8);  // [esp+0x28] = result +0x08, returned in EBP
        if (hit != 0) {
            out_position->x = delta.i * hit_fraction + base_position.x;
            out_position->y = delta.j * hit_fraction + base_position.y;
            out_position->z = delta.k * hit_fraction + base_position.z;
            return hit_result;
        }
        break;
    default:
        break;
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x571de0):

undefined4 FUN_00571de0(void)

{
  int iVar1;
  char cVar2;
  float *unaff_EBX;
  undefined4 uVar3;
  uint unaff_ESI;
  float local_430;
  float local_42c;
  float local_428;
  float local_424;
  float local_420;
  float local_41c;
  float local_418;
  undefined4 local_410;

  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  object_get_position();
  uVar3 = 0xffffffff;
  switch(*(undefined2 *)(iVar1 + 0x2f4)) {
  case 0:
  case 1:
  case 4:
  case 6:
    object_get_position();
    local_430 = *(float *)PTR_DAT_00696720 * 0.4 + local_430;
    local_42c = *(float *)(PTR_DAT_00696720 + 4) * 0.4 + local_42c;
    local_428 = *(float *)(PTR_DAT_00696720 + 8) * 0.4 + local_428;
    local_424 = *(float *)PTR_DAT_0069672c + *(float *)PTR_DAT_0069672c;
    local_420 = *(float *)(PTR_DAT_0069672c + 4) + *(float *)(PTR_DAT_0069672c + 4);
    local_41c = *(float *)(PTR_DAT_0069672c + 8) + *(float *)(PTR_DAT_0069672c + 8);
    cVar2 = FUN_00502060(DAT_00746f98,0,0,&local_430,&local_424,0x7f7fffff);
    if (cVar2 != '\0') {
      *unaff_EBX = local_424 * local_418 + local_430;
      unaff_EBX[1] = local_420 * local_418 + local_42c;
      unaff_EBX[2] = local_41c * local_418 + local_428;
      uVar3 = local_410;
    }
  }
  return uVar3;
}
#endif
