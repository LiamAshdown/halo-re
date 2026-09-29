// actor_compute_grenade_throw_vector  (Ghidra: actor_compute_grenade_throw_vector, renamed)
// address 0x410a60, size 560 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// REWRITTEN from objdump 0x410a60..0x410c8f. EBX: actor, stack: (grenade position, out vector). The draft had no
//   grenade position and passed NULL to the lob solver (0x410780), and called the impact check (0x410710) without
//   its point. The grenade's target prop (+0x6b4) names the object hit (prop kinds 2..3, returned); for kinds other
//   than 0..1 its point (+0xbc, 0.2 higher) is validated. The lob is solved from the grenade's position into
//   +0x6bc..+0x6c8 (direction, speed). On foot, a throw more than 30 degrees off the actor's facing is turned to
//   30 degrees either side of the facing, keeping its horizontal length. Out = direction * speed.
// blam-cc: EBX -> actor_index, stack -> grenade_position, out_vector

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern real_vector3d *global_up3d_pointer; // 0x00696720

extern double sqrt(double x);
extern double fabs(double x);
extern uint8_t actor_validate_grenade_impact_point(datum_index actor_index, real_point3d *candidate_point); // 0x410710, EAX, EDI
extern uint32_t actor_solve_grenade_lob(datum_index actor_index, real_point3d *point); // 0x410780
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle, real cos_angle); // 0x4cd820

uint32_t actor_compute_grenade_throw_vector(datum_index actor_index, real_point3d *grenade_position,
                                            real_vector3d *out_vector)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint32_t target_object = 0xffffffff;    // ebp
    real_vector3d direction;                // [esp+0x10]
    float speed;

    if (*(datum_index *)&((struct actor *)a)->unknown_6b4 != k_datum_index_none) {
        uint8_t *p = (uint8_t *)prop_data->data + (*(datum_index *)&((struct actor *)a)->unknown_6b4 & 0xffff) * 0x138;
        int16_t kind = ((prop *)p)->state;

        if (kind >= 2 && kind <= 3) {
            target_object = ((prop *)p)->object_index;
        }
        if (kind < 0 || kind > 1) {
            real_point3d point = *(real_point3d *)&((prop *)p)->last_known_position.x;   // [esp+0x1c]

            point.z += 0.2f;
            actor_validate_grenade_impact_point(actor_index, &point);
        }
    }
    actor_solve_grenade_lob(actor_index, grenade_position);
    // (the binary leaves the direction uninitialised in a vehicle; seeded from the solution here)
    direction = *(real_vector3d *)&((actor *)a)->grenade_unknown_6bc;
    if (((actor *)a)->active_unit_index == k_datum_index_none) {
        real length = (real)sqrt(direction.j * direction.j + direction.i * direction.i);

        if (!(fabs(length) < 9.999999747378752e-05)) {
            real inverse = 1.0f / length;
            real flat_i = direction.i * inverse;
            real flat_j = direction.j * inverse;
            real_vector3d *facing = (real_vector3d *)(a + 0x174);

            if (length > 0.0f && !(flat_j * facing->j + flat_i * facing->i >= 0.8660254f)) {
                // more than 30 degrees off the facing: throw 30 degrees to that side of it
                real_vector3d turned = *facing;     // [esp+0x1c]
                real side = flat_j * facing->i - flat_i * facing->j;
                real sign = side > 0.0f ? 1.0f : -1.0f;
                real horizontal;

                vector3d_rotate_about_axis(&turned, global_up3d_pointer, sign * 0.5f, 0.8660254f);
                horizontal = (real)sqrt(direction.j * direction.j + direction.i * direction.i);
                direction.i = turned.i * horizontal;
                direction.j = turned.j * horizontal;
            }
        }
    }
    speed = ((actor *)a)->grenade_unknown_6c8;
    out_vector->i = direction.i * speed;
    out_vector->j = direction.j * speed;
    out_vector->k = speed * direction.k;
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
