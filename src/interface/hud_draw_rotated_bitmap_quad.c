// hud_draw_rotated_bitmap_quad  (Ghidra: FUN_004acd50, renamed in the phase-4 review)
// address 0x4acd50, size 410 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4acd50..0x4acee9 in the phase-4 review. Builds the four
// hud_quad_vertex records (vertex i takes u[(i+1)&2 ? 1 : 0], v[i > 1 ? 1 : 0] and the matching
// extents), rotates the extents by the angle argument, scales them by ESI[0]/ESI[1], rounds
// with a bare fistp (round to nearest even) and adds the int16 screen position in EAX. Then
// zeroes a ui_quad_render_state and submits it to 0x51c9a0 with blend function 7. The first
// rewrite had the argument list wrong (no register arguments, invented depth/scale).
// register convention: EAX screen position (Point2DInt), ESI scale (float[2]); six stack args.
//   // blam-cc: screen_position -> EAX, scale -> ESI

#include <string.h>
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern float sinf(float x);
extern float cosf(float x);
extern long lrint(double x); // x87 fistp under the default control word (round-half-to-even)
extern void rasterizer_ui_quad_draw(ui_quad_render_state *state, hud_quad_vertex *vertices); // 0x51c9a0, rasterizer quad submitter, blam-cc: EAX state

// blam-cc: screen_position -> EAX, scale -> ESI
// Draws one rotated HUD quad. uv is {u0, u1, v0, v1}, extents {x0, x1, y0, y1} around the
// screen position (see hud_bitmap_anchor_extents), rotation in radians, color packed ARGB.
// meter_parameters is the hud_meter_color_block of a meter fill, NULL for a plain bitmap.
void hud_draw_rotated_bitmap_quad(const Point2DInt *screen_position, const float *scale,
                                  void *meter_parameters, BitmapData *bitmap, const float *uv,
                                  const float *extents, float rotation, uint32_t color)
{
    ui_quad_render_state state;
    hud_quad_vertex vertices[4];
    float sine;
    float cosine;
    int16_t i;

    sine = sinf(rotation);
    cosine = cosf(rotation);
    for (i = 0; i < 4; i++) {
        int32_t corner = i + 1;
        float u = (corner & 2) != 0 ? uv[1] : uv[0];
        float v = i > 1 ? uv[3] : uv[2];
        float x = (corner & 2) != 0 ? extents[1] : extents[0];
        float y = i > 1 ? extents[3] : extents[2];
        float rotated;

        rotated = (x * cosine - y * sine) * scale[0];
        vertices[i].x = (float)(screen_position->x + (int32_t)lrint(rotated));
        rotated = (y * cosine + x * sine) * scale[1];
        vertices[i].y = (float)(screen_position->y + (int32_t)lrint(rotated));
        vertices[i].z = 0.0f;
        vertices[i].color = color;
        vertices[i].u = u;
        vertices[i].v = v;
    }

    memset(&state, 0, sizeof(state));
    state.meter_parameters = meter_parameters;
    state.map_texel_scales[0].y = 1.0f;
    state.map_texel_scales[0].x = 1.0f;
    state.map_scales[0].y = 1.0f;
    state.map_scales[0].x = 1.0f;
    state.single_local_player = 0;
    state.framebuffer_blend_function = 7;
    state.maps[0] = bitmap;
    rasterizer_ui_quad_draw(&state, vertices);
}

#if 0
Original Ghidra decompilation (0x4acd50):

void FUN_004acd50(undefined4 param_1,undefined4 param_2,float *param_3,float *param_4,float param_5,
                 float param_6)

{
  float fVar1;
  short sVar2;
  short *in_EAX;
  float *pfVar3;
  short sVar4;
  int iVar5;
  uint uVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float *unaff_ESI;
  undefined4 *puVar11;
  float10 fVar12;
  float10 fVar13;
  undefined4 local_ec [10];
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined2 local_64;
  undefined1 local_62;
  undefined1 local_60 [4];
  float local_5c [23];

  fVar12 = (float10)fsin((float10)param_5);
  sVar4 = 0;
  uVar6 = 1;
  pfVar3 = local_5c;
  fVar13 = (float10)fcos((float10)param_5);
  do {
    if ((uVar6 & 2) == 0) {
      fVar7 = *param_3;
    }
    else {
      fVar7 = param_3[1];
    }
    if (sVar4 < 2) {
      fVar8 = param_3[2];
    }
    else {
      fVar8 = param_3[3];
    }
    if ((uVar6 & 2) == 0) {
      fVar9 = *param_4;
    }
    else {
      fVar9 = param_4[1];
    }
    if (sVar4 < 2) {
      fVar10 = param_4[2];
    }
    else {
      fVar10 = param_4[3];
    }
    pfVar3[-1] = (float)((int)*in_EAX +
                        (int)ROUND((fVar9 * (float)fVar13 - fVar10 * (float)fVar12) * *unaff_ESI));
    fVar1 = unaff_ESI[1];
    sVar2 = in_EAX[1];
    pfVar3[3] = fVar7;
    pfVar3[4] = fVar8;
    sVar4 = sVar4 + 1;
    *pfVar3 = (float)((int)sVar2 +
                     (int)ROUND((fVar9 * (float)fVar12 + fVar10 * (float)fVar13) * fVar1));
    pfVar3[1] = 0.0;
    pfVar3[2] = param_6;
    uVar6 = uVar6 + 1;
    pfVar3 = pfVar3 + 6;
  } while (sVar4 < 4);
  puVar11 = local_ec;
  for (iVar5 = 0x23; iVar5 != 0; iVar5 = iVar5 + -1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  local_ec[0] = param_1;
  local_a8 = 0x3f800000;
  local_ac = 0x3f800000;
  local_c0 = 0x3f800000;
  local_c4 = 0x3f800000;
  local_62 = 0;
  local_64 = 7;
  local_ec[3] = param_2;
  FUN_0051c9a0(local_60);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
