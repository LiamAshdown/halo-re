// quaternion_from_matrix3x3  (Ghidra: quaternion_from_matrix3x3, already named)
// address 0x4cc780, size 327 bytes
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: types/math.h real_matrix3x3 section ("quaternion_from_matrix3x3 indexes
//   in_ECX[i*3+j] and reads the diagonal at [0], [4], [8] -- row major, three rows of three");
//   k_quaternion_next_index_matrix3x3 = {1,2,0} at 0x00696668. Same Shepperd's-method
//   conversion as quaternion_from_matrix4x3 @0x4cbc00, without the +4-byte scale offset.
// register convention: matrix in ECX (in_ECX); output quaternion as the recognized stack
//   parameter (param_1), also returned unchanged (matching the decompile's `float *` return).
//   // blam-cc: ECX -> m, stack -> out

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// sqrt is a single x87 FSQRT instruction in the original code (Ghidra's SQRT() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double sqrt(double x);

extern int16_t k_quaternion_next_index_matrix3x3[3]; // 0x00696668, {1,2,0}

// Row-major element access into the 3x3 matrix, matching the byte layout the original code
// addresses directly.
static real m3x3_elem(real_matrix3x3 *m, int row, int col)
{
    real_vector3d *r;
    r = (row == 0) ? &m->forward : (row == 1) ? &m->left : &m->up;
    return (col == 0) ? r->i : (col == 1) ? r->j : r->k;
}

// Extracts a quaternion from a 3x3 rotation matrix.
real_quaternion *quaternion_from_matrix3x3(real_matrix3x3 *m, real_quaternion *out)
{
    real trace;
    real s;
    int i, j, k;
    real vec[3];

    trace = m3x3_elem(m, 1, 1) + m3x3_elem(m, 0, 0) + m3x3_elem(m, 2, 2);
    if (0.0f < trace) {
        s = (real)sqrt((double)(trace + 1.0f));
        out->w = s * 0.5f;
        s = 0.5f / s;
        out->i = (m3x3_elem(m, 2, 1) - m3x3_elem(m, 1, 2)) * s;
        out->j = (m3x3_elem(m, 0, 2) - m3x3_elem(m, 2, 0)) * s;
        out->k = (m3x3_elem(m, 1, 0) - m3x3_elem(m, 0, 1)) * s;
        return out;
    }

    i = (m3x3_elem(m, 0, 0) < m3x3_elem(m, 1, 1)) ? 1 : 0;
    if (m3x3_elem(m, i, i) < m3x3_elem(m, 2, 2)) {
        i = 2;
    }
    j = k_quaternion_next_index_matrix3x3[i];
    k = k_quaternion_next_index_matrix3x3[j];

    s = (real)sqrt((double)((m3x3_elem(m, i, i) - (m3x3_elem(m, j, j) + m3x3_elem(m, k, k))) + 1.0f));
    vec[i] = s * 0.5f;
    if (s != 0.0f) {
        s = 0.5f / s;
    }
    vec[j] = (m3x3_elem(m, i, j) + m3x3_elem(m, j, i)) * s;
    vec[k] = (m3x3_elem(m, i, k) + m3x3_elem(m, k, i)) * s;

    out->i = vec[0];
    out->j = vec[1];
    out->w = (m3x3_elem(m, k, j) - m3x3_elem(m, j, k)) * s;
    out->k = vec[2];
    return out;
}

#if 0
Original Ghidra decompilation (0x4cc780):

float * quaternion_from_matrix3x3(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_ECX;
  ushort uVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  float local_c [3];

  fVar3 = in_ECX[4] + *in_ECX + in_ECX[8];
  if (0.0 < fVar3) {
    fVar3 = SQRT(fVar3 + 1.0);
    param_1[3] = fVar3 * 0.5;
    fVar3 = 0.5 / fVar3;
    *param_1 = (in_ECX[7] - in_ECX[5]) * fVar3;
    param_1[1] = (in_ECX[2] - in_ECX[6]) * fVar3;
    param_1[2] = (in_ECX[3] - in_ECX[1]) * fVar3;
    return param_1;
  }
  uVar4 = (ushort)(*in_ECX < in_ECX[4]);
  if (in_ECX[(short)(ushort)(*in_ECX < in_ECX[4]) * 4] < in_ECX[8]) {
    uVar4 = 2;
  }
  iVar6 = (int)(short)uVar4;
  iVar5 = (int)*(short *)(&DAT_00696668 + iVar6 * 2);
  iVar7 = (int)*(short *)(&DAT_00696668 + iVar5 * 2);
  fVar3 = SQRT((in_ECX[iVar6 * 4] - (in_ECX[iVar7 * 4] + in_ECX[iVar5 * 4])) + 1.0);
  local_c[iVar6] = fVar3 * 0.5;
  if (fVar3 != 0.0) {
    fVar3 = 0.5 / fVar3;
  }
  local_c[iVar5] = (in_ECX[iVar6 * 3 + iVar5] + in_ECX[iVar5 * 3 + iVar6]) * fVar3;
  local_c[iVar7] = (in_ECX[iVar6 * 3 + iVar7] + in_ECX[iVar6 + iVar7 * 3]) * fVar3;
  fVar1 = in_ECX[iVar7 * 3 + iVar5];
  fVar2 = in_ECX[iVar5 * 3 + iVar7];
  *param_1 = local_c[0];
  param_1[1] = local_c[1];
  param_1[3] = (fVar1 - fVar2) * fVar3;
  param_1[2] = local_c[2];
  return param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
