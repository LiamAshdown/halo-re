// hud_draw_world_relative_text  (Ghidra: FUN_004653f0; named per out/phase4/game_functions.md:
// "Renders a world-relative HUD text label (with optional highlighted background and
// distance-based scaling/fade) via the engine 16-bit text rasterizer.")
// address 0x4653f0, size 474 bytes
// name confidence: 0.3   rewrite confidence: 0.45
// evidence: reuses the shared HUD text draw-state globals already named in
//   src/game/hud_draw_scoreboard_row_text.c (0x006e4734 hud_text_draw_color_or_flags,
//   0x006e4730 hud_text_draw_unknown_4730, 0x007c3140/3148/314c screen_safe_area_*), plus
//   types/tags.h Globals::interface_bitmaps (TagReflexive at +0x140) -> GlobalsInterfaceBitmaps,
//   whose second TagDependency (font_terminal, at +0x10) supplies the tag_id read at +0x1c.
//   Ghidra's decompile is followed for control flow and for the three global-state writes, all
//   of which match the disassembly (objdump -d -M intel --start-address=0x4653f0
//   --stop-address=0x4655d0) one for one, EXCEPT one bug it introduces: `(float)in_EAX[1..3]`
//   is a numeric int-to-float CAST in Ghidra's text, but the disassembly only ever moves those
//   dwords with plain `mov`/`fld` (never `cvtsi2ss`/`fild`), so the three fields are already
//   IEEE-754 floats in memory; the params struct below reads them as floats directly. The
//   disassembly also computes a packed 16.16 "row rect" value from EDX (a param Ghidra does not
//   surface as a real input either) into scratch stack slots that are never stored to a global
//   and never reach the call below (which pushes exactly 0, 0, text -- confirmed by the
//   "add esp,0xc" cleanup after it) -- Ghidra's own dataflow analysis drops that computation
//   entirely, and this rewrite does too rather than guess at a role for it.
// register convention: a pointer to a hud_world_text_params block in EAX, a row index in EDX,
//   plus the two original stack parameters (text, highlighted).
//   // blam-cc: EAX -> params, EDX -> row, stack -> (text, highlighted)
// CORRECTED (phase 4 review, objdump --start-address=0x4653f0 --stop-address=0x4655ca): the
// first pass dropped EDX and therefore (a) armed the background box unconditionally and (b)
// threw away the row rectangle. Both are wrong:
//   - 0x46543b `cmp edx,ebx` / 0x46546e `je 0x46549f`: when row == 0 the function writes only
//     background_mode = 0 and draws NO box; the four box constants are copied to the draw state
//     only when row != 0.
//   - 0x465555 `imul edx,edx,0xf` / `add edx,0x3b`: row * 15 + 0x3b is the rect's top, +0x11 its
//     bottom, and 0x465588 `lea ecx,[esp+0x18]` passes that rect to the rasterizer in ECX. The
//     rect is a real argument, not dead scratch.
//   - 0x465586 `xor eax,eax` is the rasterizer's EAX argument; the call's own return value in
//     EAX is this function's return value on the success path.
// UNSURE: this function's own name and every field of the 0x006e472c..0x006e4756 draw-state
//   block beyond the two already named elsewhere are unresolved. The background-box geometry
//   constants are kept as the literal int16 pairs the disassembly stores.
// reconciled: R36 hud_world_text_params is ColorARGB-shaped: unknown_00 -> float alpha, color_r/g/b -> red/green/blue; 0x006e4738 extern is float alpha

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern Globals *global_globals; // 0x00746fa0

extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730, always zeroed here
extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c, the resolved font_terminal tag id
extern float hud_text_draw_color_a;       // 0x006e4738, text.h text_color.alpha, copied from params->alpha
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744
extern int16_t hud_text_draw_background_mode; // 0x006e4748, 0 = none, 7 = boxed (UNSURE)
extern uint32_t text_tab_stops; // 0x006e474a, UNSURE: background box geometry
extern uint32_t hud_text_draw_box_field_474e; // 0x006e474e, UNSURE: background box geometry
extern uint32_t hud_text_draw_tabstop_c; // 0x006e4752, UNSURE: background box geometry
extern int16_t hud_text_draw_box_field_4756;  // 0x006e4756, UNSURE: background box geometry

