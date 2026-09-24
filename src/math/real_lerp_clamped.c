// real_lerp_clamped  (Ghidra: real_lerp_clamped, already named)
// address 0x4cd900, size 71 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: math_functions.md: "Linearly interpolates between two scalars and clamps the result
//   to the [0,1] range, writing it through the ECX output pointer."
// register convention: output pointer in ECX (in_ECX); a, b, t as the recognized stack
//   parameters (param_1, param_2, param_3).
//   // blam-cc: ECX -> out, stack -> (a, b, t)

#include "tags.h"
#include "math.h"

// *out = clamp(lerp(a, b, t), 0, 1)
void real_lerp_clamped(real *out, real a, real b, real t)
{
    real value;

    value = b * t + (1.0f - t) * a;
    if (value < 0.0f) {
        *out = 0.0f;
    } else if (1.0f < value) {
        *out = 1.0f;
    } else {
        *out = value;
    }
}

#if 0
Original Ghidra decompilation (0x4cd900):

void real_lerp_clamped(float param_1,float param_2,float param_3)

{
  float fVar1;
  float *in_ECX;

  fVar1 = param_2 * param_3 + (1.0 - param_3) * param_1;
  if (fVar1 < 0.0) {
    *in_ECX = 0.0;
    return;
  }
  if (1.0 < fVar1) {
    *in_ECX = 1.0;
    return;
  }
  *in_ECX = fVar1;
  return;
}
#endif
