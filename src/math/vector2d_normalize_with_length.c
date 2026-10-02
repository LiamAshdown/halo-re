// vector2d_normalize_with_length  (Ghidra: vector2d_normalize_with_length, already named)
// address 0x4018e0, size 68 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/math_types_notes.md real_vector2d section; touches exactly [0] and [1].
//   Tolerance 0.0001f documented at the top of types/math.h.
// register convention: vector pointer in ECX (in_ECX), no stack arguments.
//   // blam-cc: ECX -> v

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// sqrt/fabs are single x87 instructions (FSQRT/FABS) in the original code, which Ghidra
// renders as the pseudo-functions SQRT()/ABS(); declared locally instead of via <math.h>
// because -I types shadows that header name with types/math.h.
extern double sqrt(double x);
extern double fabs(double x);

// Normalizes a 2D vector in place and returns its original length, leaving it unchanged if the
// length is near zero.
real vector2d_normalize_with_length(real_vector2d *v)
{
    real length;
    real inv_length;

    length = (real)sqrt((double)(v->j * v->j + v->i * v->i));
    if (0.0001f <= (real)fabs((double)length)) {
        inv_length = 1.0f / length;
        v->i = inv_length * v->i;
        v->j = inv_length * v->j;
        return length;
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x4018e0):

float10 vector2d_normalize_with_length(void)

{
  float *in_ECX;
  float10 fVar1;
  
  fVar1 = SQRT((float10)in_ECX[1] * (float10)in_ECX[1] + (float10)*in_ECX * (float10)*in_ECX);
  if ((float10)9.999999747378752e-05 <= ABS(fVar1)) {
    *in_ECX = (float)(((float10)1.0 / fVar1) * (float10)*in_ECX);
    in_ECX[1] = (float)(((float10)1.0 / fVar1) * (float10)in_ECX[1]);
    return fVar1;
  }
  return (float10)0.0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
