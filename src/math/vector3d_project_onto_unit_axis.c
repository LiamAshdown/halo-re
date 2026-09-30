// vector3d_project_onto_unit_axis  (Ghidra: vector3d_project_onto_unit_axis, already named)
// address 0x4cda30, size 82 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: math_functions.md: "Splits vector EDX into components parallel and perpendicular to
//   a (caller-guaranteed unit) axis ECX, writing the parallel part to EAX and optionally the
//   perpendicular remainder to ESI." Both output pointers are optional: a NULL EAX falls back
//   to a local scratch (the parallel component is still needed to compute the perpendicular
//   remainder even if the caller doesn't want it); a NULL ESI just skips the remainder.
// register convention: parallel-component output pointer in EAX (in_EAX, nullable), axis
//   pointer in ECX (in_ECX, assumed unit length), input vector pointer in EDX (in_EDX),
//   perpendicular-remainder output pointer in ESI (unaff_ESI, nullable).
//   // blam-cc: EAX -> parallel_out, ECX -> axis, EDX -> v, ESI -> perp_out

#include "tags.h"
#include "math.h"
#include "fn_math.h"

void vector3d_project_onto_unit_axis(real_vector3d *parallel_out, real_vector3d *axis, real_vector3d *v, real_vector3d *perp_out)
{
    real dot;
    real_vector3d local_parallel;
    real_vector3d *parallel;

    dot = v->i * axis->i + axis->k * v->k + v->j * axis->j;

    parallel = parallel_out;
    if (parallel == 0) {
        parallel = &local_parallel;
    }
    parallel->i = dot * axis->i;
    parallel->j = dot * axis->j;
    parallel->k = dot * axis->k;

    if (perp_out != 0) {
        perp_out->i = v->i - parallel->i;
        perp_out->j = v->j - parallel->j;
        perp_out->k = v->k - parallel->k;
    }
}

#if 0
Original Ghidra decompilation (0x4cda30):

void vector3d_project_onto_unit_axis(void)

{
  float fVar1;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;
  float local_c [3];

  fVar1 = *in_EDX * *in_ECX + in_ECX[2] * in_EDX[2] + in_EDX[1] * in_ECX[1];
  if (in_EAX == (float *)0x0) {
    in_EAX = local_c;
  }
  *in_EAX = fVar1 * *in_ECX;
  in_EAX[1] = fVar1 * in_ECX[1];
  in_EAX[2] = fVar1 * in_ECX[2];
  if (unaff_ESI != (float *)0x0) {
    *unaff_ESI = *in_EDX - *in_EAX;
    unaff_ESI[1] = in_EDX[1] - in_EAX[1];
    unaff_ESI[2] = in_EDX[2] - in_EAX[2];
  }
  return;
}
#endif
