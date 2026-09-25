// unit_clamp_direction_to_aim_or_look_bounds  (Ghidra: FUN_005697a0)
// address 0x5697a0, size 451 bytes, name confidence 0.4, rewrite confidence 0.25
// functions.md: "Tests a world-space direction against the unit's aiming or looking angle
// limits (selected by a flag), clamps it into range, and transforms the corrected direction
// back through the unit's orientation."
// evidence: types/units.h unit_data.aiming_bounds_valid/looking_bounds_valid (0x2b6/0x2b7),
//   .aiming_bounds/.looking_bounds (0x2b8/0x2c8, four floats: -yaw,+yaw,-pitch,+pitch);
//   src/objects/object_get_orientation.c (EAX out_forward, ECX object_index, stack out_up);
//   src/units/unit_rotate_basis_about_axis.c and similar for the matrix4x3_transform_normal
//   (out EAX, normal EDX, matrix stack) convention already used elsewhere in this module.
// blam-cc: unaff_EDI -> unit_index, param_1 -> world_direction, param_2 -> use_aiming_bounds.
// UNSURE: the call `object_get_orientation(&local_38)` shows only one visible argument; this
//   rewrite treats it as the stack argument (out_up), with EAX (out_forward, implicit) writing
//   the other 3-float group, based on which slot of the eventual real_matrix4x3 each group
//   occupies (forward immediately after scale, up immediately before position). If that
//   assignment is backwards, the visible effect is a transposed/mirrored basis in the final
//   transform, not a change to whether this function accepts or clamps the direction.
// UNSURE: PTR_DAT_006966f8 (the matrix's position field) is a 12-byte constant vector this
//   module does not otherwise name; kept as a raw global.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;   // 0x008603b0
extern float global_zero_vector3d[3]; // 0x006966f8, UNSURE exact value/name

extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction
extern double fcos(double x);
extern double fsin(double x);
extern double sqrt(double x); // a single x87 FSQRT instruction in the original (Ghidra's SQRT())
extern void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up); // 0x4f6970
extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0

// FIXED (objdump 0x5697a0): the original reads exactly two stack arguments ([ebp+8] the direction,
//   [ebp+0xc] the flag) and writes the clamped direction back through the first (EAX = ECX = [ebp+8] at the
//   final matrix4x3_transform_normal call). The draft's third `out` parameter read the caller's stack junk.
// blam-cc: EDI -> unit_index, stack -> world_direction, use_aiming_bounds
uint8_t unit_clamp_direction_to_aim_or_look_bounds(uint32_t unit_index, real_vector3d *world_direction,
                                                   uint8_t use_aiming_bounds)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
    uint8_t clamped = 0;
    uint8_t valid;
    float *bounds;

    if (!use_aiming_bounds) {
        valid = unit->looking_bounds_valid;
        bounds = unit->looking_bounds;
    } else {
        valid = unit->aiming_bounds_valid;
        bounds = unit->aiming_bounds;
    }
    if (!valid) {
        return 0;
    }

    real_matrix4x3 m;
    m.scale = 1.0f;
    real_vector3d up;
    object_get_orientation(&m.forward, unit_index, &up);
    m.left.i = m.forward.j * up.k - m.forward.k * up.j;
    m.left.j = up.i * m.forward.k - m.forward.i * up.k;
    m.left.k = m.forward.i * up.j - m.forward.j * up.i;
    m.up = up;
    m.position.x = global_zero_vector3d[0];
    m.position.y = global_zero_vector3d[1];
    m.position.z = global_zero_vector3d[2];

    float dx = world_direction->i, dy = world_direction->j, dz = world_direction->k;
    float yaw_x = up.k * dx + m.forward.j * dz + m.forward.k * dy;
    float yaw_y = m.left.k * dz + m.left.j * dy + m.left.i * dx;
    float yaw = (float)atan2(yaw_y, yaw_x);
    float pitch = (float)atan2(m.up.i * dx + up.j * dz + up.k * dy,
                                 (double)sqrt((double)(yaw_x * yaw_x + yaw_y * yaw_y)));

    if (bounds[0] <= yaw) {
        if (yaw <= bounds[1]) {
            goto pitch_check;
        }
        yaw = bounds[1];
    } else {
        yaw = bounds[0];
    }
    clamped = 1;

