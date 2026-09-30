// ui_draw_rotated_screen_quad  (Ghidra: FUN_00494d70, unnamed)
// address 0x494d70, size 475 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/interface_functions.md "Constructs and submits a rotated, scaled
// screen-space quad (e.g. a HUD icon or waypoint marker) for rendering."; ui_draw_screen_quad.c
// (0x498b20) is this function's sibling per that file's own header note, sharing the same
// {X, Y, 0, color-as-float, U, V} six-float, 0x18-byte-per-vertex quad record submitted to the
// same rasterizer_ui_quad_draw, and the same "second, mostly-dead 140-byte scratch record with four floats
// forced to 1.0" pattern; reused here identically.
// register convention: origin {x,y} in EAX (in_EAX, short[2]), plus five stack parameters
// (source_record, corner_uvs, scale, rotation_radians, alpha_fraction).
// blam-cc: EAX -> origin, stack -> (source_record, corner_uvs, scale, rotation_radians, alpha)
// TYPES-GAP / UNSURE: source_record's four shorts (+4, +6, +0x10, +0x12) are not matched to any
// bitmap/sprite-sequence type in this tree; kept as raw offsets (the waypoint caller passes the
// bitmap data returned by bitmap_group_sequence_get_bitmap_data).
// Phase-4 s2 review: the fifth stack argument (objdump 0x494dd5: fld [esp+0x138], fmul 255.0 at
// 0x00672b60, ftol) is the alpha fraction of the vertex color; the earlier fixed 0xff was a
// guess made because Ghidra hid the load. The 0x8c byte record before the vertices is the
// ui_quad_render_state passed to 0x51c9a0 in EAX (mode 7, bitmap at +0x0c); the earlier rewrite
// wrote its floats at the wrong offsets and never passed it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern double fsin(double x); // FSIN
extern double fcos(double x); // FCOS
extern void rasterizer_ui_quad_draw(ui_quad_render_state *state, uint8_t *vertices); // 0x51c9a0, blam-cc: EAX state, stack vertices

