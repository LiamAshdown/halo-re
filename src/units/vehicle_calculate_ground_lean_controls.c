// vehicle_calculate_ground_lean_controls  (Ghidra: FUN_00573100; renamed from the phase2
//   proposal)
// address 0x573100, size 962 bytes
// name confidence: 0.45 (phase2 proposal at 0.45, matches functions.md summary; dispatched from
//   vehicle_update's case 3)
// rewrite confidence: 0.1 -- parallel in structure to vehicle_calculate_ground_contact_lean.c
//   and vehicle_calculate_ground_contact_lean_alt.c (this batch); the same matrix/quaternion
//   caveats apply. The leading branch (vehicle_data.flags bit 2, undocumented in
//   types/units.h) zeroes the whole per-contact-point output buffer and returns early.
// evidence: types/units.h vehicle_data.flags (0x4cc), .forward_velocity (0x4d4), .ground_lean
//   (0x4ec), .ground_contact_fraction (0x4f0); types/units.h unit_data.desired_facing_vector
//   (0x224), .unknown_338 (0x338), .unknown_2ec... angular_velocity (0x23-25 index);
//   types/objects.h object.velocity/forward/up/angular_velocity; types/tags.h
//   Vehicle.maximum_forward_speed (0x2f8); the physics.tag_id-at-0x8c idiom (contact-point
//   count at Physics+0x74, matching the sibling functions); callees
//   vehicle_create_hover_thruster_effects (0x574900, this batch), object_physics_tick (established
//   5-argument form used with a zeroed second argument here).
// register convention: unit object index in EAX (param_1); an output buffer pointer in the
//   stack parameter (param_2).
//   // blam-cc: EAX -> unit_index, stack -> out_transform
// UNSURE: essentially every field derived from the Physics tag and the quaternion section; see
//   the sibling ground-contact-lean files' headers for the same caveats.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern float DAT_0069c52c; // UNSURE global, per sibling files

extern void object_physics_tick(uint32_t unit_index, uint32_t param_2, void *out_transform,
                          void *param_4, void *param_5); // 0x507840
extern void vehicle_create_hover_thruster_effects(uint32_t unit_index); // 0x574900, this batch
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

