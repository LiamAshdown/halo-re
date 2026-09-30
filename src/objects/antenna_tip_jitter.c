// antenna_tip_jitter
// address 0x4fef40, size 197 bytes
// name confidence: 0.4 (still FUN_004fef40 in Ghidra; functions.md: "Applies a small random
//   jitter, oriented by the object's transform, to a position (e.g. antenna tip wobble)")
// rewrite confidence: 0.3
// evidence: types/math.h real_matrix4x3, real_vector3d; matrix4x3_transform_vector 0x4cbe50
//   (established: EAX -> out, EDX -> vector, stack -> m); global 0x00719cd4 effect_random_seed.
// register convention: Ghidra shows one clean stack parameter (the matrix) plus two unresolved
//   implicit inputs, `in_ECX` (a 3-float jitter amplitude, read but not written) and
//   `unaff_ESI` (the position accumulator, read-modify-written both before and after the
//   matrix4x3_transform_vector call). Neither is confirmed against a call site (this function's
//   one caller was not disassembled).
// blam-cc: ECX -> amplitude, ESI -> position, stack -> m (UNSURE, not independently confirmed)
// UNSURE: matrix4x3_transform_vector's own EAX/EDX inputs at this call site are not visible in
//   Ghidra's decompile at all (shown with zero arguments beyond the matrix); guessed here as
//   out=position, in=amplitude, which would transform the amplitude vector into position before
//   the random jitter is added on top of it. This may well be wrong.

#include "tags.h"
#include "memory.h"
#include "math.h"

extern uint32_t effect_random_seed; // 0x00719cd4

extern void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m); // 0x4cbe50

// VERIFIED against disassembly 0x4fef40..0x4ff004 (2026-09-30); fixed: see the block in the body.
void antenna_tip_jitter(real_vector3d *amplitude /*ECX*/, real_point3d *position /*ESI*/,
                         real_matrix4x3 *m)
    // blam-cc: ECX -> amplitude, ESI -> position, stack -> m (UNSURE, see file header)
{
    uint32_t s1 = effect_random_seed * 0x19660dU + 0x3c6ef35fU;
    uint32_t s2 = s1 * 0x19660dU + 0x3c6ef35fU;
    float rz = (float)(s1 >> 16) * 1.5259022e-05f;
    float ry, rx;

    effect_random_seed = s2 * 0x19660dU + 0x3c6ef35fU;
    ry = (float)(s2 >> 16) * 1.5259022e-05f;
    rx = (float)(effect_random_seed >> 16) * 1.5259022e-05f;

    {
        real_vector3d jitter;

        // 0x4fefa9..0x4fefe0: jitter = ((2*r3-1)*amp.i, (2*r2-1)*amp.j, (2*r1-1)*amp.k) is rotated IN PLACE by the matrix
        //   (matrix4x3_transform_vector with EAX = EDX = &jitter, stack m), then added to *position. The draft rotated the
        //   amplitude into *position (overwriting it) and added the un-rotated jitter.
        jitter.i = (rx + rx - 1.0f) * amplitude->i;
        jitter.j = (ry + ry - 1.0f) * amplitude->j;
        jitter.k = (rz + rz - 1.0f) * amplitude->k;
        matrix4x3_transform_vector(&jitter, &jitter, m);
        position->x = jitter.i + position->x;
        position->y = jitter.j + position->y;
        position->z = jitter.k + position->z;
    }
}

#if 0
Original Ghidra decompilation (0x4fef40):

void FUN_004fef40(undefined4 param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  uint uVar7;
  uint uVar8;
  float *in_ECX;
  float *unaff_ESI;

  uVar7 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  uVar8 = uVar7 * 0x19660d + 0x3c6ef35f;
  fVar4 = (float)(uVar7 >> 0x10) * 1.5259022e-05;
  DAT_00719cd4 = uVar8 * 0x19660d + 0x3c6ef35f;
  fVar5 = (float)(uVar8 >> 0x10) * 1.5259022e-05;
  fVar6 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
  fVar1 = *in_ECX;
  fVar2 = in_ECX[1];
  fVar3 = in_ECX[2];
  matrix4x3_transform_vector(param_1);
  *unaff_ESI = ((fVar6 + fVar6) - 1.0) * fVar1 + *unaff_ESI;
  unaff_ESI[1] = ((fVar5 + fVar5) - 1.0) * fVar2 + unaff_ESI[1];
  unaff_ESI[2] = ((fVar4 + fVar4) - 1.0) * fVar3 + unaff_ESI[2];
  return;
}
#endif
