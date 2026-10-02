// matrix4x3_transform_plane  (Ghidra: matrix4x3_transform_plane, already named)
// address 0x4cbf10, size 102 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/math.h real_plane3d section ("matrix4x3_transform_plane reads in_EDX[3] and
//   writes in_EAX[3], confirming 0x10 for the 3D plane"). Normal is transformed the same way as
//   matrix4x3_transform_normal @0x4cbec0; the new distance is the old distance scaled plus the
//   dot of the matrix position with the new normal.
// register convention: output plane in EAX (in_EAX), matrix in ECX (in_ECX), input plane in EDX
//   (in_EDX).
//   // blam-cc: EAX -> out, ECX -> m, EDX -> plane

#include "tags.h"
#include "math.h"

// Transforms a plane equation (normal+distance) by a matrix4x3.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane)
{
    real i, j, k;

    i = plane->normal.i;
    j = plane->normal.j;
    k = plane->normal.k;
    out->normal.i = i * m->forward.i + j * m->left.i + k * m->up.i;
    out->normal.j = i * m->forward.j + j * m->left.j + k * m->up.j;
    out->normal.k = i * m->forward.k + j * m->left.k + k * m->up.k;
    out->d = m->position.x * out->normal.i + out->normal.k * m->position.z +
             m->position.y * out->normal.j + plane->d * m->scale;
}

#if 0
Original Ghidra decompilation (0x4cbf10):

void matrix4x3_transform_plane(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;

  fVar1 = *in_EDX;
  fVar2 = in_EDX[1];
  fVar3 = in_EDX[2];
  *in_EAX = fVar1 * in_ECX[1] + fVar2 * in_ECX[4] + fVar3 * in_ECX[7];
  in_EAX[1] = fVar1 * in_ECX[2] + fVar2 * in_ECX[5] + fVar3 * in_ECX[8];
  fVar1 = fVar1 * in_ECX[3] + fVar2 * in_ECX[6] + fVar3 * in_ECX[9];
  in_EAX[2] = fVar1;
  in_EAX[3] = in_ECX[10] * *in_EAX +
              fVar1 * in_ECX[0xc] + in_ECX[0xb] * in_EAX[1] + in_EDX[3] * *in_ECX;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
