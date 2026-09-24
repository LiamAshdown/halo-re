// matrix3x3_multiply  (Ghidra: matrix3x3_multiply, already named)
// address 0x4cc5f0, size 277 bytes
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md ("Multiplies two 3x3 matrices"); field-by-field
//   identical to the rotation block of matrix4x3_multiply @0x4cc0d0 (out = a * b), minus the
//   scale/translation terms.
// register convention: output matrix in EAX (in_EAX), matrix a in EDX (in_EDX); matrix b as the
//   recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> a, stack -> b

#include "tags.h"
#include "math.h"

// Multiplies two 3x3 matrices.
void matrix3x3_multiply(real_matrix3x3 *out, real_matrix3x3 *a, real_matrix3x3 *b)
{
    real_matrix3x3 scratch;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }

    out->forward.i = b->forward.k * a->up.i + a->left.i * b->forward.j + a->forward.i * b->forward.i;
    out->forward.j = a->up.j * b->forward.k + a->forward.j * b->forward.i + a->left.j * b->forward.j;
    out->forward.k = a->up.k * b->forward.k + a->forward.k * b->forward.i + a->left.k * b->forward.j;
    out->left.i = b->left.k * a->up.i + b->left.i * a->forward.i + a->left.i * b->left.j;
    out->left.j = a->forward.j * b->left.i + a->left.j * b->left.j + a->up.j * b->left.k;
    out->left.k = a->forward.k * b->left.i + a->left.k * b->left.j + a->up.k * b->left.k;
    out->up.i = a->up.i * b->up.k + a->left.i * b->up.j + a->forward.i * b->up.i;
    out->up.j = a->forward.j * b->up.i + a->left.j * b->up.j + a->up.j * b->up.k;
    out->up.k = a->forward.k * b->up.i + a->left.k * b->up.j + a->up.k * b->up.k;
}

#if 0
Original Ghidra decompilation (0x4cc5f0):

void matrix3x3_multiply(float *param_1)

{
  float *in_EAX;
  int iVar1;
  float *in_EDX;
  float *pfVar2;
  float local_24 [9];

  if (in_EDX == in_EAX) {
    pfVar2 = local_24;
    for (iVar1 = 9; iVar1 != 0; iVar1 = iVar1 + -1) {
      *pfVar2 = *in_EDX;
      in_EDX = in_EDX + 1;
      pfVar2 = pfVar2 + 1;
    }
    in_EDX = local_24;
  }
  if (param_1 == in_EAX) {
    pfVar2 = local_24;
    for (iVar1 = 9; iVar1 != 0; iVar1 = iVar1 + -1) {
      *pfVar2 = *param_1;
      param_1 = param_1 + 1;
      pfVar2 = pfVar2 + 1;
    }
    param_1 = local_24;
  }
  *in_EAX = param_1[2] * in_EDX[6] + in_EDX[3] * param_1[1] + *in_EDX * *param_1;
  in_EAX[1] = in_EDX[7] * param_1[2] + in_EDX[1] * *param_1 + in_EDX[4] * param_1[1];
  in_EAX[2] = in_EDX[8] * param_1[2] + in_EDX[2] * *param_1 + in_EDX[5] * param_1[1];
  in_EAX[3] = param_1[5] * in_EDX[6] + param_1[3] * *in_EDX + in_EDX[3] * param_1[4];
  in_EAX[4] = in_EDX[1] * param_1[3] + in_EDX[4] * param_1[4] + in_EDX[7] * param_1[5];
  in_EAX[5] = in_EDX[2] * param_1[3] + in_EDX[5] * param_1[4] + in_EDX[8] * param_1[5];
  in_EAX[6] = in_EDX[6] * param_1[8] + in_EDX[3] * param_1[7] + *in_EDX * param_1[6];
  in_EAX[7] = in_EDX[1] * param_1[6] + in_EDX[4] * param_1[7] + in_EDX[7] * param_1[8];
  in_EAX[8] = in_EDX[2] * param_1[6] + in_EDX[5] * param_1[7] + in_EDX[8] * param_1[8];
  return;
}
#endif
