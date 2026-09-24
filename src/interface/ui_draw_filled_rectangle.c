// ui_draw_filled_rectangle  (Ghidra: FUN_00449780; named by the orphan pass 4 review. The first draft
//   kept FUN_00449780 because seven callers declared it under that name; they are renamed with
//   it: src/cutscene/chimera__letterbox.c, src/game/game_engine_rasterize_in_game_score.c,
//   src/interface/interface_draw_cursor.c, ui_draw_trouble_brewing_indicator.c,
//   ui_error_modal_update.c, widget_draw_fullscreen_region.c, src/rasterizer/rasterizer_end_frame.c.
//   The name follows the ui_draw_* quad builders of this module, e.g. ui_draw_screen_quad)
// address 0x449780, size 473 bytes
// name confidence: 0.6   rewrite confidence: 0.75 (vertex corners and colors re-checked against
//   objdump 0x4497c4..0x449867 in the review)
// evidence: orphan pass. modules.json guessed "cutscene" (confidence 0.5); cutscene/README.md
// explicitly calls that a misattribution ("0x449780 is the generic filled screen rectangle
// (EAX packed ARGB, ECX Rectangle2D*)... It belongs with interface/rasterizer") and never wrote
// it. Of its 7 call sites, 4 are in this module (interface_draw_cursor.c,
// ui_draw_trouble_brewing_indicator.c, ui_error_modal_update.c, widget_draw_fullscreen_region.c,
// all already declaring the EAX/ECX convention below) versus 2 in rasterizer and 1 each in
// cutscene/game, so it is placed here.
// register convention: EAX packed ARGB color (in_EAX in the Ghidra decompile, read but never
// reassigned), ECX Rectangle2D* rect (in_ECX). Matches every existing caller's extern in the
// repo (interface/README.md's calling-convention table: "FUN_00449780 | EAX packed color,
// ECX rect: solid rectangle fill").
// blam-cc: EAX -> packed_color, ECX -> rect
// Phase-4 orphan-pass review (objdump 0x449780..0x449958): builds a ui_quad_render_state
// (types/interface.h, zeroed then only meter_parameters/maps[0]/map_scales[0]/
// map_texel_scales[0]/framebuffer_blend_function set, matching every other single-bitmap
// builder in this module) and four hud_quad_vertex corners of `rect` (top-left, top-right,
// bottom-right, bottom-left; z=0, u=v=0, color=packed_color), pointed at the globals tag's
// "default_2d" bitmap's second BitmapData entry (index 1, not 0 -- offset +0x30 past the first
// BitmapData record, BitmapData being 0x30 bytes per types/tags.h), and submits it to
// rasterizer_ui_quad_draw (0x51c9a0) the same way ui_draw_screen_quad.c and the other ui_draw_*
// builders in this module do. rasterizer_vertex_buffer_lock_state (0x0069c632) is set to 8 for
// the duration of the submit call and restored to 0 after, the same bracket-and-restore idiom
// used by every other 0x0069c632 writer in the tree (build_sprite_get_group.c, etc; the exact
// meaning of the value 8 versus their 2/4/5/9/0xb/0xf/0x10 is not recovered here).
// UNSURE: why the second BitmapData entry (index 1) rather than the first is used is not
// recovered; the objdump pointer arithmetic (bitmap_data.pointer + 0x30, BitmapData being 0x30
// bytes) is unambiguous, so the +1 indexing is reproduced exactly rather than guessed at.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern Globals *global_globals;      // 0x00746fa0, foreign (game module)
extern tag_instance *tag_instances;  // 0x0087bc14, foreign (cache module)
extern int16_t rasterizer_vertex_buffer_lock_state; // 0x0069c632, foreign (rasterizer module)

extern void rasterizer_ui_quad_draw(ui_quad_render_state *state, uint8_t *vertices); // 0x51c9a0, blam-cc: EAX state, stack vertices

