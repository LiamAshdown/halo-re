// effect_random_velocity_vector  (Ghidra: FUN_00451310, still unnamed there; named from its own
// summary in out/phase4/effects_functions.md: "Computes a random-length velocity/offset vector,
// optionally rotated by a random angle about a given axis, for a particle system's spawn
// parameters")
// address 0x451310, size 309 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump 0x451310..0x451444; property roll bits FIXED) (raised by the phase-4 integration pass: the
// signature below is now read off Ghidra's own nine recognised stack parameters instead of
// being reconstructed, which resolves the conflict the effect_spawn_particles.c rewrite flagged)
// evidence: types/effects.h effect (a_scale 0x44, b_scale 0x48); src/math/vector3d_rotate_about_axis.c
// establishes vector3d_rotate_about_axis(v, axis, sin, cos); src/math/vector3d_randomize_direction.c
// establishes the sphere_point_table sampling idiom.
// register convention: caller-owned effect* in EAX (in_EAX, read after the first call for
// a_scale/b_scale); the nine stack parameters are Ghidra's own, in its own order:
// (seed, direction, out_direction, out_velocity, min, max, angle, a_bitset, b_bitset).
//   // blam-cc: EAX -> self, stack -> (seed, direction, out_direction, out_velocity, min, max,
//   //                                 angle, a_bitset, b_bitset)
// NOTE: param_3 and param_4 are OUTPUT pointers, not the `min`/`max` values an earlier draft of
// this file declared. Ghidra writes *param_3 = *param_2 verbatim and then reads param_3 back to
// build param_4, and effect_spawn_particles 0x451f90's call site reads both back as 3-float
// vectors immediately afterwards. min/max are param_5/param_6.
// UNSURE: the call to effect_property_random_value here shows only 3 of its 7 arguments
// (seed, base_min, base_max); the bit_index/self/a_bitset/b_bitset it also needs are fully
// elided. Passed here as 0/self/0/0, which is a guess.
// UNSURE: the rotation axis argument to vector3d_rotate_about_axis (ECX, per its own established
// convention) is entirely elided; reconstructed here as a fresh draw from the shared
// sphere_point_table, matching this module's other "random direction" helpers, but not proven.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026

extern double cos(double x);
extern double sin(double x);

extern real effect_property_random_value(uint8_t bit_index, effect *self, uint32_t a_bitset,
    uint32_t b_bitset, random_seed *seed, real base_min, real base_max); // 0x451290, this module
extern void vector3d_rotate_about_axis(real_vector3d *v, real_vector3d *axis, real sin_angle,
    real cos_angle); // 0x4cd820, math module; blam-cc: EAX -> v, ECX -> axis

// Copies `direction` into `out_direction`, rotates it by a random angle (up to `angle`, scaled
// by the A/B scales when bit 2 of either bitset is set) about a random axis, and writes
// out_direction scaled by a random magnitude in [min, max) into `out_velocity`.
void effect_random_velocity_vector(effect *self, random_seed *seed, real_vector3d *direction,
    real_vector3d *out_direction, real_vector3d *out_velocity, real min, real max, real angle_max,
    uint32_t a_bitset, uint8_t b_bitset)
{
    // FIXED (0x451319..0x451332): ESI / EDI carry this function's a_bitset / b_bitset into the property roll
    real magnitude = effect_property_random_value(0, self, a_bitset, b_bitset, seed, min, max);
    real angle = angle_max;

    *out_direction = *direction;

    if ((a_bitset & 4) != 0) {
        angle = angle_max * self->a_scale;
    }
    if ((b_bitset & 4) != 0) {
        angle = angle * self->b_scale;
    }

    *seed = *seed * k_random_multiplier + k_random_increment;
    angle = (real)(*seed >> k_random_value_shift) * 1.5259022e-05f * angle;

    if (angle != 0.0f) {
        real sin_angle;
        real cos_angle;
        real_vector3d axis;
        int16_t index;

        cos_angle = (real)cos(angle);
        *seed = *seed * k_random_multiplier + k_random_increment;
        sin_angle = (real)sin(angle);

        index = (int16_t)(((*seed >> k_random_value_shift) * (uint32_t)(int32_t)sphere_point_table_count) >> 16);
        axis.i = sphere_point_table[index].x;
        axis.j = sphere_point_table[index].y;
        axis.k = sphere_point_table[index].z;

        // Rotated in place (src/math/vector3d_rotate_about_axis.c: EAX is both the input and
        // the output). Ghidra's `param_3 = extraout_EDX` after the call is that same pointer
        // coming back in EDX, not a different buffer.
        vector3d_rotate_about_axis(out_direction, &axis, sin_angle, cos_angle);
    }

    out_velocity->i = magnitude * out_direction->i;
    out_velocity->j = magnitude * out_direction->j;
    out_velocity->k = magnitude * out_direction->k;
}

#if 0
Original Ghidra decompilation (0x451310):

/* WARNING: Removing unreachable block (ram,0x0045137e) */

void FUN_00451310(uint *param_1,float *param_2,float *param_3,float *param_4,undefined4 param_5,
                 undefined4 param_6,float param_7,uint param_8,byte param_9)

{
  float fVar1;
  int in_EAX;
  uint uVar2;
  float *extraout_EDX;
  float10 fVar3;
  float10 fVar4;

  fVar3 = (float10)particle_system_property_random_value(param_1,param_5,param_6);
  fVar1 = (float)fVar3;
  fVar3 = (float10)param_7;
  if ((param_8 & 4) != 0) {
    fVar3 = (float10)param_7 * (float10)*(float *)(in_EAX + 0x44);
  }
  if ((param_9 & 4) != 0) {
    fVar3 = fVar3 * (float10)*(float *)(in_EAX + 0x48);
  }
  uVar2 = *param_1 * 0x19660d + 0x3c6ef35f;
  *param_1 = uVar2;
  fVar3 = (float10)(uVar2 >> 0x10) * (float10)1.5259022e-05 * fVar3;
  *param_3 = *param_2;
  param_3[1] = param_2[1];
  param_3[2] = param_2[2];
  if (fVar3 != (float10)0.0) {
    fVar4 = (float10)fcos(fVar3);
    *param_1 = *param_1 * 0x19660d + 0x3c6ef35f;
    fVar3 = (float10)fsin(fVar3);
    vector3d_rotate_about_axis((float)fVar3,(float)fVar4);
    param_3 = extraout_EDX;
  }
  *param_4 = fVar1 * *param_3;
  param_4[1] = fVar1 * param_3[1];
  param_4[2] = fVar1 * param_3[2];
  return;
}
#endif
