// vector3d_length  (Ghidra: vector3d_length, already named)
// address 0x401960, size 33 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/math_types_notes.md real_vector3d section (touches [0],[1],[2]).
// register convention: vector pointer in EAX (in_EAX), no stack arguments.
//   // blam-cc: EAX -> v

#include "tags.h"
#include "math.h"
#include "fn_math.h"

// sqrt is a single x87 FSQRT instruction in the original code (Ghidra's SQRT() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double sqrt(double x);

// Returns the Euclidean length of a 3D vector.
real vector3d_length(real_vector3d *v)
{
    return (real)sqrt((double)(v->k * v->k + v->j * v->j + v->i * v->i));
}

#if 0
Original Ghidra decompilation (0x401960):

float10 vector3d_length(void)

{
  float *in_EAX;
  
  return SQRT((float10)in_EAX[2] * (float10)in_EAX[2] +
              (float10)in_EAX[1] * (float10)in_EAX[1] + (float10)*in_EAX * (float10)*in_EAX);
}
#endif
