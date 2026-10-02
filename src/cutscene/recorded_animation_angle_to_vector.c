// recorded_animation_angle_to_vector  (Ghidra: recorded_animation_angle_to_vector, already named)
// address 0x44a190, size 62 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/cutscene_types_notes.md "0x44a190 takes ECX = pair and EAX = float[3],
// with the scale 0x00672dd8 = pi/1000." types/cutscene.h recorded_animation_angles doc:
// "0x44a190 takes the pair in ECX and writes the unit vector (cos yaw cos pitch, sin yaw cos
// pitch, sin pitch) to the float[3] in EAX."
// register convention: EAX = out (in_EAX), ECX = angles (in_ECX). blam-cc: EAX -> out,
// ECX -> angles.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// cos/sin are single x87 FCOS/FSIN instructions in the original code (Ghidra's fcos()/fsin()
// pseudo-calls); declared locally instead of via <math.h> because -I types shadows that header
// name with types/math.h.
extern double cos(double x);
extern double sin(double x);

extern float recorded_animation_angle_scale; // 0x00672dd8, pi / 1000

// blam-cc: EAX -> out, ECX -> angles
// Converts a compressed 1000-unit fixed-point yaw/pitch pair into a normalized 3D direction
// vector (cos yaw cos pitch, sin yaw cos pitch, sin pitch). The output is a real_vector3d
// (not the tag Vector3D) because both of this function's callers write straight into a
// unit_control_data direction vector.
void recorded_animation_angle_to_vector(real_vector3d *out, recorded_animation_angles *angles)
{
    double cos_pitch;
    double cos_yaw;
    double sin_yaw;

    cos_pitch = cos((double)angles->pitch * (double)recorded_animation_angle_scale);
    cos_yaw = cos((double)angles->yaw * (double)recorded_animation_angle_scale);
    out->i = (float)(cos_yaw * cos_pitch);
    sin_yaw = sin((double)angles->yaw * (double)recorded_animation_angle_scale);
    out->j = (float)(sin_yaw * cos_pitch);
    out->k = (float)sin((double)angles->pitch * (double)recorded_animation_angle_scale);
}

#if 0
Original Ghidra decompilation (0x44a190):

void recorded_animation_angle_to_vector(void)

{
  short sVar1;
  short sVar2;
  float *in_EAX;
  short *in_ECX;
  float10 fVar3;
  float10 fVar4;

  sVar1 = *in_ECX;
  sVar2 = in_ECX[1];
  fVar3 = (float10)fcos((float10)(int)sVar2 * (float10)0.0031415927);
  fVar4 = (float10)fcos((float10)(int)sVar1 * (float10)0.0031415927);
  *in_EAX = (float)(fVar4 * fVar3);
  fVar4 = (float10)fsin((float10)(int)sVar1 * (float10)0.0031415927);
  in_EAX[1] = (float)(fVar4 * fVar3);
  fVar3 = (float10)fsin((float10)(int)sVar2 * (float10)0.0031415927);
  in_EAX[2] = (float)fVar3;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