extern uint32_t render_viewport_top; // 0x007c3140, high16 subtracted from both anchors
extern uint32_t screen_safe_area_right;  // 0x007c3148, high16 is the rect's `left`
extern uint32_t screen_safe_area_bottom; // 0x007c314c, high16 is the rect's `right`

extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text); // 0x514ab0, EAX clip, ECX dest rect, stack (0, 0, text)
    // 0x514ab0; blam-cc: EAX -> unknown (0 at both of this module's call sites),
    // ECX -> bounds (hud_text_bounds *), stack -> (unknown_0, unknown_1, text). Declared with a
    // return value because the success path below returns whatever this call leaves in EAX.

// blam-cc: EAX -> params, EDX -> row, stack -> (text, highlighted)
// Draws one row of world-relative HUD text through the globals terminal font, with the fixed
// background box armed only when `row` is non-zero, and with its colour pushed toward white when
// `highlighted` is set. Does nothing (beyond clearing the background mode) if the globals tag has
// no font_terminal assigned.
int32_t hud_draw_world_relative_text(hud_world_text_params *params, int16_t row, wchar_t *text,
                                      uint8_t highlighted)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    int32_t font_terminal_id; // TagID, read as a raw int32 (matches the -1 sentinel compare)
    real r, g, b;
    int16_t safe_left;
    hud_text_bounds bounds;
    int32_t result;

    r = params->red;
    g = params->green;
    b = params->blue;

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    font_terminal_id = (int32_t)interface_bitmaps->font_terminal.tag_id.index |
                       ((int32_t)interface_bitmaps->font_terminal.tag_id.id << 16);

    if (row != 0) {
        // The four stores the disassembly makes, as the int16 pairs it actually writes:
        // 0x006e474a = (0x19, 0x5a), 0x006e474e = (0x118, 0x159), 0x006e4752 = (0x19a, 0x1e5),
        // 0x006e4756 = 0x230, with background_mode = 7 in between.
        text_tab_stops = 0x005a0019u;
        hud_text_draw_box_field_474e = 0x01590118u;
        hud_text_draw_background_mode = 7;
        hud_text_draw_tabstop_c = 0x01e5019au;
        hud_text_draw_box_field_4756 = 0x230;
    } else {
        hud_text_draw_background_mode = 0;
    }

    safe_left = (int16_t)(render_viewport_top >> 16);
    bounds.left = (int16_t)((screen_safe_area_right >> 16) - (uint16_t)safe_left);
    bounds.right = (int16_t)((screen_safe_area_bottom >> 16) - (uint16_t)safe_left);
    bounds.top = (int16_t)(row * 0x0f + 0x3b);
    bounds.bottom = (int16_t)(bounds.top + 0x11);

    result = -(int32_t)render_viewport_top;
    if (font_terminal_id != -1) {
        if (highlighted != 0) {
            r = r + 0.4f;
            g = g + 0.4f;
            b = b + 0.4f;
            if (r > 1.0f) r = 1.0f;
            if (g > 1.0f) g = 1.0f;
            if (b > 1.0f) b = 1.0f;
        }
        hud_text_draw_color_r = r;
        hud_text_draw_color_g = g;
        hud_text_draw_color_b = b;
        hud_text_draw_color_or_flags = 0xffffu;
        hud_text_draw_column = 0;
        hud_text_draw_unknown_4730 = 0;
        hud_text_draw_font_tag_id = font_terminal_id;
        hud_text_draw_color_a = params->alpha;
        // blam-cc: EAX = 0, ECX = &bounds
        chimera__draw_16_bit_text(0, (int32_t *)&bounds, 0, 0, (const int16_t *)text);
    }

    hud_text_draw_background_mode = 0;
    return result;
}

#if 0
Original Ghidra decompilation (0x4653f0), from tools/pack.py 0x4653f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_004653f0(undefined4 param_1,char param_2)

