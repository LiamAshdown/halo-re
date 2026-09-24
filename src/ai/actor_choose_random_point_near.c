// actor_choose_random_point_near  (Ghidra: actor_choose_random_point_near, renamed)
// address 0x40faf0, size 436 bytes
// name confidence: 0.45   rewrite confidence: 0.2
// evidence: phase-4 summary "chooses a randomized point within a given radius of the
// actor, pulling it back toward the actor if the line to it is obstructed".
// register convention: radius in the recognized stack param_1 (float), in/out point in ESI
// (Ghidra's unaff_ESI).
// blam-cc: ESI -> inout_point, stack -> radius
// UNSURE: FUN_00505880 (a line/collision test outside this module) is called once with a
// single visible argument (0x23, likely flags) and once with five; both call sites are
// modeled here with the same 5-argument shape, the first using the pre-adjustment vector,
// since that is the only way the second call's fully-visible signature makes sense as the
// same function. PTR_DAT_00696720 is guessed to be a shared constant vector pointer, by
// analogy with the neighboring shared zero vectors at 0x696714/0x696718 in types/math.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern const real_vector3d *global_up3d_pointer; // 0x00696720, UNSURE identity
extern uint32_t random_seed_global; // 0x00719cd0, types/math.h
extern double fcos(double x); // FCOS
extern double fsin(double x); // FSIN
extern uint8_t FUN_00505880(uint32_t mask, real_point3d *origin, real_vector3d *delta, uint32_t exclude_object, void *scratch); // UNSURE signature

// blam-cc: ESI -> inout_point, stack -> radius
void actor_choose_random_point_near(real_point3d *inout_point, float radius)
{
    real_point3d base;
    real_point3d chosen;
    real_vector3d delta;
    float cos_angle, sin_angle;
    uint8_t line_result[20];
    float clear_fraction;

    base.x = global_up3d_pointer->i * 1.5f + inout_point->x;
    base.y = global_up3d_pointer->j * 1.5f + inout_point->y;
    base.z = global_up3d_pointer->k * 1.5f + inout_point->z;

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    {
        uint32_t roll = random_seed_global >> 0x10;
        double angle = (double)roll * 1.5259022e-05 * 6.2831855 - 3.1415927;
        cos_angle = (float)fcos(angle);
        sin_angle = (float)fsin(angle);
    }

    chosen.x = cos_angle * radius + base.x;
    chosen.y = sin_angle * radius + base.y;
    chosen.z = radius * 0.0f + base.z;

    delta.i = base.x - inout_point->x;
    delta.j = base.y - inout_point->y;
    delta.k = base.z - inout_point->z;

    if (FUN_00505880(0x23, &base, &delta, (uint32_t)-1, line_result) != 0) {
        base = *inout_point;
    }

    delta.i = chosen.x - base.x;
    delta.j = chosen.y - base.y;
    delta.k = chosen.z - base.z;

    if (FUN_00505880(0x23, &base, &delta, (uint32_t)-1, line_result) != 0) {
        clear_fraction = *(float *)&line_result[0] * radius - 0.1f;
        if (clear_fraction < 0.0f) {
            clear_fraction = 0.0f;
        }
        chosen.x = cos_angle * clear_fraction + base.x;
        chosen.y = sin_angle * clear_fraction + base.y;
        chosen.z = clear_fraction * 0.0f + base.z;
    }

    *inout_point = chosen;
}

#if 0
Original Ghidra decompilation (0x40faf0):

void FUN_0040faf0(float param_1)

{
  float fVar1;
  char cVar2;
  float *unaff_ESI;
  float10 fVar3;
  float10 fVar4;
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
  uint local_54;
  undefined1 local_50 [20];
  float local_3c;

  local_84 = *(float *)PTR_DAT_00696720 * 1.5 + *unaff_ESI;
  local_80 = *(float *)(PTR_DAT_00696720 + 4) * 1.5 + unaff_ESI[1];
  local_7c = *(float *)(PTR_DAT_00696720 + 8) * 1.5 + unaff_ESI[2];
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  local_54 = random_seed_global >> 0x10;
  fVar3 = (float10)local_54 * (float10)1.5259022e-05 * (float10)6.2831855 - (float10)3.1415927;
  fVar4 = (float10)fcos(fVar3);
  local_60 = (float)fVar4;
  fVar3 = (float10)fsin(fVar3);
  local_5c = (float)fVar3;
  local_78 = local_60 * param_1 + local_84;
  local_74 = local_5c * param_1 + local_80;
  local_70 = param_1 * 0.0 + local_7c;
  local_6c = local_84 - *unaff_ESI;
  local_68 = local_80 - unaff_ESI[1];
  local_64 = local_7c - unaff_ESI[2];
  cVar2 = FUN_00505880(0x23);
  if (cVar2 != '\0') {
    local_84 = *unaff_ESI;
    local_80 = unaff_ESI[1];
    local_7c = unaff_ESI[2];
  }
  local_6c = local_78 - local_84;
  local_68 = local_74 - local_80;
  local_64 = local_70 - local_7c;
  cVar2 = FUN_00505880(0x23,&local_84,&local_6c,0xffffffff,local_50);
  if (cVar2 != '\0') {
    fVar1 = local_3c * param_1 - 0.1;
    if (fVar1 < 0.0) {
      fVar1 = 0.0;
    }
    local_78 = local_60 * fVar1 + local_84;
    local_74 = local_5c * fVar1 + local_80;
    local_70 = fVar1 * 0.0 + local_7c;
  }
  *unaff_ESI = local_78;
  unaff_ESI[1] = local_74;
  unaff_ESI[2] = local_70;
  return;
}
#endif
