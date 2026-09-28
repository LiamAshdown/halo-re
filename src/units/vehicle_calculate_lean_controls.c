// vehicle_calculate_lean_controls  (Ghidra: FUN_00572df0; renamed from the phase2 proposal)
// address 0x572df0, size 779 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary; dispatched from
//   vehicle_update's case 2)
// rewrite confidence: 0.85 (REWRITTEN from objdump 0x572df0..0x5730fa) -- the output record (unaff_ESI) is entirely register-resident with
//   no traceable origin, so its dozen-plus field writes are reproduced at their literal byte
//   offsets rather than through a named struct.
// evidence: types/units.h vehicle_data.turning_velocity (0x4dc); types/objects.h object.velocity
//   (0x068, "puVar2[0x1a..0x1c]"); the physics.tag_id-at-0x8c double-tag_instances-lookup idiom
//   (Vehicle tag -> Physics tag -> a field at Physics+0x68 compared against 3) matches
//   vehicle_calculate_turret_controls.c and vehicle_calculate_steering_wheel_controls.c (this
//   batch), there compared against 2.
// register convention: unit object index in EAX (param_1); an output record pointer in ESI
//   (unaff_ESI); a second stack parameter (param_2) forwarded to object_physics_tick unexamined.
//   // blam-cc: EAX -> unit_index, ESI -> out_record, stack -> param_2
// UNSURE: essentially every write below +0. vector3d_angle_between_4cd4f0's return value is
//   never consumed by anything visible, and vector3d_cross_product's two calls here (like the
//   ones in vehicle_update) may be building values this decompile shows going unused.

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
extern void vector3d_cross_product(real_vector3d *out, const real_vector3d *a, const real_vector3d *b); // 0x4052c0, EAX, ECX, stack
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820, EAX, ECX, stack
extern real vector3d_angle_between_4cd4f0(real_vector3d *a, real_vector3d *b); // 0x4cd4f0, ECX, EDX
extern double sqrt(double x);
extern double sin(double x);
extern double cos(double x);
extern double fabs(double x);

// REWRITTEN from objdump. Physics tag +0x68 != 3: object_physics_tick(unit, 0, contacts, 0, 0). Otherwise the ESI
//   powered-mass-point buffer (vehicle_update [esp+0x88]) gets the drive: +0x04 forward speed, +0x0c/+0x6c 0.003,
//   +0x24/+0x28 sin/cos of turn * 0.5 * (1 - min(|speed| * 2.5, 1)), +0x88/+0xe8 1.0, +0xcc 0.005, the rest 0.
//   The roll torque about the forward axis comes from the angle between the unit's up and the target up
//   (world up minus its forward component, rotated about forward by 2pi * ((velocity x forward) . world up)),
//   signed by the side of (forward x up), sqrt-shaped, minus the current forward spin, clamped to +-0.01396 and
//   scaled by physics +0x50. object_physics_tick(unit, ESI, contacts, &zero, &torque) then runs. The draft
//   called every helper without arguments and had no buffer.
// blam-cc: stack -> unit_index, param_2 (contact points); ESI -> powered_states
void vehicle_calculate_lean_controls(uint32_t unit_index, void *mass_points, float *powered_states)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    uint8_t *tag = (uint8_t *)tag_instances[*(datum_index *)obj & 0xffff].data;
    uint8_t *physics = (uint8_t *)tag_instances[*(datum_index *)&((Unit *)tag)->base.physics.tag_id & 0xffff].data;
    real_vector3d *velocity = (real_vector3d *)(obj + 0x68);
    real_vector3d *forward = (real_vector3d *)(obj + 0x74);
    real_vector3d *up = (real_vector3d *)(obj + 0x80);
    real_vector3d *angular_velocity = (real_vector3d *)(obj + 0x8c);
    real_vector3d *world_up = global_up3d_pointer;
    uint8_t *ps = (uint8_t *)powered_states;
    real_vector3d zero_force;
    real_vector3d torque;
    real speed_factor, half_turn, steer;

    if (*(int32_t *)(physics + 0x68) != 3) {
        object_physics_tick(unit_index, 0, mass_points, 0, 0);
        return;
    }

    speed_factor = (real)fabs((double)((real)sqrt((double)(velocity->i * velocity->i + velocity->j * velocity->j +
        velocity->k * velocity->k)) * 2.5f));
    half_turn = *(real *)(obj + 0x4dc) * 0.5f;
    if (!(speed_factor <= 1.0f)) {
        speed_factor = 1.0f;
    }
    steer = (1.0f - speed_factor) * half_turn;
    *(real *)(ps + 0x04) = *(real *)(obj + 0x4d4);
    *(uint32_t *)(ps + 0x0c) = 0x3b449ba6; // 0.003
    *(real *)(ps + 0x1c) = 0.0f;
    *(real *)(ps + 0x20) = 0.0f;
    *(real *)(ps + 0x24) = (real)sin((double)steer);
    *(real *)(ps + 0x28) = (real)cos((double)steer);
    *(uint32_t *)(ps + 0x6c) = 0x3b449ba6;
    *(real *)(ps + 0x7c) = 0.0f;
    *(real *)(ps + 0x80) = 0.0f;
    *(real *)(ps + 0x84) = 0.0f;
    *(real *)(ps + 0x88) = 1.0f;
    *(uint32_t *)(ps + 0xcc) = 0x3ba3d70a; // 0.005
    *(real *)(ps + 0xe8) = 1.0f;
    *(real *)(ps + 0xdc) = 0.0f;
    *(real *)(ps + 0xe0) = 0.0f;
    *(real *)(ps + 0xe4) = 0.0f;
    zero_force.i = 0.0f;
    zero_force.j = 0.0f;
    zero_force.k = 0.0f;

    torque.i = -forward->k * forward->i + world_up->i;
    torque.j = -forward->k * forward->j + world_up->j;
    torque.k = -forward->k * forward->k + world_up->k;
    if (vector3d_normalize_with_length(&torque) == 0.0f) {
        torque.i = 0.0f;
        torque.j = 0.0f;
        torque.k = 0.0f;
    } else {
        real_vector3d side;
        real_vector3d slip;
        real angle, spin, w;
        int32_t sign;

        vector3d_cross_product(&side, forward, up);
        vector3d_cross_product(&slip, velocity, forward);
        angle = (slip.i * world_up->i + slip.j * world_up->j + slip.k * world_up->k) * 6.2831855f;
        vector3d_rotate_about_axis(&torque, forward, (real)sin((double)angle), (real)cos((double)angle));
        angle = vector3d_angle_between_4cd4f0(up, &torque);
        if (side.k * torque.k + side.j * torque.j + side.i * torque.i > 0.0f) {
            angle = -angle;
        }
        spin = forward->k * angular_velocity->k + forward->j * angular_velocity->j + forward->i * angular_velocity->i;
        sign = (angle == 0.0f) ? 0 : (angle >= 0.0f ? 1 : -1);
        w = (real)sqrt(fabs((double)angle) * 0.027925269678235054) * (real)sign - spin;
        if (!(w >= -0.013962635f)) {
            w = -0.013962635f;
        } else if (!(w <= 0.013962635f)) {
            w = 0.013962635f;
        }
        w = w * *(real *)(physics + 0x50);
        torque.i = w * forward->i;
        torque.j = w * forward->j;
        torque.k = w * forward->k;
    }
    object_physics_tick(unit_index, powered_states, mass_points, &zero_force, &torque);
}

