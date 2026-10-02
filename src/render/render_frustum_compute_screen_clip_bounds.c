// render_frustum_compute_screen_clip_bounds  (Ghidra: FUN_0050ddc0; new name, evidence below)
// address 0x50ddc0, size 99 bytes
// name confidence: 0.45   rewrite confidence: 0.7
// evidence: out/phase4/render_functions.md's phase-2 summary: "Computes the normalized (-1..1)
//   screen-space clip rectangle bounds for the current viewport/projection." The four fields read
//   (+0x144, +0x158, +0x164, +0x168) are render_frustum.projection[0][0], [1][1], [2][0] and
//   [2][1] (types/rasterizer.h; the 4x4 matrix is row-major from +0x144). This same function is
//   already declared (still under its old name) as a dependency in
//   src/structures/structure_bsp_mirror_query.c, with the note "render module, below this batch's
//   assigned range" and the identical blam-cc mapping (EAX=out, ECX=camera/frustum) confirmed
//   there from disassembly at 0x5536ba..0x5536e1; that file is outside this module and is not
//   updated to the new name here.
// register convention: EAX = out (float[4]), ECX = frustum (render_frustum*).
//   // blam-cc: EAX=out, ECX=frustum

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Computes the normalized (-1..1) screen-space clip rectangle (left, right, bottom, top) implied
// by the frustum's current projection matrix, from its two diagonal scale terms and its
// (asymmetric) perspective shear terms.
void render_frustum_compute_screen_clip_bounds(float out[4], render_frustum *frustum) // blam-cc: EAX=out, ECX=frustum
{
    float shear_x = frustum->projection[2][0];
    float shear_y = frustum->projection[2][1];
    float inverse_scale_x = -1.0f / frustum->projection[0][0];
    float inverse_scale_y = -1.0f / frustum->projection[1][1];

    out[0] = (-shear_x - 1.0f) * inverse_scale_x;
    out[1] = (-shear_x + 1.0f) * inverse_scale_x;
    out[2] = (-shear_y - 1.0f) * inverse_scale_y;
    out[3] = (-shear_y + 1.0f) * inverse_scale_y;
}

#if 0
Original Ghidra decompilation (0x50ddc0):

void FUN_0050ddc0(void)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float *in_EAX;
  int in_ECX;

  fVar1 = *(float *)(in_ECX + 0x164);
  fVar2 = *(float *)(in_ECX + 0x168);
  fVar3 = -1.0 / *(float *)(in_ECX + 0x144);
  fVar4 = -1.0 / *(float *)(in_ECX + 0x158);
  *in_EAX = (-fVar1 - 1.0) * fVar3;
  in_EAX[1] = (-fVar1 + 1.0) * fVar3;
  in_EAX[2] = (-fVar2 - 1.0) * fVar4;
  in_EAX[3] = (-fVar2 + 1.0) * fVar4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
