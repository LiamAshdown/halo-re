// matrix4x3_transform_normal  (Ghidra: matrix4x3_transform_normal, already named)
// address 0x4cbec0, size 76 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: types/math.h real_matrix4x3 section ("matrix4x3_transform_normal reads
//   +0x04/+0x10/+0x1c for the first output component, which is the same statement in byte
//   offsets" as matrix4x3_transform_point's forward/left/up rows). No scale or translation is
//   applied, matching a direction/normal transform.
// register convention: output normal in EAX (in_EAX), input normal in EDX (in_EDX); matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> normal, stack -> m

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// Transforms a normal/direction by only the rotation part of a matrix4x3.
void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m)
{
    real i, j, k;

    i = normal->i;
    j = normal->j;
    k = normal->k;
    out->i = i * m->forward.i + j * m->left.i + k * m->up.i;
    out->j = i * m->forward.j + j * m->left.j + k * m->up.j;
    out->k = i * m->forward.k + j * m->left.k + k * m->up.k;
}

#if 0
Original Ghidra decompilation (0x4cbec0):

void matrix4x3_transform_normal(int param_1)

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
            fVar2 * *(float *)(param_1 + 0x10) + fVar3 * *(float *)(param_1 + 0x1c);
  in_EAX[1] = fVar1 * *(float *)(param_1 + 8) +
              fVar2 * *(float *)(param_1 + 0x14) + fVar3 * *(float *)(param_1 + 0x20);
  in_EAX[2] = fVar1 * *(float *)(param_1 + 0xc) +
              fVar2 * *(float *)(param_1 + 0x18) + fVar3 * *(float *)(param_1 + 0x24);
  return;
}
#endif
