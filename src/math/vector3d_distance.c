// vector3d_distance  (Ghidra: vector3d_distance, already named)
// address 0x4088b0, size 41 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/math_types_notes.md real_vector3d section (touches [0],[1],[2] on both
//   pointers).
// register convention: first point in EAX (in_EAX), second point in ECX (in_ECX).
//   // blam-cc: EAX -> a, ECX -> b

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// sqrt is a single x87 FSQRT instruction in the original code (Ghidra's SQRT() pseudo-function);
// declared locally instead of via <math.h> because -I types shadows that header name.
extern double sqrt(double x);

// Returns the Euclidean distance between two 3D points.
real vector3d_distance(real_point3d *a, real_point3d *b)
{
    return (real)sqrt((double)((a->z - b->z) * (a->z - b->z) +
                                (a->y - b->y) * (a->y - b->y) +
                                (a->x - b->x) * (a->x - b->x)));
}

#if 0
Original Ghidra decompilation (0x4088b0):

float10 vector3d_distance(void)

{
  float *in_EAX;
  float *in_ECX;
  
  return SQRT(((float10)in_EAX[2] - (float10)in_ECX[2]) * ((float10)in_EAX[2] - (float10)in_ECX[2])
              + ((float10)in_EAX[1] - (float10)in_ECX[1]) *
                ((float10)in_EAX[1] - (float10)in_ECX[1]) +
                ((float10)*in_EAX - (float10)*in_ECX) * ((float10)*in_EAX - (float10)*in_ECX));
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
