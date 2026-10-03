// ui_draw_screen_quad  (Ghidra: FUN_00498b20, unnamed)
// address 0x498b20, size 737 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x498b20..0x498e00: rect build, clip clamps, uv scales, vertex loop and render state (the four 0.9f stores at entry are dead locals).)
// evidence: out/phase4/interface_functions.md's sibling FUN_00494d70 @0x494d70 (out of this
// range) is summarized as "constructs and submits a rotated, scaled screen-space quad"; this
// function has the same shape (a bitmap-sequence-like source, an optional clip rectangle, a
// color/alpha, four vertices built with UV corners from the source rect clamped against the
// clip rect) and is called by interface_draw_cursor @0x497380 and 7 other sites in this module
// with a bitmap-data pointer, an optional clip rect and a packed color. Kept as its own function
// (not folded into 0x494d70) because the two have different call signatures and neither pack
// references the other.
// register convention: source_rect in EAX (in_EAX, optional, short[4] {left,top,right,bottom}),
// dest_rect in ECX (in_ECX, required, same shape), then bitmap_data/clip_rect/vertex_color as the
// three recognized stack parameters, matching blam-cc's EAX, ECX, then-stack order.
// blam-cc: EAX -> source_rect, ECX -> dest_rect, stack -> bitmap_data/clip_rect/vertex_color
// Phase-4 s2 review (objdump 0x498d87..0x498dee): the 140 byte record below the vertices is the
// ui_quad_render_state (types/interface.h) that 0x51c9a0 receives in EAX; the vertices are its
// stack argument. The earlier rewrite treated the record as dead and put the four 1.0 floats at
// 0x14/0x18/0x20/0x24 and the word at 0x28; the stores are at 0x28/0x2c/0x40/0x44 and 0x88.
// TYPES-GAP: the quad/vertex record passed to rasterizer_ui_quad_draw (0x51c9a0, well outside this module)
// is not a type from any header in this tree; represented as a raw uint8_t[0x60] buffer of four
// {row, col, 0, w, u, v} vertices (0x18 bytes each), the layout Ghidra's overlapping stack
// locals implied once local_60 and local_50 are read as one contiguous buffer.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void rasterizer_ui_quad_draw(ui_quad_render_state *state, uint8_t *vertices); // 0x51c9a0, blam-cc: EAX state, stack vertices

