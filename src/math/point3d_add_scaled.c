// point3d_add_scaled  (Ghidra: point3d_add_scaled, already named)
// address 0x401930, size 41 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md ("out = base + scale*direction"); body writes
//   *in_EAX = param_2 * *in_ECX + *param_1 etc, matching a 3-component point offset.
// register convention: out pointer in EAX, direction pointer in ECX, base pointer and scale
//   on the stack (param_1, param_2).
//   // blam-cc: EAX -> out, ECX -> direction, stack -> (base, scale)

#include "tags.h"
#include "math.h"

// Computes out = base + scale*direction for a 3-component vector (a point offset along a
// direction by a scalar distance).
void point3d_add_scaled(real_point3d *out, real_vector3d *direction, real_point3d *base, real scale)
{
    out->x = scale * direction->i + base->x;
    out->y = scale * direction->j + base->y;
    out->z = scale * direction->k + base->z;
}

#if 0
Original Ghidra decompilation (0x401930):

void point3d_add_scaled(float *param_1,float param_2)

{
  float *in_EAX;
  float *in_ECX;
  
  *in_EAX = param_2 * *in_ECX + *param_1;
  in_EAX[1] = param_2 * in_ECX[1] + param_1[1];
  in_EAX[2] = param_2 * in_ECX[2] + param_1[2];
  return;
}
#endif
