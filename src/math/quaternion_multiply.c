// quaternion_multiply  (Ghidra: quaternion_multiply, already named)
// address 0x4cdbf0, size 202 bytes
// name confidence: 0.85   rewrite confidence: 0.75
// evidence: math_functions.md: "Multiplies two quaternions (ECX * EAX order per the formula)
//   and writes the result to EDX, safely handling operand/destination aliasing." Standard
//   Hamilton product; b (ECX) is the left/outer factor in the formula, a (EAX) the right/inner
//   factor. When an operand pointer equals the destination pointer, that operand is copied to a
//   local before any component of *out is written.
// register convention: first operand pointer in EAX (in_EAX), second operand pointer in ECX
//   (in_ECX), destination pointer in EDX (in_EDX), no stack arguments.
//   // blam-cc: EAX -> a, ECX -> b, EDX -> out

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// out = b * a  (quaternion Hamilton product, b outer/left, a inner/right)
void quaternion_multiply(real_quaternion *a, real_quaternion *b, real_quaternion *out)
{
    real_quaternion local;
    const real_quaternion *pa;
    const real_quaternion *pb;

    pa = a;
    pb = b;
    if (b == out) {
        local = *b;
        pb = &local;
    }
    if (a == out) {
        local = *a;
        pa = &local;
    }

    out->i = (pb->w * pa->i + pa->k * pb->j + pa->w * pb->i) - pa->j * pb->k;
    out->j = (pb->w * pa->j + pa->w * pb->j + pb->k * pa->i) - pb->i * pa->k;
    out->k = (pb->w * pa->k + pa->j * pb->i + pa->w * pb->k) - pa->i * pb->j;
    out->w = ((pa->w * pb->w - pb->i * pa->i) - pa->j * pb->j) - pb->k * pa->k;
}

#if 0
Original Ghidra decompilation (0x4cdbf0):

void quaternion_multiply(void)

{
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float local_10;
  float local_c;
  float local_8;
  float local_4;

  if (in_ECX == in_EDX) {
    local_10 = *in_ECX;
    local_c = in_ECX[1];
    local_4 = in_ECX[3];
    local_8 = in_ECX[2];
    in_ECX = &local_10;
  }
  if (in_EAX == in_EDX) {
    local_10 = *in_EAX;
    local_c = in_EAX[1];
    local_4 = in_EAX[3];
    local_8 = in_EAX[2];
    in_EAX = &local_10;
  }
  *in_EDX = (in_ECX[3] * *in_EAX + in_EAX[2] * in_ECX[1] + in_EAX[3] * *in_ECX) -
            in_EAX[1] * in_ECX[2];
  in_EDX[1] = (in_ECX[3] * in_EAX[1] + in_EAX[3] * in_ECX[1] + in_ECX[2] * *in_EAX) -
              *in_ECX * in_EAX[2];
  in_EDX[2] = (in_ECX[3] * in_EAX[2] + in_EAX[1] * *in_ECX + in_EAX[3] * in_ECX[2]) -
              *in_EAX * in_ECX[1];
  in_EDX[3] = ((in_EAX[3] * in_ECX[3] - *in_ECX * *in_EAX) - in_EAX[1] * in_ECX[1]) -
              in_ECX[2] * in_EAX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
