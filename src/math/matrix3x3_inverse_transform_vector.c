// matrix3x3_inverse_transform_vector  (Ghidra: matrix3x3_inverse_transform_vector, already named)
// address 0x4cc710, size 111 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/math.h real_matrix3x3 section ("matrix3x3_inverse_transform_vector computes
//   out.i = m[0]*v.i + m[3]*v.j + m[6]*v.k, i.e. the transpose of the forward/left/up rows,
//   which is the inverse for an orthonormal matrix"). Handles v aliasing out by snapshotting the
//   vector first.
// register convention: output vector in EAX (in_EAX), input vector in ECX (in_ECX); matrix as
//   the recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, ECX -> v, stack -> m

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Transforms a vector by the transpose of a 3x3 matrix (inverse transform for an orthonormal
// matrix).
void matrix3x3_inverse_transform_vector(real_vector3d *out, real_vector3d *v, real_matrix3x3 *m)
{
    real_vector3d snapshot;

    if (v == out) {
        snapshot = *v;
        v = &snapshot;
    }
    out->i = m->forward.i * v->i + m->left.i * v->j + m->up.i * v->k;
    out->j = m->left.j * v->j + m->forward.j * v->i + m->up.j * v->k;
    out->k = m->left.k * v->j + m->forward.k * v->i + m->up.k * v->k;
}

#if 0
Original Ghidra decompilation (0x4cc710):

void matrix3x3_inverse_transform_vector(float *param_1)

{
  float *in_EAX;
  float *in_ECX;
  float local_c;
  float local_8;
  float local_4;

  if (in_ECX == in_EAX) {
    local_c = *in_ECX;
    local_8 = in_ECX[1];
    local_4 = in_ECX[2];
    in_ECX = &local_c;
  }
  *in_EAX = *param_1 * *in_ECX + param_1[3] * in_ECX[1] + param_1[6] * in_ECX[2];
  in_EAX[1] = param_1[4] * in_ECX[1] + param_1[1] * *in_ECX + param_1[7] * in_ECX[2];
  in_EAX[2] = param_1[5] * in_ECX[1] + param_1[2] * *in_ECX + param_1[8] * in_ECX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
