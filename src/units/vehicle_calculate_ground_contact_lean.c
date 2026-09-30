// vehicle_calculate_ground_contact_lean  (Ghidra: FUN_00573f60; renamed from the phase2
//   proposal)
// address 0x573f60, size 1279 bytes
// name confidence: 0.4 (phase2 proposal at 0.4, matches functions.md summary; dispatched from
//   vehicle_calculate_mounted_controls_dispatch's "not > 0" branch)
// rewrite confidence: 0.9 (REWRITTEN from objdump 0x573f60..0x57445e; VERIFIED 2026-09-30, see below). The matrix3x3 / quaternion
//   calls in the middle are bound (EAX/ECX/EDX/ESI/EDI and stack) in the extern prototypes and the frame offsets were traced
//   through the callee-not-popping sub esp,8 / push sequences; Physics-tag fields at +8/+0x50/+0x54/+0x58/+0x68 are
//   still raw offsets (not in the tag headers).
// evidence: types/units.h vehicle_data.forward_velocity (0x4d4), .ground_contact_fraction
//   (0x4f0); types/units.h unit_data.desired_facing_vector (0x224), .unknown_338 (0x338),
//   .driver_unit_index (0x324); types/objects.h object.velocity/forward/up/angular_velocity;
//   types/tags.h Vehicle.maximum_forward_speed/maximum_reverse_speed (0x2f8/0x2fc),
//   .speed_acceleration/.speed_deceleration (0x300/0x304), .maximum_left_turn (0x308),
//   .turn_rate (0x314), .fixed_gun_pitch (0x364, used as a fixed rotation angle here); callee
//   vector3d_delta_toward_gravity_biased_clamp_length (this batch, out of scope, a math helper) and object_physics_tick (established
//   5-argument shape elsewhere in this batch, though here it is called with an extra pointer
//   pair that does not fit that shape .
// register convention: stack parameters (unit_index, powered mass points, contact points).
//   // blam-cc: stack -> unit_index, out_record, out_transform
// VERIFIED against disassembly 0x573f60..0x57445e (2026-09-30): physics mode 2 gate, force/torque arithmetic and clamp constants
//   (0.2, 0.01, 0.05, 0.005, 1/pi, 1/3 checked in the image), the basis build, the lean easing and the object_physics_tick call.

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern real_vector3d *global_up3d_pointer;      // 0x00696720
extern real_vector3d *global_forward3d_pointer; // 0x00696718
extern uint8_t *global_identity_quaternion_pointer;               // 0x00696738, a pointer to 16 bytes copied into the powered entries
extern void object_physics_tick(uint32_t object_index, void *powered_states, void *contact_points,
    real_vector3d *extra_force, real_vector3d *extra_torque); // 0x507840
extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990, ECX
extern void vector3d_delta_toward_gravity_biased_clamp_length(real_point3d *origin, real_point3d *target,
    real_vector3d *out_delta, real max_length_aligned, real max_length_default); // 0x572a90, EAX, ECX, ESI, stack
extern void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out); // 0x4cc560, ECX, EDX, stack
extern void vector3d_rotate_pair_in_plane(real_vector3d *a, real_vector3d *b, real sin_angle, real cos_angle); // 0x4cd790, EAX, ECX, stack
extern void vector3d_rotate_about_axis_perpendicular(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd700, EAX, ECX, stack
extern void matrix3x3_transpose(real_matrix3x3 *out, real_matrix3x3 *in); // 0x4cc500, EAX, ECX
extern void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b); // 0x4cc5f0, EAX, EDX, stack
extern real_quaternion *quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out); // 0x4cc780, ECX, stack
extern void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out); // 0x4cdb90
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);