// blam-cc: EAX -> source_rect, ECX -> dest_rect, stack -> bitmap_data/clip_rect/vertex_color
// Builds a screen-space quad (four vertices of {row, col, 0, w, u, v}) covering dest_rect
// (clamped against clip_rect when given), with per-vertex UVs mapped from source_rect
// (defaulting to the bitmap's full {0,0,width,height} extent) and every vertex's w set to
// vertex_w, then hands it to the render submission routine.
void ui_draw_screen_quad(int16_t *source_rect, int16_t *dest_rect, int32_t bitmap_data,
                          int16_t *clip_rect, uint32_t vertex_color)
{
    if (bitmap_data != 0 && dest_rect != (int16_t *)0) {
        uint8_t submit_buffer[0x60];  // passed to rasterizer_ui_quad_draw; see TYPES-GAP above
        ui_quad_render_state state;   // EAX of 0x51c9a0, see the phase-4 note in the header
        int16_t fallback_rect[4];
        // Ghidra's local_11c[5] float array runs directly into local_108/104/100 in the stack
        // frame, so the vertex loop below legitimately reads one array of 8: corners[0..4] then
        // {right, top(dup), right(dup)} -- kept as one array here rather than split in two.
        float corners[8]; // [0]=top [1]=left [2]=bottom [3]=left(dup) [4]=bottom(dup)
                           // [5]=right [6]=top(dup, aliases corners[0]) [7]=right(dup)
        int32_t clamped_bottom; // Ghidra's local_12c, reused as scratch
        float scale_u, scale_v;
        int32_t axis; // 0..3, Ghidra's sVar8 inside the vertex loop
        int32_t vertex_index; // Ghidra's iVar10
        float *corner_pair;
        int32_t i;

        if (source_rect == (int16_t *)0) {
            // Fallback source rect: {left=0, top=0, right=bitmap width, bottom=bitmap height}.
            fallback_rect[0] = 0;
            fallback_rect[1] = 0;
            fallback_rect[3] = *(int16_t *)(bitmap_data + 4); // width
            fallback_rect[2] = *(int16_t *)(bitmap_data + 6); // height
            source_rect = fallback_rect;
        }

        // Build the four destination-rect edges (clamped below against clip_rect), matching
        // dest_rect's {left, top, right, bottom} shorts.
        corners[0] = (float)(int32_t)dest_rect[1]; // top
        corners[1] = (float)(int32_t)dest_rect[0]; // left
        corners[2] = (float)((int32_t)(int16_t)(dest_rect[3] - dest_rect[1]) + (int32_t)dest_rect[1]); // bottom
        clamped_bottom = (int32_t)(int16_t)(dest_rect[2] - dest_rect[0]) + (int32_t)dest_rect[0]; // right
        corners[3] = corners[1];
        corners[4] = corners[2];
        corners[5] = (float)clamped_bottom; // right
        corners[6] = corners[0];            // top
        corners[7] = corners[5];            // right

        if (clip_rect != (int16_t *)0) {
            int16_t v;

            v = clip_rect[1]; // clip top
            if (dest_rect[1] < v) {
                clamped_bottom = (int32_t)v;
                corners[6] = (float)(int32_t)v;
                corners[0] = corners[6];
            }
            v = clip_rect[3]; // clip bottom
            if (v < (int16_t)dest_rect[3]) {
                corners[4] = (float)(int32_t)v;
                corners[2] = corners[4];
            }
            v = clip_rect[0]; // clip left
            if (dest_rect[0] < v) {
                // 0x498c69: only slots 3 and 1 ([esp+0x30], [esp+0x28]); slot 6 keeps the other clamp
                corners[3] = (float)(int32_t)v;
                corners[1] = corners[3];
            }
            if (clip_rect[2] < dest_rect[2]) { // clip right
                corners[7] = (float)(int32_t)clip_rect[2];
                corners[5] = corners[7];
            }
        }

        scale_u = (float)(int32_t)*(int16_t *)(bitmap_data + 4); // bitmap width
        if (scale_u < 1.0f) scale_u = 1.0f;
        scale_u = (float)(int32_t)(int16_t)(source_rect[3] - source_rect[1]) / scale_u;
        if (scale_u > 1.0f) scale_u = 1.0f;

        scale_v = (float)(int32_t)*(int16_t *)(bitmap_data + 6); // bitmap height
        if (scale_v < 1.0f) scale_v = 1.0f;
        scale_v = (float)(int32_t)(int16_t)(source_rect[2] - source_rect[0]) / scale_v; // 0x498ba3: src[2] - src[0]
        if (scale_v > 1.0f) scale_v = 1.0f;

        // Four vertices of 6 floats (0x18 bytes) each: {row, col, 0, w, u, v}, written at
        // submit_buffer + i*0x18. Vertex order is top-left, bottom-left, bottom-right, top-right.
        axis = 0;
        vertex_index = 0;
        corner_pair = corners + 1;
        do {
            float u, v_uv, row, col;
            uint8_t *vertex = submit_buffer + vertex_index * 0x18;

            u = (vertex_index % 3 == 0) ? 0.0f : scale_u;
            v_uv = (axis < 2) ? 0.0f : scale_v;
            row = corner_pair[-1];
            col = corner_pair[0];
            axis = axis + 1;
            *(float *)(vertex + 0x00) = row;
            *(float *)(vertex + 0x04) = col;
            *(float *)(vertex + 0x08) = 0.0f;
            *(uint32_t *)(vertex + 0x0c) = vertex_color; // objdump 0x498d30: dword copy of stack arg 3
            *(float *)(vertex + 0x10) = u;
            *(float *)(vertex + 0x14) = v_uv;
            vertex_index = vertex_index + 1;
            corner_pair = corner_pair + 2;
        } while (axis < 4);

        for (i = 0; i < 0x23; i++) {
            ((int32_t *)&state)[i] = 0;
        }
        state.meter_parameters = 0;
        state.single_local_player = 0;
        state.framebuffer_blend_function = 0;
        state.maps[0] = (BitmapData *)bitmap_data;
        state.map_texel_scales[0].y = 1.0f;
        state.map_texel_scales[0].x = 1.0f;
        state.map_scales[0].y = 1.0f;
        state.map_scales[0].x = 1.0f;

        rasterizer_ui_quad_draw(&state, submit_buffer);
    }
}

#if 0
Original Ghidra decompilation (0x498b20):

void FUN_00498b20(int param_1,short *param_2,float param_3)

