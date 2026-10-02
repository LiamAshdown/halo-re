// cubic_interpolate_divided_difference  (orphan pass 4: FUN_004fca60, no Ghidra name)
// address 0x4fca60, size 147 bytes
// name confidence: 0.55 (the arithmetic is exactly Newton's divided-difference cubic
//   interpolation through 4 (x,y) samples -- p(x) = y0 + f[x0,x1]*(x-x0) +
//   f[x0,x1,x2]*(x-x0)*(x-x1) + f[x0,x1,x2,x3]*(x-x0)*(x-x1)*(x-x2) -- so the name describes
//   the algorithm rather than any Blam-specific identifier; no string or symbol ties it to a
//   Bungie name)
// rewrite confidence: 0.75 (all 9 arguments are plain cdecl stack floats per objdump and per
//   Ghidra's own signature; the only liberty taken is collapsing the x87 float10 intermediates
//   to float/double, matching the established UNSURE convention used elsewhere in this
//   codebase, e.g. src/ai/actor_compute_target_priority_weight.c)
// evidence: out/phase4/objects_types_notes.md, "Not objects-module code": "0x4f06d0 / 0x4fca60
//   / 0x4fcb00 (barycentric and cubic interpolation) are generic helpers with no object in
//   sight. The interpolators belong with types/math.h if that header is ever extended; they
//   are not declared in objects.h." Sole caller is cubic_interpolate_vector3d 0x4fcb00 (this
//   pass, same file), which is itself called from FUN_004fde40 (glow_particle_reposition per
//   src/objects/README.md's misattribution table) to interpolate a glow particle's position
//   along a 4-point control curve.
// register convention: none; every argument is a plain cdecl stack float, confirmed by
//   objdump (progressively higher [esp+N] loads, no register-resident arguments).
// blam-cc: cdecl(y0, y1, y2, y3, x0, x1, x2, x3, x)
// UNSURE: Ghidra's float10 intermediates (the x87 80-bit extended precision the FPU computes
//   divided differences at) are narrowed to double here; the final store back to a float in
//   the caller loses the extra precision either way, matching how float10 x87 return values
//   are handled elsewhere in this codebase (e.g. actor_compute_target_priority_weight.c).

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

float cubic_interpolate_divided_difference(float y0, float y1, float y2, float y3,
                                            float x0, float x1, float x2, float x3, float x)
{
    double d_x1x2 = ((double)y2 - (double)y1) / ((double)x2 - (double)x1);
    double d_x0x1 = ((double)y1 - (double)y0) / ((double)x1 - (double)x0);
    double d_x0x1x2 = (d_x1x2 - d_x0x1) / ((double)x2 - (double)x0);

    double d_x2x3 = ((double)y3 - (double)y2) / ((double)x3 - (double)x2);
    double d_x1x2x3 = (d_x2x3 - d_x1x2) / ((double)x3 - (double)x1);
    double d_x0x1x2x3 = (d_x1x2x3 - d_x0x1x2) / ((double)x3 - (double)x0);

    return (float)(((double)x - (double)x0) *
                    (((double)x - (double)x1) *
                     (((double)x - (double)x2) * d_x0x1x2x3 + d_x0x1x2) + d_x0x1) + (double)y0);
}

#if 0
Original Ghidra decompilation (0x4fca60):

float10 FUN_004fca60(float param_1,float param_2,float param_3,float param_4,float param_5,
                    float param_6,float param_7,float param_8,float param_9)

{
  float fVar1;
  float10 fVar2;
  float10 fVar3;

  fVar2 = ((float10)param_3 - (float10)param_2) / ((float10)param_7 - (float10)param_6);
  fVar1 = (param_2 - param_1) / (param_6 - param_5);
  fVar3 = (fVar2 - (float10)fVar1) / ((float10)param_7 - (float10)param_5);
  return ((float10)param_9 - (float10)param_5) *
         (((float10)param_9 - (float10)param_6) *
          (((float10)param_9 - (float10)param_7) *
           (((float10)(float)((((float10)param_4 - (float10)param_3) /
                               ((float10)param_8 - (float10)param_7) - fVar2) /
                             ((float10)param_8 - (float10)param_6)) - fVar3) /
           ((float10)param_8 - (float10)param_5)) + fVar3) + (float10)fVar1) + (float10)param_1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
