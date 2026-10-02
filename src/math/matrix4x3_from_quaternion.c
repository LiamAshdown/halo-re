// matrix4x3_from_quaternion  (Ghidra: matrix4x3_from_quaternion, already named)
// address 0x4cbad0, size 290 bytes
// name confidence: 0.75   rewrite confidence: 0.8
// evidence: out/phase4/math_functions.md ("Converts a quaternion (ECX) into the rotation part
//   of a matrix4x3 (EDX)"); standard quaternion-to-rotation-matrix expansion with scale=2/|q|^2
//   (0 if q is the zero quaternion), scale=1 and zeroed translation on the output matrix.
// register convention: quaternion in ECX (in_ECX), output matrix in EDX (in_EDX).
//   // blam-cc: ECX -> q, EDX -> out

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Converts a quaternion (ECX) into the rotation part of a matrix4x3 (EDX).
void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out)
{
    real len_sq;
    real s;
    real si, sj, sk;
    real sii, sjj, skk;
    real sij, sik, sjk;
    real siw, sjw, skw;

    len_sq = q->w * q->w + q->k * q->k + q->j * q->j + q->i * q->i;
    s = (len_sq == 0.0f) ? 0.0f : 2.0f / len_sq;

    si = s * q->i;
    sj = s * q->j;
    sk = s * q->k;
    sii = si * q->i;
    sjj = sj * q->j;
    skk = sk * q->k;
    sij = si * q->j;
    sik = si * q->k;
    sjk = sj * q->k;
    siw = si * q->w;
    sjw = sj * q->w;
    skw = sk * q->w;

    out->scale = 1.0f;
    out->position.x = 0.0f;
    out->position.y = 0.0f;
    out->position.z = 0.0f;

    out->forward.i = 1.0f - (sjj + skk);
    out->forward.j = sij - skw;
    out->forward.k = sik + sjw;
    out->left.i = sij + skw;
    out->left.j = 1.0f - (skk + sii);
    out->left.k = sjk - siw;
    out->up.i = sik - sjw;
    out->up.j = sjk + siw;
    out->up.k = 1.0f - (sjj + sii);
}

#if 0
Original Ghidra decompilation (0x4cbad0):

void matrix4x3_from_quaternion(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  float *in_ECX;
  undefined4 *in_EDX;

  fVar3 = in_ECX[3] * in_ECX[3] + in_ECX[2] * in_ECX[2] + in_ECX[1] * in_ECX[1] + *in_ECX * *in_ECX;
  if (fVar3 == 0.0) {
    fVar3 = 0.0;
  }
  else {
    fVar3 = 2.0 / fVar3;
  }
  fVar9 = fVar3 * in_ECX[1];
  fVar8 = fVar3 * in_ECX[2];
  fVar11 = fVar3 * *in_ECX * in_ECX[3];
  fVar1 = in_ECX[3];
  fVar2 = in_ECX[3];
  fVar10 = fVar3 * *in_ECX * *in_ECX;
  fVar3 = *in_ECX;
  fVar4 = *in_ECX;
  fVar5 = in_ECX[1];
  fVar6 = in_ECX[1];
  fVar7 = in_ECX[2];
  *in_EDX = 0x3f800000;
  in_EDX[10] = 0;
  in_EDX[0xb] = 0;
  in_EDX[0xc] = 0;
  in_EDX[1] = 1.0 - (fVar9 * fVar5 + fVar8 * fVar7);
  in_EDX[2] = fVar9 * fVar3 - fVar8 * fVar2;
  in_EDX[3] = fVar8 * fVar4 + fVar9 * fVar1;
  in_EDX[4] = fVar9 * fVar3 + fVar8 * fVar2;
  in_EDX[5] = 1.0 - (fVar8 * fVar7 + fVar10);
  in_EDX[6] = fVar8 * fVar6 - fVar11;
  in_EDX[7] = fVar8 * fVar4 - fVar9 * fVar1;
  in_EDX[8] = fVar8 * fVar6 + fVar11;
  in_EDX[9] = 1.0 - (fVar9 * fVar5 + fVar10);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