// REWRITTEN from objdump. Physics tag +0x68 != 2: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise:
//   force = mass * throttle * the gravity-biased clamped delta from the velocity toward forward * speed (clamp
//   lengths frac * tag +0x300 / +0x304, frac = speed / tag +0x2f8, or -speed / tag +0x2fc in reverse);
//   desired basis (forward A = the facing +0x224, up U = world up - A.z * A normalized, left = U x A), pitched
//   by tag +0x364 when no AI drives (the +0x324 rider or the vehicle has actor -1) and turned by the side slip
//   ((A.x v.y - A.y v.x) / max * tag +0x308); torque = throttle * mean inertia (physics +0x50..+0x58) *
//   (axis * -angle * tag +0x314 / pi - angular velocity) from the rotation current^T * desired; the lean
//   (+0x4f0) follows |angular velocity| / tag +0x314, rising by at most clamp((1-lean)^2 * 0.2, 0.01, 0.05) and
//   falling by at most max(lean^2 * 0.05, 0.005); the powered entries get the throttle (+0x18 / +0x78) and the
//   16 bytes at [0x696738] (+0x1c / +0x7c); then object_physics_tick(unit, powered, contacts, &F, &T).
// blam-cc: stack -> unit_index, out_record (powered mass points), out_transform (contact points)
void vehicle_calculate_ground_contact_lean(uint32_t unit_index, void *out_record, void *out_transform)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *physics = (uint8_t *)tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    uint8_t *powered = (uint8_t *)out_record;
    real speed = ((struct vehicle_object *)obj)->vehicle.forward_velocity;
    real mass = *(real *)(physics + 0x8);
    real throttle = ((struct vehicle_object *)obj)->unit.driver_seat_power;
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *object_up = (real_vector3d *)(obj + 0x80);
    real_vector3d *angular_velocity = (real_vector3d *)(obj + 0x8c);
    real_vector3d *world_up = global_up3d_pointer;
    real_point3d target_velocity;
    real_vector3d delta, force, torque, axis;
    real_vector3d basis[3]; // [esp+0x2c]: forward, left, up -- one real_matrix3x3
    real_matrix3x3 current;
    real_matrix3x3 relative;
    real_quaternion rotation;
    real frac, angle, k, moment, spin_rate, lean, step;
    uint8_t *rider;

    if (*(int32_t *)(physics + 0x68) != 2) {
        object_physics_tick(unit_index, 0, out_transform, 0, 0);
        return;
    }

    target_velocity.x = speed * forward->i;
    target_velocity.y = speed * forward->j;
    target_velocity.z = speed * forward->k;
    frac = speed > 0.0f ? speed / *(real *)(tag + 0x2f8) : -(speed / *(real *)(tag + 0x2fc));
    vector3d_delta_toward_gravity_biased_clamp_length((real_point3d *)velocity, &target_velocity, &delta,
        frac * *(real *)(tag + 0x300), frac * *(real *)(tag + 0x304));
    force.i = delta.i * mass * throttle;
    force.j = delta.j * mass * throttle;
    force.k = delta.k * mass * throttle;
    matrix3x3_from_forward_up(object_up, forward, &current);

    basis[0] = *(real_vector3d *)&((unit_object *)obj)->unit.desired_facing_vector.i;
    basis[2].i = -basis[0].k * basis[0].i + world_up->i;
    basis[2].j = -basis[0].k * basis[0].j + world_up->j;
    basis[2].k = -basis[0].k * basis[0].k + world_up->k;
    if (vector3d_normalize_with_length(&basis[2]) == 0.0f) {
        basis[2] = *global_forward3d_pointer;
    }
    rider = obj;
    if (((unit_object *)obj)->unit.driver_unit_index != (datum_index)0xffffffff) {
        rider = (uint8_t *)((object_header *)object_data->data)[((unit_object *)obj)->unit.driver_unit_index & 0xffff].data;
    }
    if (*(datum_index *)(rider + 0x1f4) == (datum_index)0xffffffff) {
        real pitch = *(real *)(tag + 0x364);
        vector3d_rotate_pair_in_plane(&basis[2], &basis[0], (real)sin((double)pitch), (real)cos((double)pitch));
    }
    angle = (basis[0].i * velocity->j - basis[0].j * velocity->i) / *(real *)(tag + 0x2f8) * *(real *)(tag + 0x308);
    vector3d_rotate_about_axis_perpendicular(&basis[2], &basis[0], (real)sin((double)angle), (real)cos((double)angle));
    basis[1].i = basis[2].j * basis[0].k - basis[2].k * basis[0].j;
    basis[1].j = basis[2].k * basis[0].i - basis[2].i * basis[0].k;
    basis[1].k = basis[0].j * basis[2].i - basis[2].j * basis[0].i;

    matrix3x3_transpose(&current, &current);
    matrix3x3_multiply(&relative, (real_matrix3x3 *)basis, &current);
    quaternion_from_matrix3x3(&relative, &rotation);
    quaternion_to_axis_angle(&rotation, &axis, &angle);

    k = -angle * *(real *)(tag + 0x314) * 0.31830987f;
    moment = (*(real *)(physics + 0x58) + *(real *)(physics + 0x54) + *(real *)(physics + 0x50)) * 0.33333334f;
    torque.i = (axis.i * k - angular_velocity->i) * moment * throttle;
    torque.j = (axis.j * k - angular_velocity->j) * moment * throttle;
    torque.k = (axis.k * k - angular_velocity->k) * moment * throttle;

    spin_rate = (real)sqrt((double)(angular_velocity->i * angular_velocity->i + angular_velocity->j * angular_velocity->j +
        angular_velocity->k * angular_velocity->k)) / *(real *)(tag + 0x314);
    lean = ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction;
    if (spin_rate > lean) {
        step = (1.0f - lean) * (1.0f - lean) * 0.2f;
        if (!(step >= 0.01f)) {
            step = 0.01f;
        } else if (!(step <= 0.05f)) {
            step = 0.05f;
        }
        if (!(spin_rate - lean > step)) {
            step = spin_rate - lean;
        }
    } else {
        step = lean * lean * 0.05f;
        if (!(step > 0.005f)) {
            step = 0.005f;
        }
        step = -step;
        if (spin_rate - lean > step) {
            step = spin_rate - lean;
        }
    }
    ((struct vehicle_object *)obj)->vehicle.ground_contact_fraction = step + lean;

    *(real *)(powered + 0x18) = throttle;
    memcpy(powered + 0x1c, global_identity_quaternion_pointer, 16);
    *(real *)(powered + 0x78) = throttle;
    memcpy(powered + 0x7c, global_identity_quaternion_pointer, 16);
    object_physics_tick(unit_index, out_record, out_transform, &force, &torque);
}

