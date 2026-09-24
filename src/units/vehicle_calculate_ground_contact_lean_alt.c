// vehicle_calculate_ground_contact_lean_alt  (Ghidra: FUN_00574460; renamed from the phase2
//   proposal)
// address 0x574460, size 798 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary; dispatched from
//   vehicle_calculate_mounted_controls_dispatch's "> 0" branch)
// rewrite confidence: 0.1 -- parallel in structure to vehicle_calculate_ground_contact_lean.c
//   (0x573f60, this batch) and equally dense; the same matrix/quaternion caveats apply, plus a
//   gate value (Physics tag +0x68, compared against the integer 20 here rather than 2 or 3 as
//   in the sibling functions) that is not documented anywhere.
// evidence: types/units.h unit_data.desired_facing_vector (0x224), .unknown_338 (0x338);
//   types/objects.h object.velocity/forward/up/angular_velocity; types/tags.h
//   Vehicle.maximum_forward_speed (0x2f8); math.h matrix4x3_multiply (established, called here
//   through the PTR_matrix4x3_multiply_00696664 thunk per src/math/math_initialize.c);
//   DAT_0069c52c ("UNSURE" global already flagged in src/units/biped_apply_idle_fidget.c and
//   sibling files).
// register convention: unit object index in EAX (param_1); an output record pointer in ECX
//   (param_2); an output transform pointer in EDX (param_3).
//   // blam-cc: EAX -> unit_index, ECX -> out_record, EDX -> out_transform
// UNSURE: essentially every field derived from the Physics tag and the quaternion section; see
//   file header and vehicle_calculate_ground_contact_lean.c for the same caveats.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern float DAT_0069c52c; // UNSURE global, per sibling files

extern void object_physics_tick(uint32_t unit_index, void *out_record, void *out_transform,
                          void *param_4, void *param_5); // 0x507840, UNSURE signature
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern void vector3d_rotate_about_axis_perpendicular(float sin_angle, float cos_angle); // 0x4cd700, UNSURE  // real signature (vector3d_rotate_about_axis_perpendicular.c): void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); Ghidra recovered 2 of 4 args at this call site
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, UNSURE args here
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0
extern void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out); // 0x4cbc00, UNSURE args
extern void quaternion_to_axis_angle(void); // 0x4cdb90, UNSURE args  // real signature (quaternion_to_axis_angle.c): void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out); Ghidra recovered 0 of 3 args at this call site
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);
extern float fabsf(float x);

