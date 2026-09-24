// matrix4x3_inverse_transform_point  (Ghidra: matrix4x3_inverse_transform_point, already named)
// address 0x4cbf80, size 140 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: out/phase4/math_functions.md ("Transforms a point from world space into a
//   matrix4x3's local space (inverse transform) without computing a full inverse matrix");
//   subtracts the translation, undoes the scale, then dots with each rotation row (the
//   transpose of the forward/left/up basis, which is the inverse for an orthonormal rotation).
// register convention: matrix in ECX (in_ECX), output point in EDX (in_EDX), input point in ESI
//   (unaff_ESI).
//   // blam-cc: ECX -> m, EDX -> out, ESI -> point

#include "tags.h"
#include "math.h"

// Transforms a point from world space into a matrix4x3's local space (inverse transform)
// without computing a full inverse matrix.
void matrix4x3_inverse_transform_point(real_matrix4x3 *m, real_point3d *out, real_point3d *point)
{
    real dx, dy, dz;
    real inv_scale;

    if (m->scale != 0.0f) {
        dx = point->x - m->position.x;
        dy = point->y - m->position.y;
        dz = point->z - m->position.z;
        if (m->scale != 1.0f) {
            inv_scale = 1.0f / m->scale;
            dx = inv_scale * dx;
            dy = inv_scale * dy;
            dz = inv_scale * dz;
        }
        out->x = dx * m->forward.i + dy * m->forward.j + dz * m->forward.k;
        out->y = dx * m->left.i + dy * m->left.j + dz * m->left.k;
        out->z = dx * m->up.i + dy * m->up.j + dz * m->up.k;
        return;
    }
    out->x = 0.0f;
    out->y = 0.0f;
    out->z = 0.0f;
}

#if 0
Original Ghidra decompilation (0x4cbf80):

void matrix4x3_inverse_transform_point(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_ECX;
  float *in_EDX;
  float *unaff_ESI;

  if (*in_ECX != 0.0) {
    fVar1 = *unaff_ESI - in_ECX[10];
    fVar4 = unaff_ESI[1] - in_ECX[0xb];
    fVar3 = unaff_ESI[2] - in_ECX[0xc];
    if (*in_ECX != 1.0) {
      fVar2 = 1.0 / *in_ECX;
      fVar1 = fVar2 * fVar1;
      fVar4 = fVar2 * fVar4;
      fVar3 = fVar2 * fVar3;
    }
    *in_EDX = fVar1 * in_ECX[1] + fVar4 * in_ECX[2] + fVar3 * in_ECX[3];
    in_EDX[1] = fVar1 * in_ECX[4] + fVar4 * in_ECX[5] + fVar3 * in_ECX[6];
    in_EDX[2] = fVar1 * in_ECX[7] + fVar4 * in_ECX[8] + fVar3 * in_ECX[9];
    return;
  }
  *in_EDX = 0.0;
  in_EDX[1] = 0.0;
  in_EDX[2] = 0.0;
  return;
}
#endif
