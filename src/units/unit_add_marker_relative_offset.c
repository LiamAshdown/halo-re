// unit_add_marker_relative_offset  (Ghidra: FUN_00569190)
// address 0x569190, size 232 bytes, name confidence 0.3, rewrite confidence 0.85
// REWRITTEN from objdump 0x569190..0x569277: where the unit's eyes would be if it stood at world_point.
//   - a free (no parent +0x11c, no +0x106 bit 2) biped asks unit_compute_marker_offset_position directly (ECX unit,
//     EDX direction = param_4, BX mode = param_2, ESI out, stack: world_point, offset = param_5);
//   - otherwise the reference is the unit's position, or for a biped seated in a vehicle the vehicle's predicted aim
//     target (0x571de0 on the parent, when it has one), and out = camera position (0x568f80) + world_point -
//     reference.
// blam-cc: stack -> unit_index, mode, world_point, direction, offset; EAX -> out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data; // 0x008603b0

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900, EAX, ECX
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI
extern void unit_compute_marker_offset_position(uint32_t object_index, real_vector3d *reference_direction,
    int16_t mode, real_point3d *out_position, float *param_1, float *param_2); // 0x55a170, ECX, EDX, BX, ESI, stack
extern int32_t unit_predict_aim_target_position(uint32_t unit_index, real_point3d *out_position); // 0x571de0, ESI, EBX

void unit_add_marker_relative_offset(uint32_t unit_index, uint32_t param_2, float *world_point,
                                     uint32_t param_4, uint32_t param_5, real_point3d *accumulator)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    datum_index parent_index = ((unit_object *)unit)->base.parent_object;
    real_point3d reference;
    int have_reference = 0;

    if (parent_index == k_datum_index_none && (unit[0x106] & 4) == 0) {
        if (((unit_object *)unit)->base.type == 0) {
            unit_compute_marker_offset_position(unit_index, (real_vector3d *)param_4, (int16_t)param_2, accumulator,
                world_point, (float *)param_5);
            return;
        }
    } else if (((unit_object *)unit)->base.type == 0 && parent_index != k_datum_index_none) {
        uint8_t *parent = (uint8_t *)((object_header *)object_data->data)[parent_index & 0xffff].data;

        if (*(int16_t *)(parent + 0xb4) == 1 && unit_predict_aim_target_position(parent_index, &reference) != -1) {
            have_reference = 1;
        }
    }
    if (!have_reference) {
        object_get_position(&reference, unit_index);
    }
    unit_get_camera_position(unit_index, accumulator);
    accumulator->x = (world_point[0] - reference.x) + accumulator->x;
    accumulator->y = (world_point[1] - reference.y) + accumulator->y;
    accumulator->z = (world_point[2] - reference.z) + accumulator->z;
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
