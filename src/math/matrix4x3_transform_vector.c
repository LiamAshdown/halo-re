// matrix4x3_transform_vector  (Ghidra: matrix4x3_transform_vector, already named)
// address 0x4cbe50, size 100 bytes
// name confidence: 0.65   rewrite confidence: 0.9
// evidence: out/phase4/math_functions.md ("Transforms a vector by a matrix4x3's scale and
//   rotation, without translation"); identical to matrix4x3_transform_point @0x4cbde0 minus the
//   translation add.
// register convention: output vector in EAX (in_EAX), input vector in EDX (in_EDX); matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> v, stack -> m
// FIXED (register inputs, objdump): EDX carries v (read at 0x4cbe50, fld [edx]); the notes said
// "EDX -> vector" but the parameter is named v, so the checker's alias match failed and treated
// EDX as unmapped even though the code already used it correctly.

#include "tags.h"
#include "math.h"

// Transforms a vector by a matrix4x3's scale and rotation, without translation.
void matrix4x3_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m)
{
    real i, j, k;

    i = v->i;
    j = v->j;
    k = v->k;
    if (m->scale != 1.0f) {
        i = i * m->scale;
        j = j * m->scale;
        k = k * m->scale;
    }
    out->i = i * m->forward.i + j * m->left.i + k * m->up.i;
    out->j = i * m->forward.j + j * m->left.j + k * m->up.j;
    out->k = i * m->forward.k + j * m->left.k + k * m->up.k;
}

#if 0
Original Ghidra decompilation (0x4cbe50):

void matrix4x3_transform_vector(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_EDX;

  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  if (*param_1 != 1.0) {
    fVar1 = fVar1 * *param_1;
    fVar2 = fVar2 * *param_1;
    fVar3 = fVar3 * *param_1;
  }
  *in_EAX = fVar1 * param_1[1] + fVar2 * param_1[4] + fVar3 * param_1[7];
  in_EAX[1] = fVar1 * param_1[2] + fVar2 * param_1[5] + fVar3 * param_1[8];
  in_EAX[2] = fVar1 * param_1[3] + fVar2 * param_1[6] + fVar3 * param_1[9];
  return;
}
#endif
