// chimera__draw_16_bit_text  (Ghidra: chimera__draw_16_bit_text, already named -- Chimera name,
// hint only)
// address 0x514ab0, size 511 bytes
// name confidence: 0.55  rewrite confidence: 0.85 (FIXED: glyph state +0x28/+0x2c = 1.0 and +0x40/+0x44 = 1/atlas size (c17 texel scale); rest verified against the binary (gates, dest/clip rects, text_wrap_and_draw call))
// evidence: wide-character sibling of chimera__draw_8_bit_text @0x5148b0 (see that file for the
//   shared UNSURE caveats: the &LAB_00514ce0 glyph callback body is not in this pack, and the
//   007c3140..007c314c globals are undocumented). Differs by validating the string with
//   wcslen (likely a wide string length/bounds check, return unused) and by clamping its
//   caller-supplied clip rect to fixed maxima (0x1e0, 0x280) instead of the safe-area bounds.
// register convention: optional clip rect in in_EAX, optional dest rect override in in_ECX,
//   position/color and text on the stack. // blam-cc: EAX -> clip_rect_override(opt),
//   ECX -> dest_rect_override(opt), stack -> (position_or_color1, position_or_color2, text)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_689402;                         // 0x00689402
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern font_glyph_cache g_font_glyph_cache; // 0x006d8828
extern int32_t rasterizer_frame_index; // 0x0069c694

extern int16_t unknown_007c3140[2]; // 0x007c3140 UNSURE
extern int16_t unknown_007c3144[2]; // 0x007c3144 UNSURE
extern int16_t unknown_007c3148[2]; // 0x007c3148 UNSURE
extern int16_t unknown_007c314c[2]; // 0x007c314c UNSURE

// blam-cc: EDI -> state
extern void rasterizer_draw_text_begin(ui_quad_render_state *state); // 0x531b80
extern void rasterizer_draw_text_end(void); // 0x531e90
extern void wcslen(const int16_t *text); // 0x625b7a, UNSURE: return unused, likely a wide string check
extern void text_wrap_and_draw_wide(void *glyph_callback, void *dest_rect, uint32_t position_or_color1,
                          void *clip_rect, uint32_t position_or_color2, const int16_t *text); // 0x556780
extern void text_draw_glyph_callback(void *state, void *font, uint8_t *character, uint32_t color, int16_t x,
    int16_t y, int16_t source_x, int16_t source_y, int16_t width, int16_t height); // 0x514ce0
void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
                                uint32_t position_or_color1, uint32_t position_or_color2,
                                const int16_t *text)
{
    void *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    uint32_t glyph_state[0x23]; // UNSURE: exact field layout, see chimera__draw_8_bit_text.c
    int i;

    if (console_debug_toggle_689402 == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? (void *)g_font_glyph_cache.atlas : (void *)0;
    if (atlas == (void *)0 || *text == 0) {
        return;
    }

    wcslen(text); // UNSURE: return unused

    if (dest_rect_override == (int32_t *)0) {
        int16_t neg_origin_x = (int16_t)(-unknown_007c3140[0]);
        dest_rect[0] = (uint16_t)(int16_t)(unknown_007c3148[0] + neg_origin_x) |
                       ((uint16_t)(int16_t)(unknown_007c3148[1] - unknown_007c3140[1]) << 16);
        dest_rect[1] = (uint16_t)(int16_t)(unknown_007c314c[0] + neg_origin_x) |
                       ((uint16_t)(int16_t)(unknown_007c314c[1] - unknown_007c3140[1]) << 16);
    } else {
        dest_rect[0] = dest_rect_override[0];
        dest_rect[1] = dest_rect_override[1];
    }

    if (clip_rect_override == (Rectangle2D *)0) {
        clip_rect[0] = 0;
        clip_rect[1] = (uint16_t)(int16_t)(unknown_007c3144[0] - unknown_007c3140[0]) |
                       ((uint16_t)(int16_t)(unknown_007c3144[1] - unknown_007c3140[1]) << 16);
    } else {
        int16_t *r = (int16_t *)clip_rect_override;
        int16_t clip_w = (r[2] > 0x1df) ? 0x1e0 : r[2];
        int16_t clip_h = (r[3] > 0x27f) ? 0x280 : r[3];
        int16_t x0 = (r[0] < 0) ? 0 : r[0];
        int16_t y0 = (r[1] < 0) ? 0 : r[1];
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)clip_w | ((uint16_t)clip_h << 16);
    }

    for (i = 0; i < 0x23; i++) {
        glyph_state[i] = 0;
    }
    glyph_state[0] = 0;
    glyph_state[3] = (uint32_t)atlas;
    // FIXED (0x514c42..0x514c77 / 0x514a42..0x514a77): +0x28 / +0x2c (map_scales[0]) = 1.0 and
    //   +0x40 / +0x44 (map_texel_scales[0]) = 1 / atlas width, 1 / atlas height (atlas +0x04 / +0x06).
    //   rasterizer_draw_text_begin copies +0x40 / +0x44 into shader constant c17.xy, the scale that
    //   turns the callback's pixel texture coordinates into 0..1; left at 0 every glyph sampled one
    //   texel and the text drew invisibly (the a10 HUD help text).
    ((float *)glyph_state)[10] = 1.0f;
    ((float *)glyph_state)[11] = 1.0f;
    ((float *)glyph_state)[16] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 4);
    ((float *)glyph_state)[17] = 1.0f / (float)(int32_t)*(int16_t *)((uint8_t *)atlas + 6);

    rasterizer_draw_text_begin((ui_quad_render_state *)glyph_state); // EDI = &glyph_state
    text_wrap_and_draw_wide((void *)text_draw_glyph_callback, dest_rect, position_or_color1, clip_rect,
                 position_or_color2, text);
    rasterizer_draw_text_end();
}

