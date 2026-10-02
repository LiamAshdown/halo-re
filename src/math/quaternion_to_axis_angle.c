// quaternion_to_axis_angle  (Ghidra: quaternion_to_axis_angle, already named)
// address 0x4cdb90, size 94 bytes
// name confidence: 0.85   rewrite confidence: 0.65
// evidence: math_functions.md: "Converts the quaternion at EAX into an axis (written to ESI)
//   and angle in radians (written to *EDI)." Standard axis-angle extraction: axis = normalized
//   vector part, angle = 2*atan2(|v|, w); folded into [0, 2*pi) by flipping the axis when the
//   raw angle exceeds pi.
// register convention: quaternion pointer in EAX (in_EAX), axis output pointer in ESI
//   (unaff_ESI), angle output pointer in EDI (unaff_EDI), no stack arguments.
//   // blam-cc: EAX -> quat, ESI -> axis_out, EDI -> angle_out
//
// UNSURE: `vector3d_normalize_with_length()` shows with no visible argument in the original;
// reconstructed here with axis_out, the only vector it can sensibly be operating on (it was
// just filled from quat's vector part on the two lines above).

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern real vector3d_normalize_with_length(real_vector3d *v); // 0x401990
extern double atan2(double y, double x); // fpatan is a single x87 FPATAN instruction

void quaternion_to_axis_angle(real_quaternion *quat, real_vector3d *axis_out, real *angle_out)
{
    real w;
    real length;
    real half_angle;

    axis_out->i = quat->i;
    axis_out->j = quat->j;
    axis_out->k = quat->k;
    w = quat->w;

    length = vector3d_normalize_with_length(axis_out);
    half_angle = (real)atan2((double)length, (double)w);
    *angle_out = half_angle + half_angle;

    if (3.1415927f < half_angle + half_angle) {
        axis_out->i = -axis_out->i;
        axis_out->j = -axis_out->j;
        axis_out->k = -axis_out->k;
        *angle_out = 6.2831855f - *angle_out;
    }
}

#if 0
Original Ghidra decompilation (0x4cdb90):

void quaternion_to_axis_angle(void)

{
  float fVar1;
  float *in_EAX;
  float *unaff_ESI;
  float *unaff_EDI;
  unkbyte10 Var2;
  float10 fVar3;

  *unaff_ESI = *in_EAX;
  unaff_ESI[1] = in_EAX[1];
  unaff_ESI[2] = in_EAX[2];
  fVar1 = in_EAX[3];
  Var2 = vector3d_normalize_with_length();
  fVar3 = (float10)fpatan(Var2,(float10)fVar1);
  *unaff_EDI = (float)(fVar3 + fVar3);
  if ((float10)3.1415927 < fVar3 + fVar3) {
    *unaff_ESI = -*unaff_ESI;
    unaff_ESI[1] = -unaff_ESI[1];
    unaff_ESI[2] = -unaff_ESI[2];
    *unaff_EDI = 6.2831855 - *unaff_EDI;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
