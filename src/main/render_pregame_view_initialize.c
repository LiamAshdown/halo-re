// render_pregame_view_initialize  (Ghidra: FUN_004c8f20; still unnamed -> renamed)
// address 0x4c8f20, size 302 bytes
// name confidence: 0.45   rewrite confidence: 0.8
// evidence: types/main.h's own globals list: "global 0x006b79e8: render_view pregame_render_view
// FUN_004c8f20, handed to the pregame frame 0x50c590 (types/render.h)". Confirmed field-by-field
// against objdump -d -M intel bin/halo.exe at 0x4c8f20..0x4c9046: 0x006b7a40 is
// pregame_render_view.rasterizer_camera (0x006b79e8 + 0x58, types/render.h render_view), built
// directly in place field by field (position 0,0,0; forward 0,0,1; up 0,1,0; mirrored 0; fov via
// fptan/fpatan of the same two constants src/main/chimera__load_ui_map.c's camera_debug_start
// analysis already documents elsewhere in this codebase; viewport_bounds/window_bounds handed to
// viewport_split_rect_compute with view_count=1, view_index=0; z_near 0.01, z_far 1.0), then
// copied whole (21 dwords = sizeof(render_camera)) into pregame_render_view.source_camera
// (0x006b79ec = 0x006b79e8 + 0x04). Finally &pregame_render_view (EAX) is handed to
// render_pregame_frame (0x50c590, src/render/render_pregame_frame.c).
// register convention: no register-passed arguments.
// phase 4 review (disassembly 0x4c8f20..0x4c904e: every store matches; FOV factor now the float 0.6375f.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"
#include "interface.h"
#include "main.h"
#include "fn_sound.h"

extern render_view pregame_render_view; // 0x006b79e8

// cos/sin/tan/atan2 are single x87 instructions in the original code; declared locally instead
// of pulling in the C library's math.h, which would clash with this project's own math.h.
extern double tan(double x);
extern double atan2(double y, double x);


extern void viewport_split_rect_compute(int32_t view_count, int32_t view_index,
    Rectangle2D *window, Rectangle2D *out_viewport); // 0x4c8da0, this module
extern void render_pregame_frame(render_view *view); // 0x50c590, foreign (render module)
    // blam-cc: EAX=view

// Builds the default camera used before any scenario is loaded (looking down +Z, up +Y, at the
// origin, 1x1 viewport split), stamps it into pregame_render_view as both its rasterizer and
// source camera, marks the view as the non-player trailing entry, and renders one frame with it.
void render_pregame_view_initialize(void)
{
    render_camera *camera = &pregame_render_view.rasterizer_camera;

    sound_update();

    camera->position.x = 0.0f;
    camera->position.y = 0.0f;
    camera->position.z = 0.0f;
    camera->forward.i = 0.0f;
    camera->forward.j = 0.0f;
    camera->forward.k = 1.0f;
    camera->up.i = 0.0f;
    camera->up.j = 1.0f;
    camera->up.k = 0.0f;

    pregame_render_view.local_player_index = -1;
    pregame_render_view.nonplayer = 1;
    camera->mirrored = 0;
    camera->vertical_field_of_view =
        (float)(2.0 * atan2(tan(0.6981316804885864) * 0.6375f, 1.0)); // 0x00673098, 0x00673090 float

    viewport_split_rect_compute(1, 0, &camera->window_bounds, &camera->viewport_bounds);

    camera->z_near = 0.01f;
    camera->z_far = 1.0f;

    pregame_render_view.source_camera = *camera;

    render_pregame_frame(&pregame_render_view);
}

#if 0
Original Ghidra decompilation (0x4c8f20):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004c8f20(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  float10 fVar4;

  sound_update();
  fVar4 = (float10)fptan((float10)0.6981316804885864);
  DAT_006b7a40 = 0;
  DAT_006b7a44 = 0;
  _DAT_006b7a48 = 0;
  _DAT_006b7a4c = 0;
  _DAT_006b7a50 = 0;
  _DAT_006b7a54 = 0x3f800000;
  _DAT_006b7a58 = 0;
  _DAT_006b7a5c = 0x3f800000;
  _DAT_006b7a60 = 0;
  _DAT_006b79e8 = 0xffff;
  DAT_006b79ea = 1;
  DAT_006b7a64 = 0;
  fVar4 = (float10)fpatan(fVar4 * (float10)0.63750005,(float10)1.0);
  _DAT_006b7a68 = (float)(fVar4 + fVar4);
  viewport_split_rect_compute(&DAT_006b7a6c);
  _DAT_006b7a7c = 0x3c23d70a;
  _DAT_006b7a80 = 0x3f800000;
  puVar2 = &DAT_006b7a40;
  puVar3 = &DAT_006b79ec;
  for (iVar1 = 0x15; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  FUN_0050c590();
  return;
}
#endif
