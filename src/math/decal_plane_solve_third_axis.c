// decal_plane_solve_third_axis  (already named; task-provided)
// address 0x44d860, size 113 bytes
// name confidence: 0.6 (already carries this name; src/effects/README.md: "back-solves the
//   dominant coordinate of a point from a plane and the other two, through
//   k_projection_axes")
// rewrite confidence: 0.6 (confirmed against the decompilation; register convention inherited
//   from src/effects/README.md, which notes this function is "called with every register
//   argument elided" from its own callers, so those aren't independently re-verified here)
// evidence: types/math.h projection_axis_pair, k_projection_axes (0x0065c29c, indexed
//   `[dominant_axis*2 + (component > 0)]`), real_plane3d (normal 0x00, d 0x0c).
//   out/phase4/effects_types_notes.md: "0x44d860 | math | back-solves the third coordinate
//   through k_projection_axes, which math.h already owns". Moved to src/math alongside
//   plane2d_from_points and vector3d_scalar_triple_product (same pass), which the notes flag
//   with the identical "not in this module" verdict.
// register convention: EAX = component_sign (0 or 1, `(component > 0)`), ESI = dominant_axis
//   (0, 1 or 2), EBX = const real_plane3d *plane, EDI = const real_point2d *known (the two
//   surviving-axis values), output real_point3d *out in the param passed and returned unchanged
//   (the function both writes through it and returns it in EAX per its own `return param_1`).
// blam-cc: decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign /*EAX*/,
//   int32_t dominant_axis /*ESI*/, const real_plane3d *plane /*EBX*/, const real_point2d *known /*EDI*/)
// UNSURE: register roles for component_sign/dominant_axis/plane/known are inherited from
//   src/effects/README.md's note that every caller elides them; not independently confirmed by
//   this pass against a specific call site.

#include "tags.h"
#include "math.h"

extern double fabs(double x); // ABS is a single x87 FABS instruction
extern projection_axis_pair k_projection_axes[6]; // 0x0065c29c

real_point3d *decal_plane_solve_third_axis(real_point3d *out, uint32_t component_sign, int32_t dominant_axis,
                                            const real_plane3d *plane, const real_point2d *known)
{
    const projection_axis_pair *axes = &k_projection_axes[dominant_axis * 2 + (component_sign & 0xff)];
    float *out_f = (float *)out;
    const float *plane_f = (const float *)plane;

    out_f[axes->i] = known->x;
    out_f[axes->j] = known->y;

    if ((real)fabs((double)plane_f[dominant_axis]) < 0.0001f) {
        out_f[dominant_axis] = 0.0f;
    } else {
        out_f[dominant_axis] = (plane->d - plane_f[axes->i] * known->x - plane_f[axes->j] * known->y) / plane_f[dominant_axis];
    }

    return out;
}

#if 0
Original Ghidra decompilation (0x44d860):

int FUN_0044d860(int param_1)

{
  short sVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  int unaff_EBX;
  short unaff_SI;
  int iVar4;
  float *unaff_EDI;

  iVar4 = (int)unaff_SI;
  iVar3 = ((in_EAX & 0xff) + iVar4 * 2) * 4;
  sVar1 = *(short *)(&DAT_0065c29c + iVar3);
  sVar2 = *(short *)(&DAT_0065c29e + iVar3);
  *(float *)(sVar1 * 4 + param_1) = *unaff_EDI;
  *(float *)(sVar2 * 4 + param_1) = unaff_EDI[1];
  if (ABS(*(float *)(unaff_EBX + iVar4 * 4)) < 0.0001) {
    *(undefined4 *)(param_1 + iVar4 * 4) = 0;
    return param_1;
  }
  *(float *)(param_1 + iVar4 * 4) =
       ((*(float *)(unaff_EBX + 0xc) - *(float *)(sVar1 * 4 + unaff_EBX) * *unaff_EDI) -
       *(float *)(sVar2 * 4 + unaff_EBX) * unaff_EDI[1]) / *(float *)(unaff_EBX + iVar4 * 4);
  return param_1;
}
#endif
