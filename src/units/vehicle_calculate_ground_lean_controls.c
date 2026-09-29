// vehicle_calculate_ground_lean_controls  (Ghidra: FUN_00573100; renamed from the phase2
//   proposal)
// address 0x573100, size 962 bytes
// name confidence: 0.45 (phase2 proposal at 0.45, matches functions.md summary; dispatched from
//   vehicle_update's case 3)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x573100..0x5734c1) -- parallel in structure to vehicle_calculate_ground_contact_lean.c
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

extern void object_physics_tick(uint32_t object_index, void *powered_states, void *contact_points,
    real_vector3d *extra_force, real_vector3d *extra_torque); // 0x507840
extern void vehicle_create_hover_thruster_effects(uint32_t unit_index); // 0x574900
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd700, EAX, ECX, stack
extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970, EAX, ECX, stack
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, EAX, ECX
extern void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x4cc0d0 via [0x696664]
extern void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out); // 0x4cbc00, ECX, stack
extern void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out); // 0x4cdb90, EAX, ESI, EDI
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);

// REWRITTEN from objdump. Raw object offsets: +0x68 velocity, +0x74 forward, +0x80 up, +0x8c angular velocity, +0x224
//   the desired facing, +0x338 the throttle scale, +0x4cc vehicle flags, +0x4d4 forward speed, +0x4ec lean,
//   +0x4f0 lean output. Vehicle tag +0x2f8 is the maximum forward speed; physics tag +0x00 and +0x08 are the
//   radius and mass.
//   Flag bit 1 (disabled) clears the contact buffer (physics +0x74 entries of 0x130) and only spawns the
//   thruster effects. Otherwise the lean eases toward k * (1 - f^2) * throttle (k = 0.25 / 1.0 / 0.75 by flags
//   bit 2 / bit 3) by at most 0.05 per tick, with f = clamp(speed, 0, max) / max. The desired basis is
//   forward = the desired facing and up = normalize(-fx*fz, -fy*fz, 1 - fz^2), rotated about the facing by the
//   side-slip angle. The torque is the axis-angle from the current basis to it (per tick) less the angular
//   velocity, times radius^2 * mass * 0.05; the force is forward * X + up * Y. Both are scaled by the throttle
//   and handed to object_physics_tick, then the thruster effects run. The draft called every helper without
//   arguments (crash at boot in the menu scene).
void vehicle_calculate_ground_lean_controls(uint32_t unit_index, uint8_t *out_transform)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *physics = (uint8_t *)tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint16_t flags = ((struct vehicle_object *)obj)->vehicle.flags;
    real max_speed = *(real *)(tag + 0x2f8);
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real throttle = ((struct vehicle_object *)obj)->unit.driver_seat_power;
    real clamped, f2, k, delta, lean_scale, dot, x_force, y_force, angle, per_tick, torque_scale;
    real_vector3d facing, up, force, torque;
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *object_up = (real_vector3d *)(obj + 0x80);
    real_vector3d *angular_velocity = (real_vector3d *)(obj + 0x8c);
    real_matrix4x3 current, desired, relative;
    real_quaternion rotation;
    real_vector3d axis;
    int32_t i;

    if (flags & 2) {
        int32_t bytes = *(int32_t *)(physics + 0x74) * 0x130;
        for (i = 0; i < bytes; i++) {
            out_transform[i] = 0;
        }
        vehicle_create_hover_thruster_effects(unit_index);
        return;
    }

    clamped = !(speed >= 0.0f) ? 0.0f : (speed <= max_speed ? speed : max_speed);
    f2 = (clamped / max_speed) * (clamped / max_speed);
    k = (flags & 4) ? 0.25f : ((flags & 8) ? 1.0f : 0.75f);
    delta = k * ((1.0f - f2) * throttle) - ((struct vehicle_object *)obj)->vehicle.ground_lean;
    if (!(delta >= -0.05f)) {
        delta = -0.05f;
    } else if (!(delta <= 0.05f)) {
        delta = 0.05f;
    }
    ((struct vehicle_object *)obj)->vehicle.ground_lean = delta + ((struct vehicle_object *)obj)->vehicle.ground_lean;
    lean_scale = f2 * throttle;
    ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction = lean_scale;

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
    x_force = (speed - dot) * lean_scale * *(real *)(physics + 0x8) * 0.05f;
    y_force = ((real)fabs((double)(dot / max_speed)) * 1.05f + ((struct vehicle_object *)obj)->vehicle.ground_lean * 1.3f) *
        *(real *)(physics + 0x8) * 0.0035651792f;
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

    per_tick = angle * 0.033333335f;
    torque_scale = *(real *)(physics + 0x0) * *(real *)(physics + 0x0) * *(real *)(physics + 0x8) * 0.05f;
    torque.i = (axis.i * per_tick - angular_velocity->i) * torque_scale;
    torque.j = (axis.j * per_tick - angular_velocity->j) * torque_scale;
    torque.k = (axis.k * per_tick - angular_velocity->k) * torque_scale;

    force.i = throttle * force.i;
    force.j = throttle * force.j;
    force.k = throttle * force.k;
    torque.i = throttle * torque.i;
    torque.j = throttle * torque.j;
    torque.k = throttle * torque.k;
    object_physics_tick(unit_index, 0, out_transform, &force, &torque);
    vehicle_create_hover_thruster_effects(unit_index);
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
