// chimera__draw_8_bit_text  (Ghidra: chimera__draw_8_bit_text, already named -- Chimera name,
// hint only)
// address 0x5148b0, size 511 bytes
// name confidence: 0.55  rewrite confidence: 0.2
// evidence: gates on a debug-text toggle and window.type == 1, bumps the font atlas frame stamp
//   (rasterizer_frame_index, 0x0069c694), resolves the g_font_glyph_cache.atlas pointer, builds a
//   destination rect (from the optional in_ECX override or a global text-safe-area rect) and a
//   clip rect (from the optional in_EAX override or the same global rect), then drives the
//   actual glyph layout/draw through text_wrap_and_draw_narrow.
// register convention: dest position/color in param_1/param_2 (never dereferenced here, only
//   forwarded), text in param_3 (recognized), optional clip rect in in_EAX, optional dest rect
//   override in in_ECX. // blam-cc: EAX -> clip_rect(opt), ECX -> dest_rect_override(opt),
//   stack -> (position_or_color1, position_or_color2, text)
// UNSURE, substantially: text_wrap_and_draw_narrow is called with a function pointer &LAB_00514ce0 as its
//   first argument -- that label is a callback embedded inside this same original function's
//   machine code (a per-glyph draw callback Ghidra did not surface as its own decompiled
//   function), so its body is not visible in this pack at all. The ~0x8c byte local buffer
//   (local_98..local_e) that rasterizer_draw_text_begin/rasterizer_draw_text_end bracket is very likely a font draw
//   context built from g_font_glyph_cache.atlas fields, but its exact field layout could not be
//   reconstructed byte-for-byte from the decompiled text; the parts that need it are kept as a
//   raw buffer with the two clearly-identified writes at glyph_state[0] and glyph_state[3]
//   (atlas pointer) and everything else zeroed, matching the observable "zero this many dwords"
//   behaviour without asserting field meanings this pack does not evidence. The four
//   007c3140..007c314c globals are not documented anywhere in types/rasterizer.h; they are
//   modeled here as a 4-Rectangle2D-style block of raw int16 pairs with UNSURE names.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "rasterizer.h"

extern uint8_t console_debug_toggle_689402;                         // 0x00689402
extern rasterizer_window_parameters rasterizer_window;              // 0x007c1220
extern font_glyph_cache g_font_glyph_cache; // 0x006d8828
extern int32_t rasterizer_frame_index; // 0x0069c694

// UNSURE: undocumented globals, guessed as two int16 pairs (Point2D-shaped) each
extern int16_t unknown_007c3140[2]; // 0x007c3140
extern int16_t unknown_007c3144[2]; // 0x007c3144
extern int16_t unknown_007c3148[2]; // 0x007c3148
extern int16_t unknown_007c314c[2]; // 0x007c314c

// blam-cc: EDI -> state
extern void rasterizer_draw_text_begin(ui_quad_render_state *state); // 0x531b80
extern void rasterizer_draw_text_end(void); // 0x531e90
extern void text_wrap_and_draw_narrow(void *glyph_callback, void *dest_rect, uint32_t position_or_color1,
                          void *clip_rect, uint32_t position_or_color2, const char *text); // 0x556400
extern void LAB_00514ce0_glyph_callback(void); // UNSURE: an internal label of this same
                                                // original function, not a separate Ghidra
                                                // function -- its body is not in this pack

// blam-cc: EAX -> clip_rect(opt), ECX -> dest_rect_override(opt),
// stack -> (position_or_color1, position_or_color2, text)
// Draws an 8-bit (single-byte character) debug text string through the shared glyph layout
// driver text_wrap_and_draw_narrow, using either the caller-supplied clip/dest rects or the global text safe
// area when they are NULL.
void chimera__draw_8_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
                               uint32_t position_or_color1, uint32_t position_or_color2,
                               const char *text)
{
    void *atlas;
    int32_t dest_rect[2];
    int32_t clip_rect[2];
    uint32_t glyph_state[0x23]; // UNSURE: exact field layout, see file header
    int i;

    if (console_debug_toggle_689402 == 0 || rasterizer_window.type != 1) {
        return;
    }

    rasterizer_frame_index = rasterizer_frame_index + 1;
    atlas = (g_font_glyph_cache.initialized != 0) ? (void *)g_font_glyph_cache.atlas : (void *)0;
    if (atlas == (void *)0 || *text == '\0') {
        return;
    }

    if (dest_rect_override == (int32_t *)0) {
        dest_rect[0] = ((int32_t)unknown_007c3148[0] - unknown_007c3140[0]) |
                       (((int32_t)unknown_007c3148[1] - unknown_007c3140[1]) << 16);
        dest_rect[1] = ((int32_t)unknown_007c314c[0] - unknown_007c3140[0]) |
                       (((int32_t)unknown_007c314c[1] - unknown_007c3140[1]) << 16);
    } else {
        dest_rect[0] = dest_rect_override[0];
        dest_rect[1] = dest_rect_override[1];
    }

    if (clip_rect_override == (Rectangle2D *)0) {
        clip_rect[0] = 0;
        clip_rect[1] = ((int32_t)unknown_007c3144[0] - unknown_007c3140[0]) |
                       (((int32_t)unknown_007c3144[1] - unknown_007c3140[1]) << 16);
    } else {
        int16_t *r = (int16_t *)clip_rect_override;
        int32_t width = unknown_007c3144[0] - unknown_007c3140[0];
        int32_t height = unknown_007c3144[1] - unknown_007c3140[1];
        int32_t clip_w = (r[2] < width) ? r[2] : width;
        int32_t clip_h = (r[3] < height) ? r[3] : height;
        int16_t x0 = (r[0] < 0) ? 0 : r[0];
        int16_t y0 = (r[1] < 0) ? 0 : r[1];
        clip_rect[0] = (uint16_t)x0 | ((uint16_t)y0 << 16);
        clip_rect[1] = (uint16_t)(int16_t)clip_w | ((uint16_t)(int16_t)clip_h << 16);
    }

    for (i = 0; i < 0x23; i++) {
        glyph_state[i] = 0;
    }
    glyph_state[0] = 0;
    glyph_state[3] = (uint32_t)atlas;

    rasterizer_draw_text_begin((ui_quad_render_state *)glyph_state); // EDI = &glyph_state
    text_wrap_and_draw_narrow((void *)LAB_00514ce0_glyph_callback, dest_rect, position_or_color1, clip_rect,
                 position_or_color2, text);
    rasterizer_draw_text_end();
}

