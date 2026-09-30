// vector3d_project_onto_axis  (Ghidra: vector3d_project_onto_axis, already named)
// address 0x4cda90, size 143 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: math_functions.md: "Projects the vector at unaff_ESI onto the (arbitrary-length)
//   axis at in_EDX, storing the projection in in_ECX and the perpendicular remainder in
//   unaff_EDI." Unlike vector3d_project_onto_unit_axis @0x4cda30, axis is not assumed unit
//   length, so the projection factor divides by axis-length-squared; a zero-length axis yields
//   a zero parallel component and the whole input vector as the remainder.
// register convention: parallel-component output pointer in ECX (in_ECX), axis pointer in EDX
//   (in_EDX), input vector pointer in ESI (unaff_ESI), perpendicular-remainder output pointer
//   in EDI (unaff_EDI).
//   // blam-cc: ECX -> parallel_out, EDX -> axis, ESI -> v, EDI -> perp_out

#include "tags.h"
#include "math.h"
#include "fn_math.h"

void vector3d_project_onto_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out)
{
    real axis_length_squared;
    real t;

    axis_length_squared = axis->k * axis->k + axis->j * axis->j + axis->i * axis->i;
    if (axis_length_squared != 0.0f) {
        t = (v->j * axis->j + v->i * axis->i + v->k * axis->k) / axis_length_squared;
        parallel_out->i = t * axis->i;
        parallel_out->j = t * axis->j;
        parallel_out->k = t * axis->k;
        perp_out->i = v->i - parallel_out->i;
        perp_out->j = v->j - parallel_out->j;
        perp_out->k = v->k - parallel_out->k;
        return;
    }
    parallel_out->i = 0.0f;
    parallel_out->j = 0.0f;
    parallel_out->k = 0.0f;
    *perp_out = *v;
}

#if 0
Original Ghidra decompilation (0x4cda90):

void vector3d_project_onto_axis(void)

{
  float fVar1;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  float *unaff_EDI;

  fVar1 = in_EDX[2] * in_EDX[2] + in_EDX[1] * in_EDX[1] + *in_EDX * *in_EDX;
  if (fVar1 != 0.0) {
    fVar1 = (unaff_ESI[1] * in_EDX[1] + *unaff_ESI * *in_EDX + unaff_ESI[2] * in_EDX[2]) / fVar1;
    *in_ECX = fVar1 * *in_EDX;
    in_ECX[1] = fVar1 * in_EDX[1];
    in_ECX[2] = fVar1 * in_EDX[2];
    *unaff_EDI = *unaff_ESI - *in_ECX;
    unaff_EDI[1] = unaff_ESI[1] - in_ECX[1];
    unaff_EDI[2] = unaff_ESI[2] - in_ECX[2];
    return;
  }
  *in_ECX = 0.0;
  in_ECX[1] = 0.0;
  in_ECX[2] = 0.0;
  *unaff_EDI = *unaff_ESI;
  unaff_EDI[1] = unaff_ESI[1];
  unaff_EDI[2] = unaff_ESI[2];
  return;
}
#endif
