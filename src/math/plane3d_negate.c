// plane3d_negate  (already named; task-provided)
// address 0x44da20, size 31 bytes
// name confidence: 0.6 (already carries this name; negates all four plane components in place)
// rewrite confidence: 0.6 (trivial arithmetic, confirmed against the decompilation)
// evidence: types/math.h real_plane3d (normal 0x00, d 0x0c). out/phase4/effects_types_notes.md
//   "0x44da20 plane3d_negate | math | generic".
// register convention: EAX = real_plane3d *out, ECX = const real_plane3d *in.
// blam-cc: plane3d_negate(real_plane3d *out /*EAX*/, const real_plane3d *in /*ECX*/)

#include "tags.h"
#include "math.h"

void plane3d_negate(real_plane3d *out, const real_plane3d *in)
{
    out->normal.i = -in->normal.i;
    out->normal.j = -in->normal.j;
    out->normal.k = -in->normal.k;
    out->d = -in->d;
}

#if 0
Original Ghidra decompilation (0x44da20):

void plane3d_negate(void)

{
  float *in_EAX;
  float *in_ECX;

  *in_EAX = -*in_ECX;
  in_EAX[1] = -in_ECX[1];
  in_EAX[2] = -in_ECX[2];
  in_EAX[3] = -in_ECX[3];
  return;
}
#endif
