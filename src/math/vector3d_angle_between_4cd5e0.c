// vector3d_angle_between_4cd5e0  (Ghidra: vector3d_angle_between_4cd5e0, already named)
// address 0x4cd5e0, size 131 bytes
// name confidence: 0.55   rewrite confidence: 0.55
// evidence: math_functions.md: "Returns the angle in radians between two 3D vectors passed in
//   EAX/ECX, clamping the dot product to the valid acos domain." An exact-equality fast path
//   returns 0 without computing acos(1).
// register convention: first vector pointer in EAX (in_EAX), second vector pointer in ECX
//   (in_ECX), no stack arguments.
//   // blam-cc: EAX -> a, ECX -> b
//
// UNSURE: the three acos calls in the original each show up with no visible argument (Ghidra
// lost track of the x87 ST(0) value feeding FUN_00628140 across all three branches); collapsed
// here into one clamp-then-acos, which is what the branch structure and the function's own
// summary describe.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double acos(double x); // 0x00628140, CRT/compiler helper

real vector3d_angle_between_4cd5e0(real_vector3d *a, real_vector3d *b)
{
    real dot;

    if (b->i == a->i && b->j == a->j && b->k == a->k) {
        return 0.0f;
    }

    dot = b->i * a->i + b->j * a->j + b->k * a->k;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }
    return (real)acos((double)dot);
}

#if 0
Original Ghidra decompilation (0x4cd5e0):

float10 vector3d_angle_between_4cd5e0(void)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;
  float10 fVar2;

  if (((*in_ECX == *in_EAX) && (in_ECX[1] == in_EAX[1])) && (in_ECX[2] == in_EAX[2])) {
    return (float10)0.0;
  }
  fVar1 = *in_ECX * *in_EAX + in_ECX[1] * in_EAX[1] + in_ECX[2] * in_EAX[2];
  if (fVar1 < -1.0) {
    fVar2 = (float10)FUN_00628140();
    return fVar2;
  }
  if (1.0 < fVar1) {
    fVar2 = (float10)FUN_00628140();
    return fVar2;
  }
  fVar2 = (float10)FUN_00628140();
  return fVar2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
