// matrix4x3_inverse  (Ghidra: matrix4x3_inverse, already named)
// address 0x4cb7a0, size 212 bytes
// name confidence: 0.9   rewrite confidence: 0.85
// evidence: types/math.h real_matrix4x3 section (scale at +0x00, forward/left/up rows,
//   position last; zero-fills exactly 13 floats on the degenerate path, which is the evidence
//   for the struct's size). The rotation block is transposed in place (inverse of an orthonormal
//   basis), the scale is reciprocated, and the new position is -scale^-1 * position rotated by
//   the transposed (old) basis; each position component is read back from the just-written
//   `out` fields (rather than recomputed from `in`) so the function stays correct when out and
//   in alias the same matrix.
// register convention: output matrix in EAX (in_EAX), input matrix in ECX (in_ECX).
//   // blam-cc: EAX -> out, ECX -> in

#include "tags.h"
#include "math.h"

// Computes the inverse of a matrix4x3 (uniform scale + rotation + translation).
void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in)
{
    real neg_x, neg_y, neg_z;
    real inv_scale;
    real t1, t2;
    int i;
    real *zero;

    if (in->scale != 0.0f) {
        neg_x = -in->position.x;
        neg_y = -in->position.y;
        neg_z = -in->position.z;
        if (in->scale == 1.0f) {
            out->scale = 1.0f;
        } else {
            inv_scale = 1.0f / in->scale;
            out->scale = inv_scale;
            neg_x = inv_scale * neg_x;
            neg_y = inv_scale * neg_y;
            neg_z = inv_scale * neg_z;
        }

        out->forward.i = in->forward.i;
        out->left.j = in->left.j;
        out->up.k = in->up.k;

        t1 = in->forward.j;
        out->forward.j = in->left.i;
        out->left.i = t1;

        t1 = in->forward.k;
        out->forward.k = in->up.i;
        out->up.i = t1;

        t1 = in->up.j;
        t2 = in->left.k;
        out->left.k = t1;
        out->up.j = t2;

        out->position.x = neg_y * out->left.i + neg_x * out->forward.i + neg_z * out->up.i;
        out->position.y = neg_y * out->left.j + neg_x * out->forward.j + neg_z * out->up.j;
        out->position.z = t1 * neg_y + neg_z * out->up.k + neg_x * out->forward.k;
    } else {
        zero = (real *)out;
        for (i = 0xd; i != 0; i--) {
            *zero = 0.0f;
            zero++;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4cb7a0):

void matrix4x3_inverse(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float *in_EAX;
  float *in_ECX;
  int iVar6;

  if (*in_ECX != 0.0) {
    fVar3 = -in_ECX[10];
    fVar5 = -in_ECX[0xb];
    fVar4 = -in_ECX[0xc];
    if (*in_ECX == 1.0) {
      *in_EAX = 1.0;
    }
    else {
      fVar1 = 1.0 / *in_ECX;
      *in_EAX = fVar1;
      fVar3 = fVar1 * fVar3;
      fVar5 = fVar1 * fVar5;
      fVar4 = fVar1 * fVar4;
    }
    in_EAX[1] = in_ECX[1];
    in_EAX[5] = in_ECX[5];
    in_EAX[9] = in_ECX[9];
    fVar1 = in_ECX[2];
    in_EAX[2] = in_ECX[4];
    in_EAX[4] = fVar1;
    fVar1 = in_ECX[3];
    in_EAX[3] = in_ECX[7];
    in_EAX[7] = fVar1;
    fVar1 = in_ECX[8];
    fVar2 = in_ECX[6];
    in_EAX[6] = fVar1;
    in_EAX[8] = fVar2;
    in_EAX[10] = fVar5 * in_EAX[4] + fVar3 * in_EAX[1] + fVar4 * in_EAX[7];
    in_EAX[0xb] = fVar5 * in_EAX[5] + fVar3 * in_EAX[2] + fVar4 * in_EAX[8];
    in_EAX[0xc] = fVar1 * fVar5 + fVar4 * in_EAX[9] + fVar3 * in_EAX[3];
    return;
  }
  for (iVar6 = 0xd; iVar6 != 0; iVar6 = iVar6 + -1) {
    *in_EAX = 0.0;
    in_EAX = in_EAX + 1;
  }
  return;
}
#endif
