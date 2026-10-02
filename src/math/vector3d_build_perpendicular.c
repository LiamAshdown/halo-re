// vector3d_build_perpendicular  (Ghidra: vector3d_build_perpendicular, already named)
// address 0x4cd670, size 137 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: math_functions.md: "Given a direction vector in EDX, writes to ECX an arbitrary
//   vector guaranteed to be perpendicular to it." Picks the smallest-magnitude component of the
//   direction and returns a cross product of the direction with the corresponding axis
//   (X when |x| is smallest, Z when |y| is smallest, Y when |z| is smallest), which is always
//   well-defined and non-zero for a non-zero direction.
// register convention: output vector pointer in ECX (in_ECX), direction vector pointer in EDX
//   (in_EDX), no stack arguments.
//   // blam-cc: ECX -> out, EDX -> dir
//
// The Ghidra idiom `(a < b) != (a == b)` is bit-for-bit `a <= b`, including for NaN (both give
// false). The *other* idiom used below, `(a < b) == (a == b)`, is bit-for-bit `a > b` only for
// non-NaN operands (for NaN it gives true, where a plain `>` would give false); kept as the
// literal boolean expression rather than folded to `>` so NaN direction input (never expected
// in practice) still behaves exactly as decompiled.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double fabs(double x); // ABS is a single x87 FABS instruction

void vector3d_build_perpendicular(real_vector3d *out, real_vector3d *dir)
{
    real abs_i;
    real abs_j;
    real abs_k;

    abs_i = (real)fabs((double)dir->i);
    abs_j = (real)fabs((double)dir->j);
    abs_k = (real)fabs((double)dir->k);

    if ((abs_i < abs_j) != (abs_i == abs_j) && (abs_i < abs_k) != (abs_i == abs_k)) {
        out->i = 0.0f;
        out->j = dir->k;
        out->k = -dir->j;
        return;
    }
    if ((abs_j < abs_k) == (abs_j == abs_k)) {
        out->i = dir->j;
        out->j = -dir->i;
        out->k = 0.0f;
        return;
    }
    out->j = 0.0f;
    out->i = -dir->k;
    out->k = dir->i;
}

#if 0
Original Ghidra decompilation (0x4cd670):

void vector3d_build_perpendicular(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_ECX;
  float *in_EDX;

  fVar1 = ABS(*in_EDX);
  fVar2 = ABS(in_EDX[1]);
  fVar3 = ABS(in_EDX[2]);
  if ((fVar1 < fVar2 != (fVar1 == fVar2)) && (fVar1 < fVar3 != (fVar1 == fVar3))) {
    *in_ECX = 0.0;
    in_ECX[1] = in_EDX[2];
    in_ECX[2] = -in_EDX[1];
    return;
  }
  if (fVar2 < fVar3 == (fVar2 == fVar3)) {
    *in_ECX = in_EDX[1];
    fVar1 = *in_EDX;
    in_ECX[2] = 0.0;
    in_ECX[1] = -fVar1;
    return;
  }
  fVar1 = in_EDX[2];
  in_ECX[1] = 0.0;
  *in_ECX = -fVar1;
  in_ECX[2] = *in_EDX;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
