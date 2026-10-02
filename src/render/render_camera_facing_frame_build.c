// render_camera_facing_frame_build  (orphan pass 4: FUN_00458990, no Ghidra name)
// address 0x458990, size 250 bytes
// name confidence: 0.3 (out/phase4/effects_types_notes.md: "builds a camera-facing matrix
//   from the render globals at 0x007c3114..0x007c321c; no effects state at all". The output is
//   really a facing plane (a direction vector plus a dot-product d, i.e. a real_plane3d) packed
//   at the end of a 16-float block copied verbatim from a second render global table, not a
//   single 4x3/4x4 matrix.)
// rewrite confidence: 0.35 (arithmetic and copy confirmed against the decompilation; the
//   output struct's real name/shape and the two render-global tables' meanings are not
//   established -- see UNSURE)
// evidence: out/phase4/effects_types_notes.md "0x458990 | render | ...". Moved to src/render
//   per that note.
// register convention: ECX = out (a 0x50-byte struct: 16 floats at 0x00 copied from
//   render_camera_facing_basis, then a real_plane3d-shaped {normal, d} at 0x40), stack
//   argument = float distance.
// blam-cc: render_camera_facing_frame_build(void *out /*ECX*/, float distance /*stack*/)
// UNSURE: render_camera_global (0x007c3114) and camera_forward_x (0x007c3120) are
//   guessed names for a 3-float position and a 3-float direction; render_camera_facing_basis
//   (0x007c31e0) is a 16-float block whose real shape (4x4 matrix? camera frustum corners?) is
//   not established. The output struct is likewise not independently named/typed in
//   types/rasterizer.h or types/render.h.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern float render_camera_global[3]; // 0x007c3114, UNSURE: guessed name
extern float camera_forward_x[3];  // 0x007c3120, UNSURE: guessed name
extern float render_camera_facing_basis[16]; // 0x007c31e0, UNSURE: guessed name/shape

void render_camera_facing_frame_build(float *out, float distance)
{
    float px = camera_forward_x[0] * distance + render_camera_global[0];
    float py = camera_forward_x[1] * distance + render_camera_global[1];
    float pz = camera_forward_x[2] * distance + render_camera_global[2];
    int32_t i;

    out[0x10] = camera_forward_x[0];
    out[0x11] = camera_forward_x[1];
    out[0x12] = camera_forward_x[2];
    out[0x13] = px * out[0x10] + py * out[0x11] + pz * out[0x12];

    for (i = 0; i < 16; i++) {
        out[i] = render_camera_facing_basis[i];
    }
}

#if 0
Original Ghidra decompilation (0x458990):

void FUN_00458990(float param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  undefined4 *in_ECX;

  fVar1 = DAT_007c3120 * param_1 + DAT_007c3114;
  fVar3 = DAT_007c3124 * param_1 + DAT_007c3118;
  fVar2 = DAT_007c3128 * param_1 + DAT_007c311c;
  in_ECX[0x10] = DAT_007c3120;
  in_ECX[0x11] = DAT_007c3124;
  in_ECX[0x12] = DAT_007c3128;
  in_ECX[0x13] = fVar1 * (float)in_ECX[0x10] +
                 fVar3 * (float)in_ECX[0x11] + fVar2 * (float)in_ECX[0x12];
  *in_ECX = DAT_007c31e0;
  in_ECX[1] = DAT_007c31e4;
  in_ECX[2] = DAT_007c31e8;
  in_ECX[3] = DAT_007c31ec;
  in_ECX[4] = DAT_007c31f0;
  in_ECX[5] = DAT_007c31f4;
  in_ECX[6] = DAT_007c31f8;
  in_ECX[7] = DAT_007c31fc;
  in_ECX[8] = DAT_007c3200;
  in_ECX[9] = DAT_007c3204;
  in_ECX[10] = DAT_007c3208;
  in_ECX[0xb] = DAT_007c320c;
  in_ECX[0xc] = DAT_007c3210;
  in_ECX[0xd] = DAT_007c3214;
  in_ECX[0xe] = DAT_007c3218;
  in_ECX[0xf] = DAT_007c321c;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
