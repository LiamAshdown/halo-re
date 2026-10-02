// vector3d_from_yaw_pitch  (Ghidra: vector3d_from_yaw_pitch, already named)
// address 0x4ea7d0, size 221 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md ("Converts a yaw/pitch angle pair into a
// normalized 3D direction vector, snapping near-zero axis components to zero."); types/math.h
// real_vector3d.
// register convention: ECX -> out_direction; stack -> yaw, pitch.
//   // blam-cc: ECX -> out_direction, stack -> yaw, pitch
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double sin(double x); // FSIN
extern double cos(double x); // FCOS

// Computes a direction vector from yaw/pitch: x = cos(pitch)*sin(yaw), y = sin(pitch)*sin(yaw),
// z = cos(yaw) -- an unusual pairing (z from yaw's cosine, not pitch's), transcribed exactly as
// Ghidra shows it rather than the more common spherical-to-cartesian convention.
void vector3d_from_yaw_pitch(real_vector3d *out_direction, real yaw, real pitch)
    // blam-cc: ECX -> out_direction, stack -> yaw, pitch
{
    real sin_yaw, sin_pitch, cos_yaw, cos_pitch;

    sin_yaw = (real)sin((double)yaw);
    sin_pitch = (real)sin((double)pitch);
    cos_yaw = (real)cos((double)yaw);
    cos_pitch = (real)cos((double)pitch);

    if (sin_yaw < 0.0001f && -0.0001f < sin_yaw) {
        sin_yaw = 0.0f;
    }
    if (sin_pitch < 0.0001f && -0.0001f < sin_pitch) {
        sin_pitch = 0.0f;
    }
    if (cos_yaw < 0.0001f && -0.0001f < cos_yaw) {
        cos_yaw = 0.0f;
    }
    if (cos_pitch < 0.0001f && -0.0001f < cos_pitch) {
        cos_pitch = 0.0f;
    }

    out_direction->i = cos_pitch * sin_yaw;
    out_direction->j = sin_pitch * sin_yaw;
    out_direction->k = cos_yaw;
}

#if 0
Original Ghidra decompilation (0x4ea7d0), from tools/pack.py 0x4ea7d0:

void vector3d_from_yaw_pitch(float param_1,float param_2)

{
  float *in_ECX;
  float10 fVar1;
  float10 fVar2;
  float local_8;
  float local_4;

  fVar1 = (float10)fsin((float10)param_1);
  local_8 = (float)fVar1;
  fVar1 = (float10)fsin((float10)param_2);
  local_4 = (float)fVar1;
  fVar2 = (float10)fcos((float10)param_1);
  fVar1 = (float10)fcos((float10)param_2);
  param_1 = (float)fVar1;
  if ((local_8 < 0.0001) && (-0.0001 < local_8)) {
    local_8 = 0.0;
  }
  if ((local_4 < 0.0001) && (-0.0001 < local_4)) {
    local_4 = 0.0;
  }
  if ((fVar2 < (float10)0.0001) && ((float10)-0.0001 < fVar2)) {
    fVar2 = (float10)0.0;
  }
  if ((param_1 < 0.0001) && (-0.0001 < param_1)) {
    param_1 = 0.0;
  }
  *in_ECX = param_1 * local_8;
  in_ECX[1] = local_4 * local_8;
  in_ECX[2] = (float)fVar2;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