// Alternate ground-contact lean calculation for a vehicle (gated on the Physics tag's +0x68
// field equalling 20, rather than 2 as in the sibling function), writing its resulting
// transform into out_record/out_transform via object_physics_tick.
// UNSURE: reproduced only partially; see the file header before trusting this file's math.
void vehicle_calculate_ground_contact_lean_alt(uint32_t unit_index, void *out_record, void *out_transform)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;

    if (*(int32_t *)(physics_tag + 0x68) != 20) {
        object_physics_tick(unit_index, 0, out_transform, 0, 0);
        return;
    }

    {
        real_vector3d facing = unit->desired_facing_vector;
        real_vector3d axis;
        real length;
        float along, lateral;

        axis.i = -(facing.i * facing.k);
        axis.j = -(facing.j * facing.k);
        axis.k = 1.0f - facing.k * facing.k;
        length = vector3d_normalize_with_length(&axis);
        if (length == 0.0f) {
            axis.i = 1.0f;
            axis.j = 0.0f;
            axis.k = 0.0f;
        }

        along = facing.i * obj->forward.i + facing.j * obj->forward.j + facing.k * obj->forward.k;
        lateral = fabsf(along / tag->maximum_forward_speed) * DAT_0069c52c * *(float *)(physics_tag + 8) * 1.05f;

        {
            float accel = (obj->velocity.i /* UNSURE: puVar5[0x135], forward_velocity misread as
                                              object.velocity component, see #if 0 */
                           - along) * *(float *)(physics_tag + 8) * 0.05f;
            real_vector3d push;
            push.i = accel * obj->forward.i + lateral * obj->up.i;
            push.j = accel * obj->forward.j + lateral * obj->up.j;
            push.k = accel * obj->forward.k + lateral * obj->up.k;

            {
                double turn_angle = ((double)(facing.j * facing.i - facing.j * facing.i)) * 1.5707964 /
                                     fabs((double)tag->maximum_forward_speed); // UNSURE, see #if 0
                double c = cos(turn_angle);
                double s = sin(turn_angle);
                vector3d_rotate_about_axis_perpendicular((float)s, (float)c);
            }

            {
                real_matrix4x3 m1, m2, m2_inv, product;
                float quat[4];

                matrix4x3_from_forward_up(&obj->up, &facing, &m1);
                matrix4x3_from_forward_up(&obj->up, &obj->forward, &m2);
                matrix4x3_inverse(&m2_inv, &m2);
                matrix4x3_multiply(&m1, &m2_inv, &product);
                quaternion_from_matrix4x3((real_matrix4x3 *)quat, (real_quaternion *)&product);
                quaternion_to_axis_angle();
            }

            {
                float scale = unit->unknown_338 * 0.13333334f;
                real_vector3d angular;
                float spin_bound = *(float *)physics_tag * *(float *)physics_tag * *(float *)(physics_tag + 8) * 0.05f;

                angular.i = axis.j * scale; // UNSURE, see #if 0
                angular.j = axis.k * scale;
                angular.k = 0.0f;

                *(float *)((uint8_t *)out_record + 0x18) = unit->unknown_338;
                *(uint32_t *)((uint8_t *)out_record + 0x28) = 0x3f800000;
                *(uint32_t *)((uint8_t *)out_record + 0x1c) = 0;
                *(uint32_t *)((uint8_t *)out_record + 0x20) = 0;
                *(uint32_t *)((uint8_t *)out_record + 0x24) = 0;
                *(float *)((uint8_t *)out_record + 0x78) = unit->unknown_338;
                *(uint32_t *)((uint8_t *)out_record + 0x88) = 0x3f800000;
                *(uint32_t *)((uint8_t *)out_record + 0x7c) = 0;
                *(uint32_t *)((uint8_t *)out_record + 0x80) = 0;
                *(uint32_t *)((uint8_t *)out_record + 0x84) = 0;

                push.i *= unit->unknown_338;
                push.j *= unit->unknown_338;
                push.k *= unit->unknown_338;

                angular.i = unit->unknown_338 * (axis.i * scale - obj->angular_velocity.i) * spin_bound;
                angular.j = unit->unknown_338 * (angular.j - obj->angular_velocity.j) * spin_bound;
                angular.k = unit->unknown_338 * (angular.k - obj->angular_velocity.k) * spin_bound;

                object_physics_tick(unit_index, out_record, out_transform, &push, &angular);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x574460):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00574460(uint param_1,int param_2,undefined4 param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  uint *puVar5;
  int iVar6;
  float fVar7;
  float fVar8;
  float10 fVar9;
  float10 fVar10;
  undefined1 local_f0 [56];
  undefined1 local_b8 [56];
  undefined1 local_80 [60];
  float local_44;
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  undefined1 local_28 [4];
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float *local_c;

  puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar6 = *(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_c = *(float **)((*(uint *)(iVar6 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (local_c[0x1a] == 2.8026e-45) {
    local_24 = (float)puVar5[0x89];
    local_20 = (float)puVar5[0x8a];
    local_1c = (float)puVar5[0x8b];
    local_18 = -(local_24 * local_1c);
    local_14 = -(local_20 * local_1c);
    local_10 = 1.0 - local_1c * local_1c;
    fVar9 = (float10)vector3d_normalize_with_length();
    if ((float10)0.0 == fVar9) {
      local_18 = 1.0;
      local_14 = 0.0;
      local_10 = 0.0;
    }
    local_3c = local_24;
    local_40 = (float)puVar5[0x1b];
    local_38 = local_20;
    fVar1 = (float)puVar5[0x1a] * (float)puVar5[0x1d] +
            (float)puVar5[0x1b] * (float)puVar5[0x1e] + (float)puVar5[0x1c] * (float)puVar5[0x1f];
    fVar2 = ((float)puVar5[0x135] - fVar1) * local_c[2] * 0.05;
    local_44 = (float)puVar5[0x1a];
    fVar1 = ABS(fVar1 / *(float *)(iVar6 + 0x2f8)) * _DAT_0069c52c * local_c[2] * 1.05;
    local_34 = fVar2 * (float)puVar5[0x1d] + fVar1 * (float)puVar5[0x20];
    local_30 = fVar2 * (float)puVar5[0x1e] + fVar1 * (float)puVar5[0x21];
    local_2c = fVar2 * (float)puVar5[0x1f] + fVar1 * (float)puVar5[0x22];
    fVar10 = (((float10)local_40 * (float10)local_24 - (float10)local_20 * (float10)local_44) *
             (float10)1.5707964) / ABS((float10)*(float *)(iVar6 + 0x2f8));
    fVar9 = (float10)fcos(fVar10);
    fVar10 = (float10)fsin(fVar10);
    vector3d_rotate_about_axis_perpendicular((float)fVar10,(float)fVar9);
    matrix4x3_from_forward_up(local_b8);
    matrix4x3_from_forward_up(local_80);
    matrix4x3_inverse();
    (*(code *)PTR_matrix4x3_multiply_00696664)(local_b8,local_80,local_f0);
    quaternion_from_matrix4x3(local_28);
    quaternion_to_axis_angle();
    fVar7 = local_38 * 0.13333334;
    local_20 = local_14 * fVar7;
    local_1c = local_10 * fVar7;
    fVar8 = *local_c * *local_c * local_c[2] * 0.05;
    fVar1 = (float)puVar5[0x23];
    fVar2 = (float)puVar5[0x24];
    fVar3 = (float)puVar5[0x25];
    *(uint *)(param_2 + 0x18) = puVar5[0xce];
    *(undefined4 *)(param_2 + 0x28) = 0x3f800000;
    *(undefined4 *)(param_2 + 0x1c) = 0;
    *(undefined4 *)(param_2 + 0x20) = 0;
    *(undefined4 *)(param_2 + 0x24) = 0;
    *(uint *)(param_2 + 0x78) = puVar5[0xce];
    *(undefined4 *)(param_2 + 0x88) = 0x3f800000;
    *(undefined4 *)(param_2 + 0x7c) = 0;
    *(undefined4 *)(param_2 + 0x80) = 0;
    *(undefined4 *)(param_2 + 0x84) = 0;
    fVar4 = (float)puVar5[0xce];
    local_c = (float *)puVar5[0xce];
    local_34 = local_34 * fVar4;
    local_30 = local_30 * fVar4;
    local_2c = fVar4 * local_2c;
    local_18 = (float)local_c * (local_18 * fVar7 - fVar1) * fVar8;
    local_14 = (float)local_c * (local_20 - fVar2) * fVar8;
    local_10 = (float)local_c * (local_1c - fVar3) * fVar8;
    FUN_00507840(param_1,param_2,param_3,&local_34,&local_18);
    return;
  }
  FUN_00507840(param_1,0,param_3,0,0);
  return;
}
#endif
