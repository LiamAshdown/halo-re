// matrix3x3_from_forward_up  (Ghidra: FUN_004cc560; renamed, Blam-style, not previously named)
// address 0x4cc560, size 129 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/math_functions.md ("Builds a basis matrix from two vectors and their
//   cross product"); same forward/left=cross/up pattern as matrix4x3_from_forward_up @0x4cb970,
//   but for a bare real_matrix3x3 (no scale or translation).
// register convention: up vector in ECX (in_ECX), forward vector in EDX (in_EDX); output matrix
//   as the recognized stack parameter (param_1).
//   // blam-cc: ECX -> up, EDX -> forward, stack -> out

#include "tags.h"
#include "math.h"

// Builds a 3x3 basis matrix from two vectors and their cross product.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void matrix3x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix3x3 *out)
{
    out->forward = *forward;
    out->left.i = up->j * forward->k - up->k * forward->j;
    out->left.j = up->k * forward->i - forward->k * up->i;
    out->left.k = up->i * forward->j - up->j * forward->i;
    out->up = *up;
}

#if 0
Original Ghidra decompilation (0x4cc560):

void FUN_004cc560(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_ECX;
  float *in_EDX;

  *param_1 = *in_EDX;
  param_1[1] = in_EDX[1];
  param_1[2] = in_EDX[2];
  fVar1 = in_ECX[2];
  fVar2 = *in_EDX;
  fVar3 = in_EDX[2];
  fVar4 = *in_ECX;
  fVar5 = in_EDX[1];
  fVar6 = *in_ECX;
  fVar7 = in_ECX[1];
  fVar8 = *in_EDX;
  param_1[3] = in_ECX[1] * in_EDX[2] - in_ECX[2] * in_EDX[1];
  param_1[4] = fVar1 * fVar2 - fVar3 * fVar4;
  param_1[5] = fVar5 * fVar6 - fVar7 * fVar8;
  param_1[6] = *in_ECX;
  param_1[7] = in_ECX[1];
  param_1[8] = in_ECX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
