// rasterizer_frame_statistics_graph_init  (Ghidra: rasterizer_frame_statistics_graph_init,
// already named; CEA fg_init, hint only; the functions.txt name is kept)
// address 0x512700, size 662 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: objdump -d -M intel 0x512700..0x512995, re-traced in the phase-4 review (FPU stack
//   and push depth followed). types/render.h frame_graph (frame_graphs[0] at 0x006b9260).
//   - the game window rectangle 0x0069c634..0x0069c63b (types/rasterizer.h game_window_top_left
//     / game_window_bottom_right) is read as top, left, bottom, right: w = right - left
//     (0x0069c63a - 0x0069c636), h = bottom - top (0x0069c638 - 0x0069c634). Nothing is rebuilt
//     while frame_graph_window_width (0x0071d118) == h and frame_graph_window_height
//     (0x0071d11c) == w (render.h's names for the two caches are swapped relative to what they
//     hold; the comparison order is reproduced).
//   - graph bounds: top 30, left 64, bottom 150, right = (int)(right - 64).
//   - x_scale = 640 / w (0x672ba0), y_scale = 480 / h (0x672b9c), the 640 x 480 text space.
//   - all 0x200 line vertices and the 5 border vertices are zeroed (rep stosd 0xc00 / 0x1e
//     dwords), recent_samples cleared; line vertex i = ((int)(i * (right - 64 - 64) / 512 + 64),
//     150.0), colour 0xffffffff (1/512 = 0x672b94, 64 = 0x672b98).
//   - the border strip (colour 0xffffff00): (63, 29) (right - 63, 29) (right - 63, 151) (63, 151)
//     (63, 29), where right - 63 is the float (right - 64) + 1.
//   - label rectangles, all ending at (bottom 480, right 640): name at (30 y, 64 x) scaled,
//     maximum at (30 y, bounds.right x) scaled, average at (30 + 60 y, bounds.right x) scaled
//     (60.0 = 0x672b90), each through __ftol.
//   - name = "FPS" (the dword 0x00535046), maximum = 60.0, average = 0.
// review fix (phase-4 gate): the first draft divided 640 by the graph right edge and 480 by the
//   window width, dropped the (right - 128) factor of the line vertex x, and placed the border's
//   right edge at 65 - right; all three are corrected from the disassembly.
// register convention: none (void).
//   // blam-cc: void
// UNSURE: which of the two window caches CEA calls width / height (see above).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "fn_render.h"

extern Rectangle2D game_window_top_left;     // 0x0069c634 top, 0x0069c636 left, 0x0069c638 bottom,
                                           // 0x0069c63a right (rasterizer module; src/rasterizer
                                           // declares the halves as game_window_top_left /
                                           // game_window_bottom_right)
extern int32_t frame_graph_window_width;   // 0x0071d118, this module (holds bottom - top)
extern int32_t frame_graph_window_height;  // 0x0071d11c, this module (holds right - left)
extern frame_graph frame_graphs[1];        // 0x006b9260, this module

// Rebuilds the frame rate graph layout (line strip, border, label rectangles) for the current
// game window size, and resets its samples and labels, whenever the window size changed.
void rasterizer_frame_statistics_graph_init(void)
{
    frame_graph *g = &frame_graphs[0];
    int32_t width = (int32_t)game_window_top_left.right - (int32_t)game_window_top_left.left;
    int32_t height = (int32_t)game_window_top_left.bottom - (int32_t)game_window_top_left.top;
    float right;
    float span;
    float x_scale;
    float y_scale;
    float border_right;
    uint32_t *raw;
    int32_t i;

    if (frame_graph_window_width == height && frame_graph_window_height == width) {
        return;
    }

    right = (float)((int32_t)game_window_top_left.right - 0x40);
    frame_graph_window_width = height;
    frame_graph_window_height = width;
    g->bounds.left = 0x40;
    g->bounds.top = 0x1e;
    x_scale = 640.0f / (float)width;
    y_scale = 480.0f / (float)height;
    g->bounds.right = (int16_t)(int32_t)right;
    span = right - 64.0f;
    g->bounds.bottom = 0x96;

    raw = (uint32_t *)g->vertices;
    for (i = 0; i < 0xc00; i++) {
        raw[i] = 0;
    }
    raw = (uint32_t *)g->frame_vertices;
    for (i = 0; i < 0x1e; i++) {
        raw[i] = 0;
    }
    g->recent_samples[0] = 0.0f;
    g->recent_samples[1] = 0.0f;
    g->recent_samples[2] = 0.0f;
    g->recent_samples[3] = 0.0f;

    for (i = 0; i < 0x200; i++) {
        g->vertices[i].y = 150.0f;
        g->vertices[i].color = 0xffffffff;
        g->vertices[i].x = (float)(int32_t)((float)i * span * 0.001953125f + 64.0f);
    }

    border_right = right + 1.0f;
    g->frame_vertices[0].x = 63.0f;
    g->frame_vertices[1].x = border_right;
    g->frame_vertices[0].y = 29.0f;
    g->frame_vertices[2].x = border_right;
    g->frame_vertices[0].color = 0xffffff00;
    g->frame_vertices[1].y = 29.0f;
    g->frame_vertices[1].color = 0xffffff00;
    g->frame_vertices[2].y = 151.0f;
    g->frame_vertices[2].color = 0xffffff00;
    g->frame_vertices[3].x = 63.0f;
    g->frame_vertices[3].y = 151.0f;
    g->frame_vertices[3].color = 0xffffff00;
    g->frame_vertices[4].x = 63.0f;
    g->frame_vertices[4].y = 29.0f;
    g->frame_vertices[4].color = 0xffffff00;

    g->name_bounds.left = (int16_t)(int32_t)((float)g->bounds.left * x_scale);
    g->name_bounds.top = (int16_t)(int32_t)((float)g->bounds.top * y_scale);
    g->name_bounds.right = 0x280;
    g->name_bounds.bottom = 0x1e0;
    g->maximum_bounds.left = (int16_t)(int32_t)((float)g->bounds.right * x_scale);
    g->maximum_bounds.top = (int16_t)(int32_t)((float)g->bounds.top * y_scale);
    g->maximum_bounds.right = 0x280;
    g->maximum_bounds.bottom = 0x1e0;
    g->average_bounds.left = (int16_t)(int32_t)((float)g->bounds.right * x_scale);
    g->average_bounds.top = (int16_t)(int32_t)(((float)g->bounds.top + 60.0f) * y_scale);
    g->average_bounds.right = 0x280;
    g->average_bounds.bottom = 0x1e0;

    *(uint32_t *)g->name = 0x00535046; // "FPS"
    g->maximum = 60.0f;
    g->average = 0.0f;
}