{
  undefined4 *in_EAX;
  int iVar1;
  int iVar2;
  int in_EDX;
  float local_c;
  float local_8;
  float local_4;

  local_c = (float)in_EAX[1];
  local_8 = (float)in_EAX[2];
  local_4 = (float)in_EAX[3];
  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(DAT_00746fa0 + 0x144);
  }
  if (in_EDX == 0) {
    DAT_006e4748 = 0;
  }
  else {
    _DAT_006e474a = 0x5a0019;
    _DAT_006e474e = 0x1590118;
    DAT_006e4748 = 7;
    _DAT_006e4752 = 0x1e5019a;
    _DAT_006e4756 = 0x230;
  }
  iVar2 = -DAT_007c3140;
  if (*(int *)(iVar1 + 0x1c) != -1) {
    if (param_2 != '\0') {
      local_c = local_c + 0.4;
      local_8 = local_8 + 0.4;
      local_4 = local_4 + 0.4;
      if (1.0 < local_c) {
        local_c = 1.0;
      }
      if (1.0 < local_8) {
        local_8 = 1.0;
      }
      if (1.0 < local_4) {
        local_4 = 1.0;
      }
    }
    DAT_006e473c = local_c;
    DAT_006e4740 = local_8;
    DAT_006e4744 = local_4;
    DAT_006e4734._0_2_ = 0xffff;
    DAT_006e4734._2_2_ = 0;
    _DAT_006e4730 = 0;
    DAT_006e472c = *(int *)(iVar1 + 0x1c);
    DAT_006e4738 = *in_EAX;
    iVar2 = chimera__draw_16_bit_text(0,0,param_1);
  }
  DAT_006e4748 = 0;
  return iVar2;
}

Raw disassembly (objdump -d -M intel --start-address=0x4653f0 --stop-address=0x4655d0), kept
for the parts of the trace that motivated the fixes above; the EDX/row-rect scratch computation
between 0x4654a6 and 0x465583 is omitted from the rewrite as unreachable-in-effect (see header):

004653f0:  sub esp,0x28
004653f3:  mov ecx,[eax+0x4]
004653f6:  push ebx
004653f7:  push esi
004653f8:  mov [esp+0x24],ecx
004653fc:  mov ecx,[eax+0x8]
004653ff:  push edi
00465400:  mov edi,[eax]
00465402:  mov eax,[eax+0xc]
00465405:  mov [esp+0x30],eax
00465409:  mov eax,ds:0x7c314c
0046540e:  mov [esp+0x2c],ecx
00465412:  mov ecx,ds:0x7c3148
00465418:  mov [esp+0x10],eax
0046541c:  mov eax,ds:0x746fa0
00465421:  mov [esp+0xc],ecx
00465425:  mov ecx,[eax+0x140]
0046542b:  xor ebx,ebx
0046542d:  cmp ecx,ebx
0046542f:  je 0x465439
00465431:  mov eax,[eax+0x144]
00465437:  jmp 0x46543b
00465439:  xor eax,eax
0046543b:  cmp edx,ebx
0046543d:  mov esi,[eax+0x1c]
... (background-box constant stores and the EDX/row-rect scratch, see header)
004654d1:  cmp esi,0xffffffff
004654d4:  je 0x4655bc
004654da:  cmp byte ptr [esp+0x3c],bl
004654de:  je 0x465551
004654e0:  fld dword ptr [esp+0x28]
004654e4:  fadd dword ptr ds:0x672bc8      ; 0.4
004654ea:  fstp dword ptr [esp+0x28]
... (same +0.4 for [esp+0x2c] and [esp+0x30])
0046550a:  fld dword ptr [esp+0x28]
0046550e:  fcomp dword ptr ds:0x672ac4     ; 1.0
00465514:  fnstsw ax
00465516:  test ah,0x41
00465519:  jne 0x465523
0046551b:  mov dword ptr [esp+0x28],0x3f800000   ; 1.0
... (same clamp for [esp+0x2c] and [esp+0x30])
00465568:  mov ds:0x6e473c,eax
0046557e:  mov ds:0x6e4740,ecx
00465571:  push eax            ; text
00465584:  push ebx            ; 0
00465585:  push ebx            ; 0
0046558c:  mov ds:0x6e472c,esi
00465592:  mov ds:0x6e4738,edi
00465598:  mov ds:0x6e4744,edx
0046559e:  mov word ptr ds:0x6e4734,0xffff
004655a7:  mov word ptr ds:0x6e4736,bx
004655ae:  mov dword ptr ds:0x6e4730,ebx
004655b4:  call 0x514ab0
004655b9:  add esp,0xc
004655bc:  pop edi
004655bd:  pop esi
004655be:  mov word ptr ds:0x6e4748,bx
004655c5:  pop ebx
004655c6:  add esp,0x28
004655c9:  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
