// vector2d_angle_between  (Ghidra: vector2d_angle_between, already named)
// address 0x4cd480, size 101 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: math_functions.md: "Returns the signed angle between two 2D vectors." Dot product
//   clamped to [-1,1] and passed to acos (FUN_00628140 @0x00628140, a CRT/compiler helper per
//   math_types_notes.md item 7), then negated when the 2D cross product is negative.
// register convention: first vector pointer in ESI (unaff_ESI), second vector pointer in EDI
//   (unaff_EDI), no stack arguments.
//   // blam-cc: ESI -> a, EDI -> b

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double acos(double x); // 0x00628140, CRT/compiler helper

// Returns the signed angle in radians between two 2D vectors: the magnitude is acos of the
// clamped dot product, and the sign follows the 2D cross product (positive = a to b
// counter-clockwise).
real vector2d_angle_between(real_vector2d *a, real_vector2d *b)
{
    real dot;
    real angle;

    dot = a->j * b->j + b->i * a->i;
    if (dot < -1.0f) {
        dot = -1.0f;
    } else if (1.0f < dot) {
        dot = 1.0f;
    }

    angle = (real)acos((double)dot);
    if (a->j * b->i - a->i * b->j < 0.0f) {
        angle = -angle;
    }
    return angle;
}

#if 0
Original Ghidra decompilation (0x4cd480):

float10 vector2d_angle_between(void)

{
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar1;
  float fVar2;

  fVar2 = unaff_ESI[1] * unaff_EDI[1] + *unaff_EDI * *unaff_ESI;
  if (-1.0 <= fVar2) {
    if (1.0 < fVar2) {
      fVar2 = 1.0;
    }
  }
  else {
    fVar2 = -1.0;
  }
  fVar1 = (float10)FUN_00628140(fVar2);
  if (unaff_ESI[1] * *unaff_EDI - *unaff_ESI * unaff_EDI[1] < 0.0) {
    fVar1 = -fVar1;
  }
  return fVar1;
}
#endif
