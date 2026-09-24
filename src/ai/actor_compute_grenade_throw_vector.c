// actor_compute_grenade_throw_vector  (Ghidra: actor_compute_grenade_throw_vector, renamed)
// address 0x410a60, size 560 bytes
// name confidence: 0.5   rewrite confidence: 0.35
// evidence: it reads actor.unknown_6b4 (the grenade target prop handle actor_commit_grenade_toss
//   @0x411180 writes), optionally revalidates the impact point through 0x410710, runs
//   actor_solve_grenade_lob @0x410780 and then turns the four grenade scratch floats
//   (grenade_unknown_6bc..6c8) into the caller aim vector, rotating it back toward the
//   actor facing when the throw would be outside a 30 degree cone.
// register convention: actor_index in EBX; the out-vector is a Ghidra-recognized stack
//   parameter.
//
// UNSURE (high): Ghidra leaves local_18 / local_14 / local_10 uninitialized on the vehicle
// path (actor.active_unit_index valid), and drops every argument of
// vector3d_rotate_about_axis except the two it could see. The rewrite keeps the control
// flow exactly and seeds the three locals from the committed throw direction, which is the
// only reading in which the tail makes sense. Treat the vehicle path as unverified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern const real_vector3d *global_up3d_pointer; // 0x00696720

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern void vector3d_rotate_about_axis(real_vector3d *v, const real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820
extern uint8_t actor_validate_grenade_impact_point(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_validate_grenade_impact_point at 0x410710
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_solve_grenade_lob(datum_index actor_index, real_point3d *point); // 0x410780, this module

// blam-cc: EBX -> actor_index, stack -> out_vector
// Produces the final grenade throw vector and returns the object the grenade is aimed at,
// or none. A grenade target prop whose kind is 2 or 3 contributes that object handle; any
// kind outside 0..1 also re-runs the impact point validation. On foot, a throw direction
// more than 30 degrees off the actor facing is rotated back by half a radian toward it.
uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_vector3d *out_vector)
{
    actor *self;
    prop *target;
    real_vector3d throw_direction;
    uint32_t target_object;
    int16_t kind;
    float length;
    float unit_i;
    float unit_j;
    float facing_i;
    float facing_j;
    float speed;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target_object = 0xffffffff;

    if (self->unknown_6b4 != 0xffffffff) {
        target = &((prop *)prop_data->data)[self->unknown_6b4 & 0xffff];
        kind = target->kind;
        if (kind > 1 && kind < 4) {
            target_object = (uint32_t)target->object_index;
        }
        if (kind < 0 || kind > 1) {
            // UNSURE: actor_validate_grenade_impact_point takes its arguments in registers.
            actor_validate_grenade_impact_point();
        }
    }

    // UNSURE: bare call in the original; actor_index is the only live value.
    actor_solve_grenade_lob(actor_index, (real_point3d *)0);

    throw_direction.i = self->grenade_unknown_6bc;
    throw_direction.j = self->grenade_unknown_6c0;
    throw_direction.k = self->grenade_unknown_6c4;

    if (self->active_unit_index == (datum_index)0xffffffff) {
        length = (float)sqrt((double)(throw_direction.i * throw_direction.i +
                                      throw_direction.j * throw_direction.j));
        if (length >= 0.0001f || length <= -0.0001f) {
            unit_i = throw_direction.i * (1.0f / length);
            unit_j = throw_direction.j * (1.0f / length);
            if (length > 0.0f &&
                unit_i * self->facing.i + unit_j * self->facing.j < 0.8660254f) {
                facing_i = self->facing.i;
                facing_j = self->facing.j;
                // Half a radian, signed by which side of the facing the throw fell on.
                vector3d_rotate_about_axis(&throw_direction, global_up3d_pointer,
                    (float)((int32_t)((unit_j * self->facing.i -
                                       unit_i * self->facing.j > 0.0f) * 2 - 1)) * 0.5f,
                    0.8660254f);
                length = (float)sqrt((double)(throw_direction.i * throw_direction.i +
                                              throw_direction.j * throw_direction.j));
                throw_direction.i = facing_i * length;
                throw_direction.j = facing_j * length;
            }
        }
    }

    speed = self->grenade_unknown_6c8;
    out_vector->i = throw_direction.i * speed;
    out_vector->j = throw_direction.j * speed;
    out_vector->k = speed * throw_direction.k;
    return target_object;
}

#if 0
Original Ghidra decompilation (0x410a60):

undefined4 FUN_00410a60(undefined4 param_1,float *param_2)

{
  float *pfVar1;
  short sVar2;
  uint uVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int iVar8;
  uint unaff_EBX;
  undefined4 uVar9;
  int iVar10;
  float local_18;
  float local_14;
  float local_10;

  iVar10 = (unaff_EBX & 0xffff) * 0x724;
  uVar3 = *(uint *)(iVar10 + 0x6b4 + *(int *)(DAT_00880360 + 0x34));
  iVar10 = iVar10 + *(int *)(DAT_00880360 + 0x34);
  uVar9 = 0xffffffff;
  if (uVar3 != 0xffffffff) {
    iVar8 = (uVar3 & 0xffff) * 0x138;
    sVar2 = *(short *)(iVar8 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
    if ((1 < sVar2) && (sVar2 < 4)) {
      uVar9 = *(undefined4 *)(iVar8 + *(int *)(DAT_008802c0 + 0x34) + 0x18);
    }
    if ((sVar2 < 0) || (1 < sVar2)) {
      FUN_00410710();
    }
  }
  FUN_00410780();
  if (*(int *)(iVar10 + 0x158) == -1) {
    local_18 = *(float *)(iVar10 + 0x6bc);
    local_14 = *(float *)(iVar10 + 0x6c0);
    local_10 = *(float *)(iVar10 + 0x6c4);
    fVar4 = SQRT(local_18 * local_18 + local_14 * local_14);
    if (0.0001 <= ABS(fVar4)) {
      fVar6 = local_18 * (1.0 / fVar4);
      fVar7 = local_14 * (1.0 / fVar4);
      if ((0.0 < fVar4) &&
         (pfVar1 = (float *)(iVar10 + 0x174),
         fVar6 * *pfVar1 + fVar7 * *(float *)(iVar10 + 0x178) < 0.8660254)) {
        fVar4 = *pfVar1;
        fVar5 = *(float *)(iVar10 + 0x178);
        vector3d_rotate_about_axis
                  ((float)(int)((uint)(0.0 < fVar7 * *pfVar1 - fVar6 * *(float *)(iVar10 + 0x178)) *
                                2 + -1) * 0.5,0x3f5db3d7);
        fVar6 = SQRT(local_18 * local_18 + local_14 * local_14);
        local_18 = fVar4 * fVar6;
        local_14 = fVar5 * fVar6;
      }
    }
  }
  fVar4 = *(float *)(iVar10 + 0x6c8);
  *param_2 = local_18 * fVar4;
  param_2[1] = local_14 * fVar4;
  param_2[2] = fVar4 * local_10;
  return uVar9;
}
#endif