// Computes ground-hugging lean/roll for a hovering vehicle each tick and triggers its hover/jet
// thruster particle effects.
// UNSURE: reproduced only partially; see the file header before trusting this file's math.
void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Vehicle *tag = (Vehicle *)tag_instances[obj->definition_tag & 0xffff].data;
    vehicle_data *vehicle = (vehicle_data *)((uint8_t *)obj + k_unit_object_size);
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    uint8_t *physics_tag = tag_instances[*(uint32_t *)((uint8_t *)tag + 0x8c) & 0xffff].data;
    uint32_t flags = vehicle->flags;

    if ((flags & 2) != 0) {
        int32_t count = *(int32_t *)(physics_tag + 0x74);
        int32_t bytes = count * 0x130;
        int32_t i;
        for (i = 0; i < bytes; i++) {
            out_transform[i] = 0;
        }
        vehicle_create_hover_thruster_effects(unit_index);
        return;
    }

    {
        float speed_fraction;
        float lean_target;
        float delta;
        real_vector3d facing = unit->desired_facing_vector;
        real_vector3d axis;
        real length;
        float along;
        float scaled;
        real_vector3d push;

        speed_fraction = (vehicle->forward_velocity >= 0.0f)
            ? ((vehicle->forward_velocity <= tag->maximum_forward_speed) ? vehicle->forward_velocity : tag->maximum_forward_speed)
            : 0.0f;
        speed_fraction = speed_fraction / tag->maximum_forward_speed;
        speed_fraction = speed_fraction * speed_fraction;

        if ((flags & 4) != 0) {
            scaled = 0.25f;
        } else if ((flags & 8) == 0) {
            scaled = 0.75f;
        } else {
            scaled = 1.0f;
        }

        delta = (1.0f - speed_fraction) * unit->unknown_338 * scaled - vehicle->ground_lean;
        if (delta < -0.05f) delta = -0.05f;
        else if (delta > 0.05f) delta = 0.05f;
        vehicle->ground_lean += delta;

        vehicle->ground_contact_fraction = speed_fraction * unit->unknown_338;

        axis.i = -(facing.i * facing.k);
        axis.j = -(facing.j * facing.k);
        axis.k = 1.0f - facing.k * facing.k;
        length = vector3d_normalize_with_length(&axis);
        if (length == 0.0f) {
            axis.i = 1.0f;
            axis.j = 0.0f;
            axis.k = 0.0f;
        }

        along = facing.i * obj->velocity.i + facing.j * obj->velocity.j + facing.k * obj->velocity.k;
        {
            float accel = (vehicle->forward_velocity - along) * vehicle->ground_contact_fraction *
                          *(float *)(physics_tag + 8) * 0.05f;
            float lateral = ((vehicle->ground_lean * 1.3f) + fabsf(along / tag->maximum_forward_speed) * 1.05f) *
                            *(float *)(physics_tag + 8) * DAT_0069c52c;
            push.i = accel * facing.i + lateral * obj->up.i;
            push.j = accel * facing.j + lateral * obj->up.j;
            push.k = accel * facing.k + lateral * obj->up.k;
        }

        {
            double turn_angle = (((double)obj->velocity.j * facing.i - (double)facing.j * obj->velocity.i) *
                                  1.5707964) / fabs((double)tag->maximum_forward_speed);
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
            float ang_scale = vehicle->ground_contact_fraction * 0.033333335f;
            float spin_bound = *(float *)physics_tag * *(float *)physics_tag * *(float *)(physics_tag + 8) * 0.05f;
            real_vector3d angular;

            push.i *= unit->unknown_338;
            push.j *= unit->unknown_338;
            push.k *= unit->unknown_338;

            angular.i = unit->unknown_338 * (axis.i * ang_scale - obj->angular_velocity.i) * spin_bound;
            angular.j = unit->unknown_338 * (axis.j * ang_scale - obj->angular_velocity.j) * spin_bound;
            angular.k = unit->unknown_338 * (axis.k * ang_scale - obj->angular_velocity.k) * spin_bound;

            object_physics_tick(unit_index, 0, out_transform, &push, &angular);
            vehicle_create_hover_thruster_effects(unit_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x573100):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00573100(uint param_1,undefined4 *param_2)

{
  float fVar1;
  uint *puVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  uint uVar6;
  int iVar7;
  float10 fVar8;
  float10 fVar9;
  undefined1 local_ec [56];
  undefined1 local_b4 [56];
  undefined1 local_7c [60];
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  undefined1 local_24 [4];
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;
  float local_c;
  float *local_8;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar7 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  uVar6 = puVar2[0x133];
  local_8 = *(float **)((*(uint *)(iVar7 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((uVar6 & 2) != 0) {
    for (uVar6 = (uint)((int)local_8[0x1d] * 0x130) >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
      *param_2 = 0;
      param_2 = param_2 + 1;
    }
    for (iVar7 = 0; iVar7 != 0; iVar7 = iVar7 + -1) {
      *(undefined1 *)param_2 = 0;
      param_2 = (undefined4 *)((int)param_2 + 1);
    }
    vehicle_create_hover_thruster_effects(param_1);
    return;
  }
  if (0.0 <= (float)puVar2[0x135]) {
    if ((float)puVar2[0x135] <= *(float *)(iVar7 + 0x2f8)) {
      local_28 = (float)puVar2[0x135];
    }
    else {
      local_28 = *(float *)(iVar7 + 0x2f8);
    }
  }
  else {
    local_28 = 0.0;
  }
  local_28 = local_28 / *(float *)(iVar7 + 0x2f8);
  local_28 = local_28 * local_28;
  if ((uVar6 & 4) == 0) {
    if ((uVar6 & 8) == 0) {
      fVar1 = 0.75;
    }
    else {
      fVar1 = 1.0;
    }
  }
  else {
    fVar1 = 0.25;
  }
  fVar1 = (1.0 - local_28) * (float)puVar2[0xce] * fVar1 - (float)puVar2[0x13b];
  if (-0.05 <= fVar1) {
    if (0.05 < fVar1) {
      fVar1 = 0.05;
    }
  }
  else {
    fVar1 = -0.05;
  }
  puVar2[0x13b] = (uint)(fVar1 + (float)puVar2[0x13b]);
  local_20 = (float)puVar2[0x89];
  local_1c = (float)puVar2[0x8a];
  local_28 = local_28 * (float)puVar2[0xce];
  local_18 = (float)puVar2[0x8b];
  puVar2[0x13c] = (uint)local_28;
  local_14 = -(local_20 * local_18);
  local_10 = -(local_1c * local_18);
  local_c = 1.0 - local_18 * local_18;
  fVar8 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar8) {
    local_14 = 1.0;
    local_10 = 0.0;
    local_c = 0.0;
  }
  local_2c = local_20;
  local_3c = (float)puVar2[0x1b];
  fVar1 = (float)puVar2[0x1d] * (float)puVar2[0x1a] +
          (float)puVar2[0x1b] * (float)puVar2[0x1e] + (float)puVar2[0x1c] * (float)puVar2[0x1f];
  fVar3 = ((float)puVar2[0x135] - fVar1) * local_28;
  local_28 = local_1c;
  fVar3 = fVar3 * local_8[2] * 0.05;
  local_40 = (float)puVar2[0x1a];
  fVar1 = ((float)puVar2[0x13b] * 1.3 + ABS(fVar1 / *(float *)(iVar7 + 0x2f8)) * 1.05) * local_8[2]
          * _DAT_0069c52c;
  local_38 = fVar3 * (float)puVar2[0x1d] + fVar1 * (float)puVar2[0x20];
  local_34 = fVar3 * (float)puVar2[0x1e] + fVar1 * (float)puVar2[0x21];
  local_30 = fVar3 * (float)puVar2[0x1f] + fVar1 * (float)puVar2[0x22];
  fVar9 = (((float10)local_3c * (float10)local_20 - (float10)local_1c * (float10)local_40) *
          (float10)1.5707964) / ABS((float10)*(float *)(iVar7 + 0x2f8));
  fVar8 = (float10)fcos(fVar9);
  fVar9 = (float10)fsin(fVar9);
  vector3d_rotate_about_axis_perpendicular((float)fVar9,(float)fVar8);
  matrix4x3_from_forward_up(local_b4);
  matrix4x3_from_forward_up(local_7c);
  matrix4x3_inverse();
  (*(code *)PTR_matrix4x3_multiply_00696664)(local_b4,local_7c,local_ec);
  quaternion_from_matrix4x3(local_24);
  quaternion_to_axis_angle();
  fVar4 = local_28 * 0.033333335;
  fVar3 = (float)puVar2[0xce];
  local_1c = local_10 * fVar4;
  local_18 = local_c * fVar4;
  fVar5 = *local_8 * *local_8 * local_8[2] * 0.05;
  fVar1 = (float)puVar2[0xce];
  local_38 = local_38 * fVar1;
  local_34 = local_34 * fVar1;
  local_30 = fVar1 * local_30;
  local_14 = fVar3 * (local_14 * fVar4 - (float)puVar2[0x23]) * fVar5;
  local_10 = fVar3 * (local_1c - (float)puVar2[0x24]) * fVar5;
  local_c = fVar3 * (local_18 - (float)puVar2[0x25]) * fVar5;
  local_8 = (float *)fVar3;
  FUN_00507840(param_1,0,param_2,&local_38,&local_14);
  vehicle_create_hover_thruster_effects(param_1);
  return;
}
#endif
