// matrix4x3_from_forward_up  (Ghidra: FUN_004cb970; renamed, Blam-style, not previously named)
// address 0x4cb970, size 147 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: out/phase4/math_functions.md ("Builds the rotation part of a matrix4x3 from two
//   vectors and their cross product, leaving scale=1 and translation zeroed"). forward = the
//   ECX vector unchanged, up = the EAX vector unchanged, left = cross(up, forward), matching
//   types/math.h real_matrix4x3's forward/left/up row order.
// register convention: up vector in EAX (in_EAX), forward vector in ECX (in_ECX); output
//   matrix as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> up, ECX -> forward, stack -> out

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Builds the rotation part of a matrix4x3 from two vectors and their cross product, leaving
// scale=1 and translation zeroed.
void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out)
{
    out->scale = 1.0f;
    out->forward = *forward;
    out->left.i = up->j * forward->k - up->k * forward->j;
    out->left.j = up->k * forward->i - forward->k * up->i;
    out->left.k = up->i * forward->j - up->j * forward->i;
    out->up = *up;
    out->position.x = 0.0f;
    out->position.y = 0.0f;
    out->position.z = 0.0f;
}

#if 0
Original Ghidra decompilation (0x4cb970):

void FUN_004cb970(undefined4 *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float *in_EAX;
  float *in_ECX;

  *param_1 = 0x3f800000;
  param_1[1] = *in_ECX;
  param_1[2] = in_ECX[1];
  param_1[3] = in_ECX[2];
  fVar1 = in_EAX[2];
  fVar2 = *in_ECX;
  fVar3 = in_ECX[2];
  fVar4 = *in_EAX;
  fVar5 = *in_EAX;
  fVar6 = in_ECX[1];
  fVar7 = in_EAX[1];
  fVar8 = *in_ECX;
  param_1[4] = in_EAX[1] * in_ECX[2] - in_EAX[2] * in_ECX[1];
  param_1[5] = fVar1 * fVar2 - fVar3 * fVar4;
  param_1[6] = fVar5 * fVar6 - fVar7 * fVar8;
  param_1[7] = *in_EAX;
  param_1[8] = in_EAX[1];
  param_1[9] = in_EAX[2];
  param_1[10] = 0;
  param_1[0xb] = 0;
  param_1[0xc] = 0;
  return;
}
#endif
