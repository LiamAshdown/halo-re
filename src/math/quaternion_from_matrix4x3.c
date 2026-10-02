// quaternion_from_matrix4x3  (Ghidra: quaternion_from_matrix4x3, already named)
// address 0x4cbc00, size 337 bytes
// name confidence: 0.7   rewrite confidence: 0.75
// evidence: types/math.h real_matrix4x3/quaternion-index-table sections ("indexes
//   *(float*)(in_ECX + 4 + (i*3+j)*4), i.e. a 3x3 block starting at byte +4, with the diagonal
//   at +0x04, +0x14, +0x24"; k_quaternion_next_index_matrix4x3 = {1,2,0} at 0x0069665c).
//   Standard Shepperd's-method matrix-to-quaternion conversion: use the trace directly when
//   positive, otherwise pivot on the largest diagonal element.
// register convention: matrix in ECX (in_ECX); output quaternion as the recognized stack
//   parameter (param_1).
//   // blam-cc: ECX -> m, stack -> out

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// sqrt is a single x87 FSQRT instruction in the original code (Ghidra's SQRT() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double sqrt(double x);

extern int16_t k_quaternion_next_index_matrix4x3[3]; // 0x0069665c, {1,2,0}

// Row-major element access into the 3x3 rotation block (forward/left/up rows), matching the
// byte layout the original code addresses directly.
static real m4x3_elem(real_matrix4x3 *m, int row, int col)
{
    real_vector3d *r;
    r = (row == 0) ? &m->forward : (row == 1) ? &m->left : &m->up;
    return (col == 0) ? r->i : (col == 1) ? r->j : r->k;
}

// Extracts a quaternion from the rotation part of a matrix4x3.
void quaternion_from_matrix4x3(real_matrix4x3 *m, real_quaternion *out)
{
    real trace;
    real s;
    int i, j, k;
    real vec[3];

    trace = m4x3_elem(m, 0, 0) + m4x3_elem(m, 1, 1) + m4x3_elem(m, 2, 2);
    if (0.0f < trace) {
        s = (real)sqrt((double)(trace + 1.0f));
        out->w = s * 0.5f;
        s = 0.5f / s;
        out->i = (m4x3_elem(m, 2, 1) - m4x3_elem(m, 1, 2)) * s;
        out->j = (m4x3_elem(m, 0, 2) - m4x3_elem(m, 2, 0)) * s;
        out->k = (m4x3_elem(m, 1, 0) - m4x3_elem(m, 0, 1)) * s;
        return;
    }

    i = (m4x3_elem(m, 0, 0) < m4x3_elem(m, 1, 1)) ? 1 : 0;
    if (m4x3_elem(m, i, i) < m4x3_elem(m, 2, 2)) {
        i = 2;
    }
    j = k_quaternion_next_index_matrix4x3[i];
    k = k_quaternion_next_index_matrix4x3[j];

    s = (real)sqrt((double)((m4x3_elem(m, i, i) - (m4x3_elem(m, j, j) + m4x3_elem(m, k, k))) + 1.0f));
    vec[i] = s * 0.5f;
    if (s != 0.0f) {
        s = 0.5f / s;
    }
    vec[j] = (m4x3_elem(m, i, j) + m4x3_elem(m, j, i)) * s;
    vec[k] = (m4x3_elem(m, i, k) + m4x3_elem(m, k, i)) * s;

    out->i = vec[0];
    out->j = vec[1];
    out->w = (m4x3_elem(m, k, j) - m4x3_elem(m, j, k)) * s;
    out->k = vec[2];
}

#if 0
Original Ghidra decompilation (0x4cbc00):

void quaternion_from_matrix4x3(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  int in_ECX;
  ushort uVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  float local_c [3];

  fVar3 = *(float *)(in_ECX + 4) + *(float *)(in_ECX + 0x14) + *(float *)(in_ECX + 0x24);
  if (0.0 < fVar3) {
    fVar3 = SQRT(fVar3 + 1.0);
    param_1[3] = fVar3 * 0.5;
    fVar3 = 0.5 / fVar3;
    *param_1 = (*(float *)(in_ECX + 0x20) - *(float *)(in_ECX + 0x18)) * fVar3;
    param_1[1] = (*(float *)(in_ECX + 0xc) - *(float *)(in_ECX + 0x1c)) * fVar3;
    param_1[2] = (*(float *)(in_ECX + 0x10) - *(float *)(in_ECX + 8)) * fVar3;
    return;
  }
  bVar4 = *(float *)(in_ECX + 4) < *(float *)(in_ECX + 0x14);
  uVar5 = (ushort)bVar4;
  if (*(float *)((short)(ushort)bVar4 * 0x10 + 4 + in_ECX) < *(float *)(in_ECX + 0x24)) {
    uVar5 = 2;
  }
  iVar7 = (int)(short)uVar5;
  iVar6 = (int)*(short *)(&DAT_0069665c + iVar7 * 2);
  iVar8 = (int)*(short *)(&DAT_0069665c + iVar6 * 2);
  fVar3 = SQRT((*(float *)(iVar7 * 0x10 + 4 + in_ECX) -
               (*(float *)(iVar8 * 0x10 + 4 + in_ECX) + *(float *)(in_ECX + 4 + iVar6 * 0x10))) +
               1.0);
  local_c[iVar7] = fVar3 * 0.5;
  if (fVar3 != 0.0) {
    fVar3 = 0.5 / fVar3;
  }
  local_c[iVar6] =
       (*(float *)(in_ECX + 4 + (iVar7 * 3 + iVar6) * 4) +
       *(float *)(in_ECX + 4 + (iVar6 * 3 + iVar7) * 4)) * fVar3;
  local_c[iVar8] =
       (*(float *)(in_ECX + 4 + (iVar7 * 3 + iVar8) * 4) +
       *(float *)(in_ECX + 4 + (iVar7 + iVar8 * 3) * 4)) * fVar3;
  fVar1 = *(float *)(in_ECX + 4 + (iVar8 * 3 + iVar6) * 4);
  fVar2 = *(float *)(in_ECX + 4 + (iVar6 * 3 + iVar8) * 4);
  *param_1 = local_c[0];
  param_1[1] = local_c[1];
  param_1[3] = (fVar1 - fVar2) * fVar3;
  param_1[2] = local_c[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
