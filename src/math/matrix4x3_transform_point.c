// matrix4x3_transform_point  (Ghidra: matrix4x3_transform_point, already named)
// address 0x4cbde0, size 109 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: types/math.h real_matrix4x3 section ("out.x = p.i*m[1] + p.j*m[4] + p.k*m[7] +
//   m[10]", the primary evidence for the forward/left/up row layout and the translation at
//   +0x28). Scale is applied only when != 1.0.
// register convention: output point in EAX (in_EAX), input point in EDX (in_EDX); matrix as the
//   recognized stack parameter (param_1).
//   // blam-cc: EAX -> out, EDX -> point, stack -> m

#include "tags.h"
#include "math.h"

// Transforms a point by a matrix4x3 (scale, rotate, translate).
void matrix4x3_transform_point(real_point3d *out, real_point3d *point, real_matrix4x3 *m)
{
    real x, y, z;

    x = point->x;
    y = point->y;
    z = point->z;
    if (m->scale != 1.0f) {
        x = x * m->scale;
        y = y * m->scale;
        z = z * m->scale;
    }
    out->x = x * m->forward.i + y * m->left.i + z * m->up.i + m->position.x;
    out->y = x * m->forward.j + y * m->left.j + z * m->up.j + m->position.y;
    out->z = x * m->forward.k + y * m->left.k + z * m->up.k + m->position.z;
}

#if 0
Original Ghidra decompilation (0x4cbde0):

void matrix4x3_transform_point(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_EDX;

  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  if (*param_1 != 1.0) {
    fVar1 = fVar1 * *param_1;
    fVar2 = fVar2 * *param_1;
    fVar3 = fVar3 * *param_1;
  }
  *in_EAX = fVar1 * param_1[1] + fVar2 * param_1[4] + fVar3 * param_1[7] + param_1[10];
  in_EAX[1] = fVar1 * param_1[2] + fVar2 * param_1[5] + fVar3 * param_1[8] + param_1[0xb];
  in_EAX[2] = fVar1 * param_1[3] + fVar2 * param_1[6] + fVar3 * param_1[9] + param_1[0xc];
  return;
}
#endif