#if 0
Original Ghidra decompilation (0x5148b0):

void chimera__draw_8_bit_text(undefined4 param_1,undefined4 param_2,char *param_3)

{
  short *in_EAX;
  int iVar1;
  undefined4 *in_ECX;
  int iVar2;
  uint uVar3;
  short sVar4;
  short sVar5;
  uint *puVar6;
  undefined4 local_ac;
  undefined4 local_a8;
  int local_a4;
  undefined4 local_a0;
  undefined4 local_9c;
  uint local_98 [10];
  undefined4 local_70;
  undefined4 local_6c;
  float local_58;
  float local_54;
  undefined2 local_10;
  undefined1 local_e;

  if ((DAT_00689402 != '\0') && ((short)DAT_007c1220 == 1)) {
    DAT_0069c694 = DAT_0069c694 + 1;
    uVar3 = -(uint)(DAT_006d8828 != '\0') & DAT_006d8834;
    if ((uVar3 != 0) && (*param_3 != '\0')) {
      sVar5 = (short)DAT_007c3140;
      if (in_ECX == (undefined4 *)0x0) {
        local_a0._0_2_ = (short)DAT_007c3148;
        local_a0 = CONCAT22((short)((uint)DAT_007c3148 >> 0x10) + -DAT_007c3140._2_2_,
                            (short)local_a0 + -sVar5);
        local_9c._0_2_ = (short)DAT_007c314c;
        local_9c = CONCAT22((short)((uint)DAT_007c314c >> 0x10) + -DAT_007c3140._2_2_,
                            (short)local_9c + -sVar5);
      }
      else {
        local_a0 = *in_ECX;
        local_9c = in_ECX[1];
      }
      if (in_EAX == (short *)0x0) {
        local_ac = 0;
        local_a8 = CONCAT22(DAT_007c3144._2_2_ - DAT_007c3140._2_2_,(short)DAT_007c3144 - sVar5);
      }
      else {
        iVar2 = (int)(short)DAT_007c3144 - (int)sVar5;
        if ((int)in_EAX[2] < (int)(short)DAT_007c3144 - (int)sVar5) {
          iVar2 = (int)in_EAX[2];
        }
        iVar1 = (int)DAT_007c3144._2_2_ - (int)DAT_007c3140._2_2_;
        if ((int)in_EAX[3] < (int)DAT_007c3144._2_2_ - (int)DAT_007c3140._2_2_) {
          iVar1 = (int)in_EAX[3];
        }
        sVar5 = *in_EAX;
        if (sVar5 < 0) {
          sVar5 = 0;
        }
        sVar4 = in_EAX[1];
        if (sVar4 < 0) {
          sVar4 = 0;
        }
        local_ac = CONCAT22(sVar4,sVar5);
        local_a8 = CONCAT22((short)iVar1,(short)iVar2);
      }
      puVar6 = local_98;
      for (iVar2 = 0x23; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar6 = 0;
        puVar6 = puVar6 + 1;
      }
      local_a4 = (int)*(short *)(uVar3 + 6);
      local_58 = 1.0 / (float)(int)*(short *)(uVar3 + 4);
      local_6c = 0x3f800000;
      local_70 = 0x3f800000;
      local_98[0] = 0;
      local_e = 0;
      local_10 = 0;
      local_54 = 1.0 / (float)local_a4;
      local_98[3] = uVar3;
      FUN_00531b80();
      FUN_00556400(&LAB_00514ce0,&local_a0,param_1,&local_ac,param_2,param_3);
      FUN_00531e90();
    }
  }
  return;
}
#endif