#if 0
Original Ghidra decompilation (0x573f60):

void FUN_00573f60(uint param_1,int param_2,undefined4 param_3)

{
  float fVar1;
  uint *puVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  undefined *puVar6;
  undefined4 uVar7;
  int iVar8;
  float10 fVar9;
  float10 fVar10;
  float local_9c;
  float local_94;
  float local_90;
  float local_8c;
  float local_84;
  float local_80;
  float local_7c;
  float local_78;
  float local_74;
  float local_70;
  float local_6c;
  float local_68;
  float local_64;
  float local_60;
  float local_5c;
  float local_58;
  float local_54;
  float local_50;
  float local_4c;
  undefined1 local_48 [72];

  iVar8 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(iVar8 + 8 + *(int *)(DAT_008603b0 + 0x34));
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = *(int *)((*(uint *)(iVar3 + 0x8c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(int *)(iVar4 + 0x68) != 2) {
    FUN_00507840(param_1,0,param_3,0,0);
    return;
  }
  local_8c = (float)puVar2[0x135];
  local_94 = local_8c * (float)puVar2[0x1d];
  local_90 = local_8c * (float)puVar2[0x1e];
  local_8c = local_8c * (float)puVar2[0x1f];
  if ((float)puVar2[0x135] <= 0.0) {
    fVar1 = -((float)puVar2[0x135] / *(float *)(iVar3 + 0x2fc));
  }
  else {
    fVar1 = (float)puVar2[0x135] / *(float *)(iVar3 + 0x2f8);
  }
  FUN_00572a90(fVar1 * *(float *)(iVar3 + 0x300),fVar1 * *(float *)(iVar3 + 0x304));
  fVar1 = *(float *)(iVar4 + 8);
  fVar5 = (float)puVar2[0xce];
  local_54 = local_60 * fVar1 * fVar5;
  local_50 = local_5c * fVar1 * fVar5;
  local_4c = local_58 * fVar1 * fVar5;
  matrix3x3_from_forward_up(local_48);
  local_84 = (float)puVar2[0x89];
  local_80 = (float)puVar2[0x8a];
  local_7c = (float)puVar2[0x8b];
  fVar1 = -local_7c;
  local_6c = local_84 * fVar1 + *(float *)PTR_DAT_00696720;
  local_68 = local_80 * fVar1 + *(float *)(PTR_DAT_00696720 + 4);
  local_64 = fVar1 * local_7c + *(float *)(PTR_DAT_00696720 + 8);
  fVar9 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 == fVar9) {
    local_6c = *(float *)PTR_DAT_00696718;
    local_68 = *(float *)(PTR_DAT_00696718 + 4);
    local_64 = *(float *)(PTR_DAT_00696718 + 8);
  }
  iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar8);
  if (*(uint *)(iVar8 + 0x324) != 0xffffffff) {
    iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar8 + 0x324) & 0xffff) * 0xc);
  }
  if (*(int *)(iVar8 + 500) == -1) {
    fVar9 = (float10)fcos((float10)*(float *)(iVar3 + 0x364));
    fVar10 = (float10)fsin((float10)*(float *)(iVar3 + 0x364));
    vector3d_rotate_pair_in_plane((float)fVar10,(float)fVar9);
  }
  fVar9 = (((float10)local_84 * (float10)(float)puVar2[0x1b] -
           (float10)local_80 * (float10)(float)puVar2[0x1a]) / (float10)*(float *)(iVar3 + 0x2f8)) *
          (float10)*(float *)(iVar3 + 0x308);
  fVar10 = (float10)fcos(fVar9);
  fVar9 = (float10)fsin(fVar9);
  vector3d_rotate_about_axis_perpendicular((float)fVar9,(float)fVar10);
  local_94 = local_68 * local_7c - local_64 * local_80;
  local_90 = local_64 * local_84 - local_6c * local_7c;
  local_8c = local_80 * local_6c - local_68 * local_84;
  local_78 = local_94;
  local_74 = local_90;
  local_70 = local_8c;
  uVar7 = matrix3x3_transpose();
  matrix3x3_multiply(uVar7);
  quaternion_from_matrix3x3(&local_94);
  quaternion_to_axis_angle();
  fVar1 = -fVar5 * *(float *)(iVar3 + 0x314) * 0.31830987;
  local_58 = local_58 * fVar1 - (float)puVar2[0x25];
  fVar5 = (*(float *)(iVar4 + 0x58) + *(float *)(iVar4 + 0x54) + *(float *)(iVar4 + 0x50)) *
          0.33333334;
  local_8c = (float)puVar2[0xce];
  local_94 = (local_60 * fVar1 - (float)puVar2[0x23]) * fVar5 * local_8c;
  local_90 = (local_5c * fVar1 - (float)puVar2[0x24]) * fVar5 * local_8c;
  local_8c = local_58 * fVar5 * local_8c;
  fVar1 = SQRT((float)puVar2[0x25] * (float)puVar2[0x25] +
               (float)puVar2[0x24] * (float)puVar2[0x24] + (float)puVar2[0x23] * (float)puVar2[0x23]
              ) / *(float *)(iVar3 + 0x314);
  if (fVar1 <= (float)puVar2[0x13c]) {
    local_9c = (float)puVar2[0x13c] * (float)puVar2[0x13c] * 0.05;
    if (local_9c <= 0.005) {
      local_9c = 0.005;
    }
    local_9c = -local_9c;
    fVar5 = fVar1 - (float)puVar2[0x13c];
    if (fVar1 - (float)puVar2[0x13c] <= local_9c) goto LAB_005743b1;
  }
  else {
    local_9c = (1.0 - (float)puVar2[0x13c]) * (1.0 - (float)puVar2[0x13c]) * 0.2;
    if (0.01 <= local_9c) {
      if (0.05 < local_9c) {
        local_9c = 0.05;
      }
    }
    else {
      local_9c = 0.01;
    }
    fVar5 = fVar1 - (float)puVar2[0x13c];
    if (local_9c < fVar1 - (float)puVar2[0x13c]) goto LAB_005743b1;
  }
  local_9c = fVar5;
LAB_005743b1:
  puVar2[0x13c] = (uint)(local_9c + (float)puVar2[0x13c]);
  *(uint *)(param_2 + 0x18) = puVar2[0xce];
  puVar6 = PTR_DAT_00696738;
  *(undefined4 *)(param_2 + 0x1c) = *(undefined4 *)PTR_DAT_00696738;
  *(undefined4 *)(param_2 + 0x20) = *(undefined4 *)(puVar6 + 4);
  *(undefined4 *)(param_2 + 0x24) = *(undefined4 *)(puVar6 + 8);
  *(undefined4 *)(param_2 + 0x28) = *(undefined4 *)(puVar6 + 0xc);
  *(uint *)(param_2 + 0x78) = puVar2[0xce];
  *(undefined4 *)(param_2 + 0x7c) = *(undefined4 *)puVar6;
  *(undefined4 *)(param_2 + 0x80) = *(undefined4 *)(puVar6 + 4);
  *(undefined4 *)(param_2 + 0x84) = *(undefined4 *)(puVar6 + 8);
  *(undefined4 *)(param_2 + 0x88) = *(undefined4 *)(puVar6 + 0xc);
  FUN_00507840(param_1,param_2,param_3,&local_54,&local_94);
  return;
}
#endif