// blam-cc: EAX -> origin, stack -> (source_record, corner_uvs, scale, rotation_radians, alpha)
// Builds a 4-vertex screen-space quad centered on `origin`, rotated by `rotation_radians` and
// scaled by `scale`: each corner's local offset comes from source_record's four shorts (treated
// as {left, top, right, bottom}-shaped extents) combined with corner_uvs (defaulting to the
// standard {0,1,0,1} unit square when NULL), rotated and translated to `origin`, with every
// vertex's color forced to opaque white and its UV taken straight from corner_uvs. Submits the
// result via rasterizer_ui_quad_draw, alongside a second, mostly-unused scratch record that stores
// source_record and forces four of its floats to 1.0 (preserved verbatim from the decompile;
// see ui_draw_screen_quad.c's identical note on this pattern).
void ui_draw_rotated_screen_quad(int16_t *origin, int32_t source_record, float *corner_uvs,
                                  float scale, float rotation_radians, float alpha_fraction)
{
    float sin_r = (float)fsin((double)rotation_radians);
    float cos_r = (float)fcos((double)rotation_radians);
    float default_uvs[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
    int32_t alpha = (int32_t)(alpha_fraction * 255.0f); // ftol of arg 5 times 0x00672b60 (255.0)
    int16_t left, right, top, bottom;
    int16_t origin_x, origin_y;
    int16_t corner;
    float vertices[4 * 6]; // {X, Y, 0, color, U, V} per vertex, matching ui_draw_screen_quad.c
    uint8_t quad[0x60];
    ui_quad_render_state state;

    if (corner_uvs == (float *)0) {
        corner_uvs = default_uvs;
    }

    left = *(int16_t *)((char *)source_record + 4);
    right = *(int16_t *)((char *)source_record + 0x10);
    top = *(int16_t *)((char *)source_record + 6);
    bottom = *(int16_t *)((char *)source_record + 0x12);
    origin_x = origin[0];
    origin_y = origin[1];

    for (corner = 0; corner < 4; corner++) {
        // Ghidra's uVar13 (the U selector) is corner+1, one ahead of sVar14 (the V selector,
        // == corner here): (((corner + 1) & 2) == 0) ? uv[0] : uv[1].
        float u = (((corner + 1) & 2) == 0) ? corner_uvs[0] : corner_uvs[1];
        float v = (corner < 2) ? corner_uvs[2] : corner_uvs[3];
        float local_x = ((float)left * u - (float)right) * scale;
        float local_y = ((float)top * v - (float)bottom) * scale;
        float *vert = &vertices[corner * 6];

        vert[0] = (local_x * cos_r + (float)origin_x) - local_y * sin_r;
        vert[1] = local_x * sin_r + local_y * cos_r + (float)origin_y;
        vert[2] = 0.0f;
        *(int32_t *)&vert[3] = (alpha << 0x18) | 0xffffff;
        vert[4] = u;
        vert[5] = v;
    }
    memcpy(quad, vertices, sizeof(vertices));

    memset(&state, 0, sizeof(state));
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.meter_parameters = 0;
    state.single_local_player = 0;
    state.framebuffer_blend_function = 7;
    state.maps[0] = (BitmapData *)source_record;

    rasterizer_ui_quad_draw(&state, quad); // objdump 0x494ef0: EAX state (esp+0x38), stack vertices (esp+0xc4)
}

#if 0
Original Ghidra decompilation (0x494d70):

void FUN_00494d70(int param_1,float *param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  short sVar8;
  float fVar9;
  float fVar10;
  short *in_EAX;
  int iVar11;
  float *pfVar12;
  uint uVar13;
  short sVar14;
  float10 fVar15;
  float10 fVar16;
  float local_fc [7];
  int local_e0;
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined2 local_64;
  undefined1 local_62;
  undefined1 local_60 [4];
  float local_5c [23];

  fVar15 = (float10)fsin((float10)param_4);
  local_fc[0] = 0.0;
  local_fc[1] = 1.0;
  local_fc[2] = 0.0;
  local_fc[3] = 1.0;
  fVar16 = (float10)fcos((float10)param_4);
  if (param_2 == (float *)0x0) {
    param_2 = local_fc;
  }
  iVar11 = __ftol();
  sVar3 = *(short *)(param_1 + 4);
  sVar4 = *(short *)(param_1 + 0x10);
  sVar5 = *(short *)(param_1 + 6);
  sVar6 = *(short *)(param_1 + 0x12);
  sVar7 = *in_EAX;
  sVar8 = in_EAX[1];
  sVar14 = 0;
  uVar13 = 1;
  pfVar12 = local_5c;
  do {
    if ((uVar13 & 2) == 0) {
      fVar1 = *param_2;
    }
    else {
      fVar1 = param_2[1];
    }
    if (sVar14 < 2) {
      fVar2 = param_2[2];
    }
    else {
      fVar2 = param_2[3];
    }
    sVar14 = sVar14 + 1;
    pfVar12[1] = 0.0;
    pfVar12[2] = (float)(iVar11 << 0x18 | 0xffffff);
    uVar13 = uVar13 + 1;
    fVar9 = ((float)(int)sVar3 * fVar1 - (float)(int)sVar4) * param_3;
    fVar10 = ((float)(int)sVar5 * fVar2 - (float)(int)sVar6) * param_3;
    pfVar12[-1] = (fVar9 * (float)fVar16 + (float)(int)sVar7) - fVar10 * (float)fVar15;
    *pfVar12 = fVar9 * (float)fVar15 + fVar10 * (float)fVar16 + (float)(int)sVar8;
    pfVar12[3] = fVar1;
    pfVar12[4] = fVar2;
    pfVar12 = pfVar12 + 6;
  } while (sVar14 < 4);
  pfVar12 = local_fc + 4;
  for (iVar11 = 0x23; iVar11 != 0; iVar11 = iVar11 + -1) {
    *pfVar12 = 0.0;
    pfVar12 = pfVar12 + 1;
  }
  local_a8 = 0x3f800000;
  local_ac = 0x3f800000;
  local_c0 = 0x3f800000;
  local_c4 = 0x3f800000;
  local_fc[4] = 0.0;
  local_62 = 0;
  local_64 = 7;
  local_e0 = param_1;
  FUN_0051c9a0(local_60);
  return;
}
#endif
