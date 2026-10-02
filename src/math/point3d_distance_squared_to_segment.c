// point3d_distance_squared_to_segment  (Ghidra: point3d_distance_squared_to_segment, already named)
// address 0x4cde30, size 191 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: math_functions.md: "Returns the squared distance from point EDX to the line segment
//   starting at EAX with direction ECX, clamping the projection parameter to [0,1]."
// register convention: segment start pointer in EAX (in_EAX), segment direction pointer in ECX
//   (in_ECX), point pointer in EDX (in_EDX), no stack arguments.
//   // blam-cc: EAX -> segment_start, ECX -> segment_direction, EDX -> point

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

real point3d_distance_squared_to_segment(real_point3d *segment_start, real_vector3d *segment_direction, real_point3d *point)
{
    real t;
    real dx;
    real dy;
    real dz;

    t = ((point->x - segment_start->x) * segment_direction->i +
         (point->y - segment_start->y) * segment_direction->j +
         (point->z - segment_start->z) * segment_direction->k) /
        (segment_direction->k * segment_direction->k + segment_direction->j * segment_direction->j +
         segment_direction->i * segment_direction->i);

    if (t < 0.0f) {
        t = 0.0f;
    } else if (1.0f < t) {
        t = 1.0f;
    }
    t = -t;

    dx = t * segment_direction->i + (point->x - segment_start->x);
    dy = t * segment_direction->j + (point->y - segment_start->y);
    dz = t * segment_direction->k + (point->z - segment_start->z);

    return dx * dx + dy * dy + dz * dz;
}

#if 0
Original Ghidra decompilation (0x4cde30):

float10 point3d_distance_squared_to_segment(void)

{
  float fVar1;
  float fVar2;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;
  float10 fVar3;

  fVar1 = (float)((((float10)*in_EDX - (float10)*in_EAX) * (float10)*in_ECX +
                  (float10)(in_EDX[1] - in_EAX[1]) * (float10)in_ECX[1] +
                  ((float10)in_EDX[2] - (float10)in_EAX[2]) * (float10)in_ECX[2]) /
                 ((float10)in_ECX[2] * (float10)in_ECX[2] +
                 (float10)in_ECX[1] * (float10)in_ECX[1] + (float10)*in_ECX * (float10)*in_ECX));
  if (0.0 <= fVar1) {
    if (fVar1 <= 1.0) {
      fVar3 = (float10)fVar1;
    }
    else {
      fVar3 = (float10)1.0;
    }
  }
  else {
    fVar3 = (float10)0.0;
  }
  fVar3 = -fVar3;
  fVar1 = (float)(fVar3 * (float10)*in_ECX + ((float10)*in_EDX - (float10)*in_EAX));
  fVar2 = (float)(fVar3 * (float10)in_ECX[1] + (float10)(in_EDX[1] - in_EAX[1]));
  fVar3 = fVar3 * (float10)in_ECX[2] + ((float10)in_EDX[2] - (float10)in_EAX[2]);
  return (float10)fVar1 * (float10)fVar1 + (float10)fVar2 * (float10)fVar2 + fVar3 * fVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