// blam-cc: EAX -> packed_color, ECX -> rect
// Fills `rect` with `packed_color` by submitting a single opaque screen-space quad textured
// against the globals tag's default_2d bitmap (its second BitmapData entry, sampled at a fixed
// (0,0) UV so no part of the bitmap actually shows through the tint).
void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect)
{
    uint8_t vertices[4 * sizeof(hud_quad_vertex)];
    hud_quad_vertex *v = (hud_quad_vertex *)vertices;
    ui_quad_render_state state;
    GlobalsRasterizerData *rasterizer_data;
    datum_index default_2d_tag;
    Bitmap *default_2d_bitmap;
    BitmapData *default_2d_bitmap_data;
    int32_t i;

    rasterizer_data = (global_globals->rasterizer_data.count == 0)
        ? (GlobalsRasterizerData *)0
        : (GlobalsRasterizerData *)global_globals->rasterizer_data.pointer;
    default_2d_tag = *(datum_index *)&rasterizer_data->default_2d.tag_id;
    default_2d_bitmap = (Bitmap *)tag_instances[default_2d_tag & 0xffff].data;
    default_2d_bitmap_data = (BitmapData *)default_2d_bitmap->bitmap_data.pointer;

    // Top-left, top-right, bottom-right, bottom-left; every vertex shares z=0, uv=(0,0) and the
    // caller's color (objdump 0x4497c4..0x449867 builds exactly these four corners of `rect`).
    v[0].x = (float)(int32_t)rect->left;  v[0].y = (float)(int32_t)rect->top;
    v[1].x = (float)(int32_t)rect->right; v[1].y = (float)(int32_t)rect->top;
    v[2].x = (float)(int32_t)rect->right; v[2].y = (float)(int32_t)rect->bottom;
    v[3].x = (float)(int32_t)rect->left;  v[3].y = (float)(int32_t)rect->bottom;
    for (i = 0; i < 4; i++) {
        v[i].z = 0.0f;
        v[i].u = 0.0f;
        v[i].v = 0.0f;
        v[i].color = packed_color;
    }

    for (i = 0; i < (int32_t)(sizeof(state) / sizeof(int32_t)); i++) {
        ((int32_t *)&state)[i] = 0;
    }
    state.meter_parameters = (void *)0;
    state.maps[0] = &default_2d_bitmap_data[1]; // second BitmapData entry, see UNSURE above
    state.map_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_texel_scales[0].y = 1.0f;
    state.framebuffer_blend_function = 0;
    state.single_local_player = 0;

    rasterizer_vertex_buffer_lock_state = 8;
    rasterizer_ui_quad_draw(&state, vertices);
    rasterizer_vertex_buffer_lock_state = 0;
}

#if 0
Original Ghidra decompilation (0x449780):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00449780(void)

{
  short *in_ECX;
  int iVar1;
  int *piVar2;
  float local_f8;
  float local_f4;
  undefined4 local_f0;
  undefined4 local_e8;
  undefined4 local_e4;
  float local_e0;
  float local_dc;
  undefined4 local_d8;
  undefined4 local_d0;
  undefined4 local_cc;
  float local_c8;
  float local_c4;
  undefined4 local_c0;
  undefined4 local_b8;
  undefined4 local_b4;
  float local_b0;
  float local_ac;
  undefined4 local_a8;
  undefined4 local_a0;
  undefined4 local_9c;
  int local_98 [10];
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_58;
  undefined4 local_54;
  undefined2 local_10;
  undefined1 local_e;

  if (*(int *)(DAT_00746fa0 + 0x134) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(DAT_00746fa0 + 0x138);
  }
  local_f8 = (float)(int)in_ECX[1];
  local_f4 = (float)(int)*in_ECX;
  local_e0 = (float)(int)in_ECX[3];
  local_c4 = (float)(int)in_ECX[2];
  local_98[3] = *(int *)(*(int *)((*(uint *)(iVar1 + 0xb8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                        100) + 0x30;
  piVar2 = local_98;
  for (iVar1 = 0x23; iVar1 != 0; iVar1 = iVar1 + -1) {
    *piVar2 = 0;
    piVar2 = piVar2 + 1;
  }
  _DAT_0069c632 = 8;
  local_e8 = 0;
  local_e4 = 0;
  local_f0 = 0;
  local_d0 = 0;
  local_cc = 0;
  local_d8 = 0;
  local_b8 = 0;
  local_b4 = 0;
  local_c0 = 0;
  local_a0 = 0;
  local_9c = 0;
  local_a8 = 0;
  local_10 = 0;
  local_54 = 0x3f800000;
  local_58 = 0x3f800000;
  local_6c = 0x3f800000;
  local_70 = 0x3f800000;
  local_98[0] = 0;
  local_e = 0;
  local_dc = local_f4;
  local_c8 = local_e0;
  local_b0 = local_f8;
  local_ac = local_c4;
  rasterizer_ui_quad_draw(&local_f8);
  _DAT_0069c632 = 0;
  return;
}
#endif
