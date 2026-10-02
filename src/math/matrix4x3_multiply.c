// matrix4x3_multiply  (Ghidra: matrix4x3_multiply, already named)
// address 0x4cc0d0, size 384 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: types/math.h real_matrix4x3 section ("matrix4x3_multiply spills 13 floats when the
//   destination aliases either operand" and "matrix4x3_multiply sets *out = *a * *b" for scale).
//   Ghidra already recovered the full __cdecl signature and field layout here; direct field-by-
//   field translation of `out = a * b`.
// register convention: __cdecl, all three parameters (a, b, out) on the stack as declared.

#include "tags.h"
#include "math.h"

// Multiplies two matrix4x3 matrices (param_1 * param_2) into a third.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void matrix4x3_multiply(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out)
{
    real_matrix4x3 scratch;

    if (a == out) {
        scratch = *a;
        a = &scratch;
    }
    if (b == out) {
        scratch = *b;
        b = &scratch;
    }

    out->forward.i = b->forward.k * a->up.i + a->forward.i * b->forward.i + a->left.i * b->forward.j;
    out->forward.j = a->left.j * b->forward.j + a->forward.j * b->forward.i + a->up.j * b->forward.k;
    out->forward.k = a->left.k * b->forward.j + a->forward.k * b->forward.i + a->up.k * b->forward.k;
    out->left.i = a->left.i * b->left.j + b->left.k * a->up.i + a->forward.i * b->left.i;
    out->left.j = a->forward.j * b->left.i + a->left.j * b->left.j + a->up.j * b->left.k;
    out->left.k = a->forward.k * b->left.i + a->left.k * b->left.j + a->up.k * b->left.k;
    out->up.i = a->left.i * b->up.j + a->forward.i * b->up.i + b->up.k * a->up.i;
    out->up.j = b->up.j * a->left.j + b->up.k * a->up.j + a->forward.j * b->up.i;
    out->up.k = b->up.j * a->left.k + b->up.k * a->up.k + a->forward.k * b->up.i;
    out->position.x = (b->position.z * a->up.i + a->left.i * b->position.y + a->forward.i * b->position.x) * a->scale + a->position.x;
    out->position.y = (b->position.x * a->forward.j + b->position.y * a->left.j + a->up.j * b->position.z) * a->scale + a->position.y;
    out->position.z = (b->position.x * a->forward.k + b->position.y * a->left.k + a->up.k * b->position.z) * a->scale + a->position.z;
    out->scale = a->scale * b->scale;
}

#if 0
Original Ghidra decompilation (0x4cc0d0):

void __cdecl matrix4x3_multiply(float *a,float *b,float *out)

{
  int iVar1;
  float *pfVar2;
  float local_34 [13];

  if (a == out) {
    pfVar2 = local_34;
    for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
      *pfVar2 = *a;
      a = a + 1;
      pfVar2 = pfVar2 + 1;
    }
    a = local_34;
  }
  if (b == out) {
    pfVar2 = local_34;
    for (iVar1 = 0xd; iVar1 != 0; iVar1 = iVar1 + -1) {
      *pfVar2 = *b;
      b = b + 1;
      pfVar2 = pfVar2 + 1;
    }
    b = local_34;
  }
  out[1] = b[3] * a[7] + a[1] * b[1] + a[4] * b[2];
  out[2] = a[5] * b[2] + a[2] * b[1] + a[8] * b[3];
  out[3] = a[6] * b[2] + a[3] * b[1] + a[9] * b[3];
  out[4] = a[4] * b[5] + b[6] * a[7] + a[1] * b[4];
  out[5] = a[2] * b[4] + a[5] * b[5] + a[8] * b[6];
  out[6] = a[3] * b[4] + a[6] * b[5] + a[9] * b[6];
  out[7] = a[4] * b[8] + a[1] * b[7] + b[9] * a[7];
  out[8] = b[8] * a[5] + b[9] * a[8] + a[2] * b[7];
  out[9] = b[8] * a[6] + b[9] * a[9] + a[3] * b[7];
  out[10] = (b[0xc] * a[7] + a[4] * b[0xb] + a[1] * b[10]) * *a + a[10];
  out[0xb] = (b[10] * a[2] + b[0xb] * a[5] + a[8] * b[0xc]) * *a + a[0xb];
  out[0xc] = (b[10] * a[3] + b[0xb] * a[6] + a[9] * b[0xc]) * *a + a[0xc];
  *out = *a * *b;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