#if 0
Original Ghidra decompilation (0x514ab0):

void chimera__draw_16_bit_text(undefined4 param_1,undefined4 param_2,short *param_3)

{
  short sVar1;
  short sVar2;
  short *in_EAX;
  short sVar3;
  undefined4 *in_ECX;
  int iVar4;
  short sVar5;
  uint *puVar6;
  undefined4 local_ac;
  short local_a8;
  short sStack_a6;
  undefined4 local_a4;
  undefined4 local_a0;
  uint local_9c;
  uint local_98 [10];
  undefined4 local_70;
  undefined4 local_6c;
  float local_58;
  float local_54;
  undefined2 local_10;
  undefined1 local_e;

  if ((DAT_00689402 != '\0') && ((short)DAT_007c1220 == 1)) {
    DAT_0069c694 = DAT_0069c694 + 1;
    local_9c = -(uint)(DAT_006d8828 != '\0') & DAT_006d8834;
    if ((local_9c != 0) && (*param_3 != 0)) {
      FUN_00625b7a(param_3);
      if (in_ECX == (undefined4 *)0x0) {
        local_a4._0_2_ = (short)DAT_007c3148;
        sVar1 = -(short)DAT_007c3140;
        local_a4 = CONCAT22((short)((uint)DAT_007c3148 >> 0x10) + -DAT_007c3140._2_2_,
                            (short)local_a4 + sVar1);
        local_a0._0_2_ = (short)DAT_007c314c;
        local_a0 = CONCAT22((short)((uint)DAT_007c314c >> 0x10) + -DAT_007c3140._2_2_,
                            (short)local_a0 + sVar1);
      }
      else {
        local_a4 = *in_ECX;
        local_a0 = in_ECX[1];
      }
      if (in_EAX == (short *)0x0) {
        local_a8 = (short)DAT_007c3144;
        local_ac = 0;
        _local_a8 = CONCAT22((short)((uint)DAT_007c3144 >> 0x10) - DAT_007c3140._2_2_,
                             local_a8 - (short)DAT_007c3140);
      }
      else {
        sVar1 = in_EAX[2];
        if (0x1df < sVar1) {
          sVar1 = 0x1e0;
        }
        sVar5 = in_EAX[3];
        if (0x27f < sVar5) {
          sVar5 = 0x280;
        }
        sVar3 = *in_EAX;
        if (sVar3 < 0) {
          sVar3 = 0;
        }
        sVar2 = in_EAX[1];
        if (sVar2 < 0) {
          sVar2 = 0;
        }
        local_ac = CONCAT22(sVar2,sVar3);
        _local_a8 = CONCAT22(sVar5,sVar1);
      }
      puVar6 = local_98;
      for (iVar4 = 0x23; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      local_58 = 1.0 / (float)(int)*(short *)(local_9c + 4);
      local_6c = 0x3f800000;
      local_70 = 0x3f800000;
      local_98[0] = 0;
      local_e = 0;
      local_10 = 0;
      local_98[3] = local_9c;
      local_54 = 1.0 / (float)(int)*(short *)(local_9c + 6);
      local_9c = (int)*(short *)(local_9c + 6);
      FUN_00531b80();
      FUN_00556780(&LAB_00514ce0,&local_a4,param_1,&local_ac,param_2,param_3);
      FUN_00531e90();
    }
  }
  return;
}
#endif
