// vehicle_calculate_ground_contact_lean_alt  (Ghidra: FUN_00574460; renamed from the phase2
//   proposal)
// address 0x574460, size 798 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary; dispatched from
//   vehicle_calculate_mounted_controls_dispatch's "> 0" branch)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x574460..0x57477d) -- parallel in structure to vehicle_calculate_ground_contact_lean.c
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
extern real_vector3d *global_up3d_pointer; // 0x00696720
extern void object_physics_tick(uint32_t object_index, void *powered_states, void *contact_points,
    real_vector3d *extra_force, real_vector3d *extra_torque); // 0x507840
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd700, EAX, ECX, stack
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0
extern void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out); // 0x4cbc00
extern void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out); // 0x4cdb90
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);

// REWRITTEN from objdump. Physics tag +0x68 != 2: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise the same
//   hover solve as vehicle_calculate_ground_lean_controls without the lean state: desired basis from the facing
//   (+0x224) with world up minus its facing component, rotated by the side-slip angle; the force is
//   forward * (speed - v.forward) * mass * 0.05 plus up * |v.forward / max| * 0.0035651792 * mass * 1.05; the torque
//   is the per-tick (4/30) axis-angle to the desired basis less the angular velocity, times radius^2 * mass * 0.05.
//   The powered buffer (arg 1) gets the throttle at +0x18 / +0x78 with unit weights at +0x28 / +0x88. Force and
//   torque are scaled by the throttle and passed to object_physics_tick(unit, powered, contacts, &F, &T).
// blam-cc: stack -> unit_index, out_record (powered mass points), out_transform (contact points)
void vehicle_calculate_ground_contact_lean_alt(uint32_t unit_index, void *out_record, void *out_transform)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *physics = (uint8_t *)tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint8_t *powered = (uint8_t *)out_record;
    real max_speed = *(real *)(tag + 0x2f8);
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real throttle;
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *object_up = (real_vector3d *)(obj + 0x80);
    real_vector3d *angular_velocity = (real_vector3d *)(obj + 0x8c);
    real_vector3d facing, up, force, torque, axis;
    real_matrix4x3 current, desired, relative;
    real_quaternion rotation;
    real dot, x_force, y_force, angle, per_tick, torque_scale;

    if (*(int32_t *)(physics + 0x68) != 2) {
        object_physics_tick(unit_index, 0, out_transform, 0, 0);
        return;
    }

    facing = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
    up.i = -(facing.i * facing.k);
    up.j = -(facing.j * facing.k);
    up.k = 1.0f - facing.k * facing.k;
    if (vector3d_normalize_with_length(&up) == 0.0f) {
        up.i = 1.0f;
        up.j = 0.0f;
        up.k = 0.0f;
    }

    dot = velocity->i * forward->i + velocity->j * forward->j + velocity->k * forward->k;
    x_force = (speed - dot) * *(real *)(physics + 0x8) * 0.05f;
    y_force = (real)fabs((double)(dot / max_speed)) * 0.0035651792f * *(real *)(physics + 0x8) * 1.05f;
    force.i = object_up->i * y_force + forward->i * x_force;
    force.j = object_up->j * y_force + forward->j * x_force;
    force.k = forward->k * x_force + object_up->k * y_force;

    angle = (velocity->j * facing.i - facing.j * velocity->i) * 1.5707964f / (real)fabs((double)max_speed);
    vector3d_rotate_about_axis_perpendicular(&up, &facing, (real)sin((double)angle), (real)cos((double)angle));
    matrix4x3_from_forward_up(object_up, forward, &current);
    matrix4x3_from_forward_up(&up, &facing, &desired);
    matrix4x3_inverse(&desired, &desired);
    matrix4x3_multiply(&current, &desired, &relative);
    quaternion_from_matrix4x3(&relative, &rotation);
    quaternion_to_axis_angle(&rotation, &axis, &angle);

    per_tick = angle * 0.13333334f;
    torque_scale = *(real *)(physics + 0x0) * *(real *)(physics + 0x0) * *(real *)(physics + 0x8) * 0.05f;
    torque.i = (axis.i * per_tick - angular_velocity->i) * torque_scale;
    torque.j = (axis.j * per_tick - angular_velocity->j) * torque_scale;
    torque.k = (axis.k * per_tick - angular_velocity->k) * torque_scale;

    throttle = ((struct vehicle_object *)obj)->unit.unknown_338;
    *(real *)(powered + 0x18) = throttle;
    *(real *)(powered + 0x28) = 1.0f;
    *(real *)(powered + 0x1c) = 0.0f;
    *(real *)(powered + 0x20) = 0.0f;
    *(real *)(powered + 0x24) = 0.0f;
    *(real *)(powered + 0x78) = throttle;
    *(real *)(powered + 0x88) = 1.0f;
    *(real *)(powered + 0x7c) = 0.0f;
    *(real *)(powered + 0x80) = 0.0f;
    *(real *)(powered + 0x84) = 0.0f;

    force.i = throttle * force.i;
    force.j = throttle * force.j;
    force.k = throttle * force.k;
    torque.i = throttle * torque.i;
    torque.j = throttle * torque.j;
    torque.k = throttle * torque.k;
    object_physics_tick(unit_index, out_record, out_transform, &force, &torque);
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
