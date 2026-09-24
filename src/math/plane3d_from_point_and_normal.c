// plane3d_from_point_and_normal  (already named; task-provided per out/phase4/effects_types_notes.md)
// address 0x44d9e0, size 50 bytes
// name confidence: 0.6 (out/phase4/effects_types_notes.md: "0x44d9e0 | math |
//   plane3d_from_point_and_normal"; the body copies a normal and computes d = normal . point,
//   the textbook "plane through point with given normal" construction)
// rewrite confidence: 0.6 (pure arithmetic, confirmed against the decompilation)
// evidence: types/math.h real_plane3d (normal 0x00, d 0x0c), real_vector3d, real_point3d.
// register convention: out on the stack (the incoming param_1, loaded into EAX at entry; orphan
//   pass 4 review, objdump 0x44d9e0 `mov eax,[esp+0x4]`), ECX = const real_vector3d *normal,
//   EDX = const real_point3d *point.
// blam-cc: plane3d_from_point_and_normal(real_plane3d *out /*stack*/, const real_vector3d *normal /*ECX*/,
//   const real_point3d *point /*EDX*/)

#include "tags.h"
#include "math.h"

void plane3d_from_point_and_normal(real_plane3d *out, const real_vector3d *normal, const real_point3d *point)
{
    out->normal.i = normal->i;
    out->normal.j = normal->j;
    out->normal.k = normal->k;
    out->d = out->normal.i * point->x + out->normal.j * point->y + out->normal.k * point->z;
}

#if 0
Original Ghidra decompilation (0x44d9e0):

void FUN_0044d9e0(float *param_1)

{
  float *in_ECX;
  float *in_EDX;

  *param_1 = *in_ECX;
  param_1[1] = in_ECX[1];
  param_1[2] = in_ECX[2];
  param_1[3] = *param_1 * *in_EDX + param_1[1] * in_EDX[1] + param_1[2] * in_EDX[2];
  return;
}
#endif
