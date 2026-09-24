// unit_add_marker_relative_offset  (Ghidra: FUN_00569190)
// address 0x569190, size 232 bytes, name confidence 0.3, rewrite confidence 0.25
// functions.md: "Computes the offset between a given world point and the unit's camera
// position, accumulating it into an output vector."
// evidence: types/objects.h object.parent_object (0x11c), .vitality_flags (0x106), .type (0xb4).
// blam-cc: param_1 -> unit_index, param_2/param_4 unused here, param_3 -> world_point,
//   param_5 -> forwarded to unit_compute_marker_offset_position, unaff_EAX -> accumulator (in/out).
// UNSURE: param_2 and param_4 are never read in this function's own body; kept as unused
//   parameters rather than dropped, since the callee's signature (as Ghidra sees it) has them.
// UNSURE: unit_predict_aim_target_position's return value is used as a marker-transform pointer/handle passed
//   implicitly to whatever fills local_c/local_8/local_4 in the original; not fully recovered.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80
extern void unit_compute_marker_offset_position(float *world_point, uint32_t param_5); // 0x55a170, UNSURE signature  // real signature (unit_compute_marker_offset_position.c): void unit_compute_marker_offset_position(uint32_t object_index, real_vector3d *reference_direction, int16_t mode, real_point3d *out_position, float *param_1, float *param_2); Ghidra recovered 2 of 6 args at this call site
extern int32_t unit_predict_aim_target_position(void); // 0x571de0, UNSURE signature  // real signature (unit_predict_aim_target_position.c): int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position); Ghidra recovered 0 of 2 args at this call site

void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t param_2, float *world_point,
                                     uint32_t param_4, uint32_t param_5, real_point3d *accumulator) // blam-cc: unaff_EAX -> accumulator
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    real_point3d reference;

    if ((unit_obj->parent_object == k_datum_index_none) && ((unit_obj->vitality_flags & _object_health_frozen_bit) == 0)) {
        if (unit_obj->type == _object_type_biped) {
            unit_compute_marker_offset_position(world_point, param_5);
            return;
        }
    } else if ((unit_obj->type == _object_type_biped) && (unit_obj->parent_object != k_datum_index_none) &&
               (((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data->type == _object_type_vehicle)) {
        if (unit_predict_aim_target_position() != -1) {
            goto have_reference; // UNSURE: original leaves `reference` populated by unit_predict_aim_target_position's own side effects
        }
    }
    object_get_position(&reference, unit_index);

have_reference:
    unit_get_camera_position(unit_index, accumulator);
    accumulator->x = (world_point[0] - reference.x) + accumulator->x;
    accumulator->y = (world_point[1] - reference.y) + accumulator->y;
    accumulator->z = (world_point[2] - reference.z) + accumulator->z;
    return;
}

#if 0
Original Ghidra decompilation (0x569190):

void FUN_00569190(uint param_1,undefined4 param_2,float *param_3,undefined4 param_4,
                 undefined4 param_5)

{
  float fVar1;
  float fVar2;
  uint uVar3;
  float *in_EAX;
  int iVar4;
  float local_c;
  float local_8;
  float local_4;

  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  uVar3 = *(uint *)(iVar4 + 0x11c);
  if ((uVar3 == 0xffffffff) && ((*(byte *)(iVar4 + 0x106) & 4) == 0)) {
    if (*(short *)(iVar4 + 0xb4) == 0) {
      FUN_0055a170(param_3,param_5);
      return;
    }
  }
  else if ((*(short *)(iVar4 + 0xb4) == 0) &&
          ((uVar3 != 0xffffffff &&
           (*(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc) + 0xb4)
            == 1)))) {
    iVar4 = FUN_00571de0();
    if (iVar4 != -1) goto LAB_00569237;
  }
  object_get_position();
LAB_00569237:
  unit_get_camera_position();
  fVar1 = param_3[1];
  fVar2 = param_3[2];
  *in_EAX = (*param_3 - local_c) + *in_EAX;
  in_EAX[1] = (fVar1 - local_8) + in_EAX[1];
  in_EAX[2] = (fVar2 - local_4) + in_EAX[2];
  return;
}
#endif