#if 0
Original Ghidra decompilation (0x572df0):

void FUN_00572df0(uint param_1,undefined4 param_2)

{
  float fVar1;
  uint *puVar2;
  undefined *puVar3;
  float *pfVar4;
  int unaff_ESI;
  float10 fVar5;
  float10 fVar6;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  if (*(int *)(*(int *)((*(uint *)(*(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x8c)
                        & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x68) != 3) {
    FUN_00507840(param_1,0,param_2,0,0);
    return;
  }
  fVar5 = ABS(SQRT((float10)(float)puVar2[0x1c] * (float10)(float)puVar2[0x1c] +
                   (float10)(float)puVar2[0x1b] * (float10)(float)puVar2[0x1b] +
                   (float10)(float)puVar2[0x1a] * (float10)(float)puVar2[0x1a]) * (float10)2.5);
  fVar1 = (float)puVar2[0x137];
  if ((float10)1.0 < fVar5) {
    fVar5 = (float10)1.0;
  }
  *(uint *)(unaff_ESI + 4) = puVar2[0x135];
  fVar5 = ((float10)1.0 - fVar5) * (float10)(fVar1 * 0.5);
  *(undefined4 *)(unaff_ESI + 0xc) = 0x3b449ba6;
  puVar3 = PTR_DAT_00696720;
  fVar6 = (float10)fsin(fVar5);
  *(undefined4 *)(unaff_ESI + 0x1c) = 0;
  *(undefined4 *)(unaff_ESI + 0x20) = 0;
  *(float *)(unaff_ESI + 0x24) = (float)fVar6;
  fVar5 = (float10)fcos(fVar5);
  *(float *)(unaff_ESI + 0x28) = (float)fVar5;
  *(undefined4 *)(unaff_ESI + 0x6c) = 0x3b449ba6;
  *(undefined4 *)(unaff_ESI + 0x7c) = 0;
  *(undefined4 *)(unaff_ESI + 0x80) = 0;
  *(undefined4 *)(unaff_ESI + 0x84) = 0;
  *(undefined4 *)(unaff_ESI + 0x88) = 0x3f800000;
  *(undefined4 *)(unaff_ESI + 0xcc) = 0x3ba3d70a;
  *(undefined4 *)(unaff_ESI + 0xe8) = 0x3f800000;
  *(undefined4 *)(unaff_ESI + 0xdc) = 0;
  *(undefined4 *)(unaff_ESI + 0xe0) = 0;
  *(undefined4 *)(unaff_ESI + 0xe4) = 0;
  fVar5 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 != fVar5) {
    vector3d_cross_product(puVar2 + 0x20);
    pfVar4 = (float *)vector3d_cross_product(puVar2 + 0x1d);
    fVar5 = ((float10)*pfVar4 * (float10)*(float *)puVar3 +
            (float10)pfVar4[1] * (float10)*(float *)(puVar3 + 4) +
            (float10)pfVar4[2] * (float10)*(float *)(puVar3 + 8)) * (float10)6.2831855;
    fVar6 = (float10)fcos(fVar5);
    fVar5 = (float10)fsin(fVar5);
    vector3d_rotate_about_axis((float)fVar5,(float)fVar6);
    vector3d_angle_between_4cd4f0();
  }
  FUN_00507840(param_1);
  return;
}
#endif