#if 0
Original Ghidra decompilation (0x512700):

void __cdecl rasterizer_frame_statistics_graph_init(void)

{
  int iVar1;
  int iVar2;
  undefined4 *puVar3;
  float10 extraout_ST1;

  if ((DAT_0071d118 != (int)(short)DAT_0069c638 - (int)(short)DAT_0069c634) ||
     (DAT_0071d11c != (int)DAT_0069c638._2_2_ - (int)DAT_0069c634._2_2_)) {
    DAT_006b9262 = 0x40;
    DAT_006b9260 = 0x1e;
    DAT_0071d118 = (int)(short)DAT_0069c638 - (int)(short)DAT_0069c634;
    DAT_0071d11c = (int)DAT_0069c638._2_2_ - (int)DAT_0069c634._2_2_;
    DAT_006b9266 = __ftol();
    _DAT_006b9264 = 0x96;
    puVar3 = &DAT_006b9280;
    for (iVar2 = 0xc00; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    puVar3 = &DAT_006bc280;
    for (iVar2 = 0x1e; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = 0;
      puVar3 = puVar3 + 1;
    }
    _DAT_006bc300 = 0;
    _DAT_006bc304 = 0;
    _DAT_006bc308 = 0;
    iVar2 = 0;
    _DAT_006bc30c = 0;
    puVar3 = &DAT_006b928c;
    do {
      iVar1 = __ftol();
      iVar2 = iVar2 + 1;
      puVar3[-2] = 0x43160000;
      *puVar3 = 0xffffffff;
      puVar3[-3] = (float)iVar1;
      puVar3 = puVar3 + 6;
    } while (iVar2 < 0x200);
    DAT_006bc280 = 0x427c0000;
    _DAT_006bc298 = (float)(extraout_ST1 + (float10)1.0);
    DAT_006bc284 = 0x41e80000;
    _DAT_006bc2b0 = (float)(extraout_ST1 + (float10)1.0);
    _DAT_006bc28c = 0xffffff00;
    _DAT_006bc29c = 0x41e80000;
    _DAT_006bc2a4 = 0xffffff00;
    _DAT_006bc2b4 = 0x43170000;
    _DAT_006bc2bc = 0xffffff00;
    _DAT_006bc2c8 = 0x427c0000;
    _DAT_006bc2cc = 0x43170000;
    _DAT_006bc2d4 = 0xffffff00;
    _DAT_006bc2e0 = 0x427c0000;
    _DAT_006bc2e4 = 0x41e80000;
    _DAT_006bc2ec = 0xffffff00;
    _DAT_006b926a = __ftol();
    _DAT_006b9268 = __ftol();
    _DAT_006b926e = 0x280;
    _DAT_006b926c = 0x1e0;
    _DAT_006b9272 = __ftol();
    _DAT_006b9270 = __ftol();
    _DAT_006b9276 = 0x280;
    _DAT_006b9274 = 0x1e0;
    _DAT_006b927a = __ftol();
    _DAT_006b9278 = __ftol();
    _DAT_006b927e = 0x280;
    _DAT_006b927c = 0x1e0;
    _DAT_006bc310 = &DAT_00535046;
    _DAT_006bc2f8 = 0x42700000;
    _DAT_006bc2fc = 0;
  }
  return;
}

Reconstructed instruction-for-instruction from objdump (0x512700..0x512995) because the decompile
above elides every FPU operand as extraout_ST1/bare __ftol(); the label rectangles' left/top
values (name_bounds, maximum_bounds, average_bounds) come from:
  a = (float)(window_b.y - 0x40)
  b = 0x672ba0 / a
  c = 0x672b9c / (float)height
  name_bounds.left     = (int)(bounds.left  * b)      ; bounds.left  == 0x40
  name_bounds.top      = (int)(bounds.top   * c)      ; bounds.top   == 0x1e
  maximum_bounds.left  = (int)(bounds.right * b)
  maximum_bounds.top   = (int)(bounds.top   * c)
  average_bounds.left  = (int)(bounds.right * b)
  average_bounds.top   = (int)((bounds.top + 0x672b90) * c)
  border_right         = 0x672b98 - a + 1.0            ; frame_vertices[1].x and [2].x
  vertices[i].x         = (int)(i * 0x672b94 + 0x672b98)
#endif