{
  ushort uVar1;
  short sVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  int *in_EAX;
  short *in_ECX;
  float *pfVar7;
  short sVar8;
  float *pfVar9;
  int iVar10;
  int *piVar11;
  int local_12c;
  undefined2 local_128;
  undefined2 local_126;
  int local_124;
  uint local_120;
  float local_11c [5];
  float local_108;
  float local_104;
  float local_100;
  undefined4 local_fc;
  undefined4 local_f8;
  undefined4 local_f4;
  undefined4 local_f0;
  int local_ec [10];
  undefined4 local_c4;
  undefined4 local_c0;
  undefined4 local_ac;
  undefined4 local_a8;
  undefined2 local_64;
  undefined1 local_62;
  undefined1 local_60 [16];
  float local_50 [20];

  if ((param_1 != 0) && (in_ECX != (short *)0x0)) {
    local_f8 = 0x3f666666;
    local_f4 = 0x3f666666;
    local_f0 = 0x3f666666;
    local_fc = 0x3f666666;
    if (in_EAX == (int *)0x0) {
      local_126 = *(undefined2 *)(param_1 + 4);
      local_12c = 0;
      local_128 = *(undefined2 *)(param_1 + 6);
      in_EAX = &local_12c;
    }
    uVar1 = in_ECX[3];
    sVar8 = in_ECX[1];
    sVar2 = *in_ECX;
    local_120 = (uint)uVar1;
    local_11c[0] = (float)(int)sVar8;
    local_11c[1] = (float)(int)sVar2;
    local_11c[2] = (float)((int)(short)(uVar1 - sVar8) + (int)sVar8);
    local_12c = (int)(short)(in_ECX[2] - sVar2) + (int)sVar2;
    local_11c[3] = local_11c[1];
    local_11c[4] = local_11c[2];
    local_108 = (float)local_12c;
    local_104 = local_11c[0];
    local_100 = local_108;
    if (param_2 != (short *)0x0) {
      sVar8 = param_2[1];
      if (in_ECX[1] < sVar8) {
        local_12c = (int)sVar8;
        local_104 = (float)(int)sVar8;
        local_11c[0] = local_104;
      }
      sVar8 = param_2[3];
      if (sVar8 < (short)uVar1) {
        local_120 = (int)sVar8;
        local_11c[4] = (float)(int)sVar8;
        local_11c[2] = local_11c[4];
      }
      sVar8 = *param_2;
      if (sVar2 < sVar8) {
        local_120 = (int)sVar8;
        local_11c[3] = (float)(int)sVar8;
        local_11c[1] = local_11c[3];
      }
      if (param_2[2] < in_ECX[2]) {
        local_100 = (float)(int)param_2[2];
        local_108 = local_100;
      }
    }
    local_124 = (int)*(short *)(param_1 + 4);
    fVar5 = (float)(int)*(short *)(param_1 + 4);
    if (fVar5 < 1.0) {
      fVar5 = 1.0;
    }
    fVar5 = (float)(int)(short)(*(short *)((int)in_EAX + 6) - *(short *)((int)in_EAX + 2)) / fVar5;
    if (1.0 < fVar5) {
      fVar5 = 1.0;
    }
    fVar6 = (float)(int)*(short *)(param_1 + 6);
    if (fVar6 < 1.0) {
      fVar6 = 1.0;
    }
    fVar6 = (float)(int)(short)((short)in_EAX[1] - (short)*in_EAX) / fVar6;
    if (1.0 < fVar6) {
      fVar6 = 1.0;
    }
    sVar8 = 0;
    iVar10 = 0;
    pfVar9 = local_11c + 1;
    pfVar7 = local_50;
    do {
      pfVar7[-1] = param_3;
      fVar4 = fVar5;
      if (iVar10 % 3 == 0) {
        fVar4 = 0.0;
      }
      *pfVar7 = fVar4;
      fVar4 = fVar6;
      if (sVar8 < 2) {
        fVar4 = 0.0;
      }
      fVar3 = pfVar9[-1];
      pfVar7[1] = fVar4;
      fVar4 = *pfVar9;
      sVar8 = sVar8 + 1;
      pfVar7[-4] = fVar3;
      pfVar7[-3] = fVar4;
      pfVar7[-2] = 0.0;
      iVar10 = iVar10 + 1;
      pfVar9 = pfVar9 + 2;
      pfVar7 = pfVar7 + 6;
    } while (sVar8 < 4);
    piVar11 = local_ec;
    for (iVar10 = 0x23; iVar10 != 0; iVar10 = iVar10 + -1) {
      *piVar11 = 0;
      piVar11 = piVar11 + 1;
    }
    local_ec[0] = 0;
    local_62 = 0;
    local_64 = 0;
    local_ec[3] = param_1;
    local_a8 = 0x3f800000;
    local_ac = 0x3f800000;
    local_c0 = 0x3f800000;
    local_c4 = 0x3f800000;
    local_11c[0] = local_104;
    local_11c[1] = local_11c[3];
    local_11c[2] = local_11c[4];
    local_108 = local_100;
    FUN_0051c9a0(local_60);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
