// player_effect_random_shake_offset  (Ghidra: FUN_00457280, still unnamed there; not individually
//   named by out/phase4/effects_types_notes.md, which lists it only as one of the 15 functions
//   in the 0x456730..0x457d50 player_effect range; named here from its role as the two-call
//   helper player_effect_build_camera_shake_matrix 0x457390 uses to build a random rotation +
//   translation offset matrix)
// address 0x457280, size 262 bytes
// name confidence: 0.3 (LOW -- own name is a guess)   rewrite confidence: 0.9 (VERIFIED against objdump 0x457280..0x457385) (LOW -- see UNSURE)
// evidence: src/math/vector3d_randomize_direction.c establishes sphere_point_table /
//   sphere_point_table_count (0x006b7af4 / 0x006b7af8); types/math.h real_matrix4x3.position
//   (+0x28) matches the three floats this function writes at +0x28/+0x2c/+0x30;
//   src/units/unit_update_recoil_decay.c establishes matrix4x3_from_axis_angle's real
//   (out, axis, sin_angle, cos_angle) signature.
// register convention: output matrix pointer in EDX (unaff_EDX, never assigned a value anywhere
//   in this function's own decompile); magnitude and angle are Ghidra's own recognized stack
//   parameters (param_1, param_2).
//   // blam-cc: unaff_EDX -> out, stack -> (magnitude, angle)
// UNSURE: matrix4x3_from_axis_angle's `axis` argument is fully elided by Ghidra along with `out`;
//   reconstructed as a second, independent roll into the same sphere_point_table every other
//   random-axis lookup in this module uses, by analogy with effect_random_direction_from_table.c
//   -- not read from the decompile itself. Because that reconstructed roll is inserted before
//   the position roll, and each roll advances the shared LCG, the exact random *sequence* this
//   produces relative to the original binary cannot be verified from the decompile alone.

#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern real_point3d *sphere_point_table;  // 0x006b7af4, 1026 unit vectors
extern int16_t sphere_point_table_count;  // 0x006b7af8, 1026
extern random_seed effect_random_seed;    // 0x00719cd4

extern double cos(double x);
extern double sin(double x);
extern void matrix4x3_from_axis_angle(real_matrix4x3 *out, real_vector3d *axis, real sin_angle,
                                       real cos_angle); // 0x4cb880

// Builds a random offset matrix: when `angle` is nonzero, a rotation of `angle` radians about a
// random unit axis (via matrix4x3_from_axis_angle); when `magnitude` is nonzero, a random
// direction scaled by `magnitude` written into the matrix's position field.
void player_effect_random_shake_offset(real_matrix4x3 *out, real magnitude, real angle)
{
    if (angle != 0.0f) {
        real cos_angle = (real)cos((double)angle);
        real_vector3d axis;
        int16_t axis_index;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment; // UNSURE,
                                    // see file header
        axis_index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)sphere_point_table_count) >> 16);
        axis = *(real_vector3d *)&sphere_point_table[axis_index];

        {
            real sin_angle = (real)sin((double)angle);
            matrix4x3_from_axis_angle(out, &axis, sin_angle, cos_angle);
        }
    }

    if (magnitude != 0.0f) {
        int16_t index;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        index = (int16_t)(((effect_random_seed >> k_random_value_shift) *
            (uint32_t)(int32_t)sphere_point_table_count) >> 16);

        out->position.x = sphere_point_table[index].x * magnitude;
        out->position.y = sphere_point_table[index].y * magnitude;
        out->position.z = sphere_point_table[index].z * magnitude;
    }
}

#if 0
Original Ghidra decompilation (0x457280):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00457280(float param_1,float param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int in_EDX;
  int extraout_EDX;
  short sVar6;
  float10 fVar7;
  float10 fVar8;

  iVar4 = DAT_006b7af4;
  sVar6 = (short)_DAT_006b7af8;
  if (param_2 != 0.0) {
    fVar7 = (float10)fcos((float10)param_2);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    fVar8 = (float10)fsin((float10)param_2);
    matrix4x3_from_axis_angle((float)fVar8,(float)fVar7);
    in_EDX = extraout_EDX;
  }
  if (param_1 != 0.0) {
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    iVar5 = (int)(short)((DAT_00719cd4 >> 0x10) * (int)sVar6 >> 0x10);
    iVar1 = iVar4 + iVar5 * 0xc;
    fVar2 = *(float *)(iVar1 + 4);
    fVar3 = *(float *)(iVar1 + 8);
    *(float *)(in_EDX + 0x28) = *(float *)(iVar4 + iVar5 * 0xc) * param_1;
    *(float *)(in_EDX + 0x2c) = fVar2 * param_1;
    *(float *)(in_EDX + 0x30) = fVar3 * param_1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
