// matrix4x3_inverse_transform_vector  (Ghidra: matrix4x3_inverse_transform_vector, already named)
// address 0x4cc010, size 104 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/math_functions.md ("Transforms a vector from world space into a
//   matrix4x3's local space, without translation"); same undo-scale-then-dot-with-rows pattern
//   as matrix4x3_inverse_transform_point @0x4cbf80, minus the translation subtraction. Unlike
//   that function, there is no scale==0 special case here -- dividing by a zero scale is left
//   as-is, matching the decompile.
// register convention: output vector in EAX (in_EAX), input vector in EDX (in_EDX); matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> v, stack -> m

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Transforms a vector from world space into a matrix4x3's local space, without translation.
void matrix4x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix4x3 *m)
{
    real i, j, k;
    real inv_scale;

    i = v->i;
    j = v->j;
    k = v->k;
    if (m->scale != 1.0f) {
        inv_scale = 1.0f / m->scale;
        i = inv_scale * i;
        j = inv_scale * j;
        k = inv_scale * k;
    }
    out->i = i * m->forward.i + j * m->forward.j + k * m->forward.k;
    out->j = i * m->left.i + j * m->left.j + k * m->left.k;
    out->k = i * m->up.i + j * m->up.j + k * m->up.k;
}

#if 0
Original Ghidra decompilation (0x4cc010):

void matrix4x3_inverse_transform_vector(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  float *in_EDX;

  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  if (*param_1 != 1.0) {
    fVar4 = 1.0 / *param_1;
    fVar1 = fVar4 * fVar1;
    fVar2 = fVar4 * fVar2;
    fVar3 = fVar4 * fVar3;
  }
  *in_EAX = fVar1 * param_1[1] + fVar2 * param_1[2] + fVar3 * param_1[3];
  in_EAX[1] = fVar1 * param_1[4] + fVar2 * param_1[5] + fVar3 * param_1[6];
  in_EAX[2] = fVar1 * param_1[7] + fVar2 * param_1[8] + fVar3 * param_1[9];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
