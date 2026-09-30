// plane2d_from_points  (already named; task-provided)
// address 0x44d950, size 135 bytes
// name confidence: 0.7 (already carries this name; already declared and used by
//   src/math/polygon2d_clip_to_planes.c and src/math/polygon2d_points_classify.c, and
//   documented in src/math/README.md: "ECX = out plane, EAX = a, EDX = b: normal =
//   (a.y - b.y, b.x - a.x), normalized, d = normal . b, returning NULL when the points are
//   within 0.0001 of each other")
// rewrite confidence: 0.7 (confirmed against the decompilation and against the two existing
//   callers' extern declaration, which this file matches exactly)
// evidence: src/math/README.md (quoted above); types/math.h real_plane2d, real_point2d.
// register convention: ECX = out_plane, EAX = a, EDX = b (per polygon2d_clip_to_planes.c).
// blam-cc: plane2d_from_points(real_plane2d *out_plane /*ECX*/, const real_point2d *a /*EAX*/, const real_point2d *b /*EDX*/)

#include "tags.h"
#include "math.h"
#include "fn_math.h"

extern double sqrt(double x); // x87 FSQRT

real_plane2d *plane2d_from_points(real_plane2d *out_plane, const real_point2d *a, const real_point2d *b)
{
    float length;

    out_plane->normal.i = a->y - b->y;
    out_plane->normal.j = b->x - a->x;

    length = (float)sqrt((double)(out_plane->normal.j * out_plane->normal.j + out_plane->normal.i * out_plane->normal.i));

    if (length >= 0.0001f) {
        out_plane->normal.i = (1.0f / length) * out_plane->normal.i;
        out_plane->normal.j = (1.0f / length) * out_plane->normal.j;
        if (length != 0.0f) {
            out_plane->d = out_plane->normal.i * b->x + out_plane->normal.j * b->y;
            return out_plane;
        }
    }

    out_plane->d = 0.0f;
    return 0;
}

#if 0
Original Ghidra decompilation (0x44d950):

float * FUN_0044d950(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float *in_EAX;
  float *in_ECX;
  float *in_EDX;

  *in_ECX = in_EAX[1] - in_EDX[1];
  fVar1 = *in_EDX - *in_EAX;
  in_ECX[1] = fVar1;
  fVar1 = SQRT(fVar1 * fVar1 + *in_ECX * *in_ECX);
  if (0.0001 <= ABS(fVar1)) {
    fVar3 = (1.0 / fVar1) * *in_ECX;
    *in_ECX = fVar3;
    fVar2 = (1.0 / fVar1) * in_ECX[1];
    in_ECX[1] = fVar2;
    if (fVar1 != 0.0) {
      in_ECX[2] = fVar3 * *in_EDX + fVar2 * in_EDX[1];
      return in_ECX;
    }
  }
  in_ECX[2] = 0.0;
  return (float *)0x0;
}
#endif
