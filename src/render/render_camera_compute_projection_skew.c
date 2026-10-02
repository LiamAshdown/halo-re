// render_camera_compute_projection_skew  (Ghidra: FUN_0050ca90; new name, evidence below)
// address 0x50ca90, size 215 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: out/phase4/render_functions.md's phase-2 summary: "Computes the projection-center
//   offset/skew terms for an (asymmetric) viewport rectangle." Fully disassembled
//   (objdump -d -M intel, 0x50ca90..0x50cb70) while tracing render_player_frame's calls into
//   this function; confirmed there that EAX is the camera and ECX the 4-float output, and that
//   the four outputs are exactly the asymmetric-frustum bounds render_player_frame threads
//   through to chimera__render_camera_build_frustum.
// register convention: EAX = camera (render_camera*), ECX = bounds_out (float[4]).
//   // blam-cc: EAX=camera, ECX=bounds_out

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

// Computes the four asymmetric-projection skew terms for a camera whose viewport is a sub-
// rectangle of its window (e.g. a split-screen pane): bounds_out[0..1] are the left/right terms
// (scaled by the viewport's aspect ratio against its own width), and bounds_out[2..3] are the
// (negated) bottom/top terms, all normalized by the window's height.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void render_camera_compute_projection_skew(render_camera *camera, float bounds_out[4]) // blam-cc: EAX=camera, ECX=bounds_out
{
    float aspect = (float)(camera->viewport_bounds.bottom - camera->viewport_bounds.top) /
                   (float)(camera->viewport_bounds.right - camera->viewport_bounds.left);
    float inverse_window_height = 1.0f / (float)(camera->window_bounds.bottom - camera->window_bounds.top);
    float left = (float)(camera->viewport_bounds.left * 2 - camera->window_bounds.left -
                          camera->window_bounds.right) * inverse_window_height;
    float right = (float)(camera->viewport_bounds.right * 2 - camera->window_bounds.left -
                           camera->window_bounds.right) * inverse_window_height;
    // Named for the viewport edge each is derived from, not for the output slot it ends up in --
    // see the final assignments below, which swap and negate them exactly as the binary does.
    float term_from_viewport_top = (float)(camera->viewport_bounds.top * 2 - camera->window_bounds.bottom -
                                            camera->window_bounds.top) * inverse_window_height;
    float term_from_viewport_bottom = (float)(camera->viewport_bounds.bottom * 2 - camera->window_bounds.bottom -
                                               camera->window_bounds.top) * inverse_window_height;

    bounds_out[0] = aspect * left;
    bounds_out[1] = aspect * right;
    bounds_out[2] = -term_from_viewport_bottom;
    bounds_out[3] = -term_from_viewport_top;
}

#if 0
Original Ghidra decompilation (0x50ca90):

void FUN_0050ca90(void)

{
  float fVar1;
  float fVar2;
  int in_EAX;
  float *in_ECX;

  fVar2 = (float)((int)*(short *)(in_EAX + 0x30) - (int)*(short *)(in_EAX + 0x2c)) /
          (float)((int)*(short *)(in_EAX + 0x32) - (int)*(short *)(in_EAX + 0x2e));
  fVar1 = 1.0 / (float)((int)*(short *)(in_EAX + 0x38) - (int)*(short *)(in_EAX + 0x34));
  *in_ECX = (float)((*(short *)(in_EAX + 0x2e) * 2 - (int)*(short *)(in_EAX + 0x36)) -
                   (int)*(short *)(in_EAX + 0x3a)) * fVar1;
  in_ECX[1] = (float)((*(short *)(in_EAX + 0x32) * 2 - (int)*(short *)(in_EAX + 0x36)) -
                     (int)*(short *)(in_EAX + 0x3a)) * fVar1;
  in_ECX[2] = (float)((*(short *)(in_EAX + 0x2c) * 2 - (int)*(short *)(in_EAX + 0x38)) -
                     (int)*(short *)(in_EAX + 0x34)) * fVar1;
  in_ECX[3] = (float)((*(short *)(in_EAX + 0x30) * 2 - (int)*(short *)(in_EAX + 0x38)) -
                     (int)*(short *)(in_EAX + 0x34)) * fVar1;
  *in_ECX = fVar2 * *in_ECX;
  in_ECX[1] = fVar2 * in_ECX[1];
  fVar1 = in_ECX[2];
  in_ECX[2] = -in_ECX[3];
  in_ECX[3] = -fVar1;
  return;
}

Field mapping (render_camera, types/rasterizer.h): viewport_bounds (Rectangle2D: top,left,bottom,
right) at +0x2c..+0x33, window_bounds at +0x34..+0x3b. So in_EAX+0x2c/0x2e/0x30/0x32 are
viewport.top/left/bottom/right, and in_EAX+0x34/0x36/0x38/0x3a are window.top/left/bottom/right.
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