pitch_check:
    if (bounds[2] <= pitch) {
        if (pitch <= bounds[3]) {
            if (!clamped) {
                return 0;
            }
        } else {
            clamped = 1;
            pitch = bounds[3];
        }
    } else {
        clamped = 1;
        pitch = bounds[2];
    }

    float cos_pitch = (float)fcos((double)pitch);
    float cos_yaw = (float)fcos((double)yaw);
    float sin_yaw = (float)fsin((double)yaw);
    float sin_pitch = (float)fsin((double)pitch);
    real_vector3d local_dir = { cos_yaw * cos_pitch, sin_yaw * cos_pitch, sin_pitch };
    matrix4x3_transform_normal(world_direction, &local_dir, &m); // in place
    return clamped;
}

#if 0
Original Ghidra decompilation (0x5697a0):

char FUN_005697a0(float *param_1,char param_2)

{
  float fVar1;
  float fVar2;
  int iVar3;
  char cVar4;
  char cVar5;
  float *pfVar6;
  uint unaff_EDI;
  float10 fVar7;
  float10 fVar8;
  float10 fVar9;
  float10 fVar10;
  undefined4 local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  float local_18;
  float local_14;
  float local_10;
  float local_c;

  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EDI & 0xffff) * 0xc);
  cVar5 = '\0';
  if (param_2 == '\0') {
    cVar4 = *(char *)(iVar3 + 0x2b7);
    pfVar6 = (float *)(iVar3 + 0x2c8);
  }
  else {
    cVar4 = *(char *)(iVar3 + 0x2b6);
    pfVar6 = (float *)(iVar3 + 0x2b8);
  }
  if (cVar4 == '\0') {
    return '\0';
  }
  local_54 = 0x3f800000;
  object_get_orientation(&local_38);
  local_44 = local_48 * local_34 - local_4c * local_30;
  local_40 = local_30 * local_50 - local_48 * local_38;
  local_2c = *(undefined4 *)PTR_DAT_006966f8;
  local_3c = local_4c * local_38 - local_34 * local_50;
  local_28 = *(undefined4 *)(PTR_DAT_006966f8 + 4);
  local_24 = *(undefined4 *)(PTR_DAT_006966f8 + 8);
  fVar7 = (float10)*param_1;
  fVar8 = (float10)param_1[1];
  fVar9 = (float10)param_1[2];
  fVar1 = (float)((float10)local_50 * fVar7 + (float10)local_48 * fVar9 + (float10)local_4c * fVar8)
  ;
  fVar2 = (float)((float10)local_3c * fVar9 + (float10)local_40 * fVar8 + (float10)local_44 * fVar7)
  ;
  fVar10 = (float10)fpatan((float10)fVar2,(float10)fVar1);
  local_c = (float)fVar10;
  fVar7 = (float10)fpatan((float10)local_38 * fVar7 +
                          (float10)local_30 * fVar9 + (float10)local_34 * fVar8,
                          SQRT((float10)fVar1 * (float10)fVar1 + (float10)fVar2 * (float10)fVar2));
  if (*pfVar6 <= local_c) {
    if (local_c <= pfVar6[1]) goto LAB_005698f7;
    local_c = pfVar6[1];
  }
  else {
    local_c = *pfVar6;
  }
  cVar5 = '\x01';
LAB_005698f7:
  if ((float10)pfVar6[2] <= fVar7) {
    if (fVar7 <= (float10)pfVar6[3]) {
      if (cVar5 == '\0') {
        return '\0';
      }
    }
    else {
      cVar5 = '\x01';
      fVar7 = (float10)pfVar6[3];
    }
  }
  else {
    cVar5 = '\x01';
    fVar7 = (float10)pfVar6[2];
  }
  fVar8 = (float10)fcos(fVar7);
  fVar9 = (float10)fcos((float10)local_c);
  local_18 = (float)(fVar9 * fVar8);
  fVar9 = (float10)fsin((float10)local_c);
  local_14 = (float)(fVar9 * fVar8);
  fVar7 = (float10)fsin(fVar7);
  local_10 = (float)fVar7;
  matrix4x3_transform_normal(&local_54);
  return cVar5;
}
#endif
