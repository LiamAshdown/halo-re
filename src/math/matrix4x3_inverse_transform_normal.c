// matrix4x3_inverse_transform_normal  (Ghidra: matrix4x3_inverse_transform_normal, already named)
// address 0x4cc080, size 76 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: types/math.h real_matrix4x3 section ("matrix4x3_inverse_transform_normal reads
//   +0x04/+0x08/+0x0c for the first output component -- the transpose -- confirming the rows
//   from the other direction"). Dots the input normal with each rotation row, i.e. multiplies
//   by the transpose of the forward/left/up basis; no scale or translation.
// register convention: output normal in EAX (in_EAX), input normal in EDX (in_EDX); matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> normal, stack -> m

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Transforms a normal/direction into a matrix4x3's local space using only the transposed
// rotation.
void matrix4x3_inverse_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m)
{
    real i, j, k;

    i = normal->i;
    j = normal->j;
    k = normal->k;
    out->i = i * m->forward.i + j * m->forward.j + k * m->forward.k;
    out->j = i * m->left.i + j * m->left.j + k * m->left.k;
    out->k = i * m->up.i + j * m->up.j + k * m->up.k;
}

#if 0
Original Ghidra decompilation (0x4cc080):

void matrix4x3_inverse_transform_normal(int param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_EDX;

  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  *in_EAX = fVar1 * *(float *)(param_1 + 4) +
            fVar2 * *(float *)(param_1 + 8) + fVar3 * *(float *)(param_1 + 0xc);
  in_EAX[1] = fVar1 * *(float *)(param_1 + 0x10) +
              fVar2 * *(float *)(param_1 + 0x14) + fVar3 * *(float *)(param_1 + 0x18);
  in_EAX[2] = fVar1 * *(float *)(param_1 + 0x1c) +
              fVar2 * *(float *)(param_1 + 0x20) + fVar3 * *(float *)(param_1 + 0x24);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
