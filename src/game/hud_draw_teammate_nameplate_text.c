// hud_draw_teammate_nameplate_text  (Ghidra: FUN_00461f20; renamed per its summary)
// address 0x461f20, size 220 bytes
// name confidence: 0.4   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Draws the teammate nameplate's name and value text at
// a fixed position on the HUD"); reuses the shared HUD text draw-state globals already named in
// src/game/hud_draw_scoreboard_row_text.c and src/game/hud_draw_world_relative_text.c
// (hud_text_draw_font_tag_id @0x006e472c, hud_text_draw_unknown_4730 @0x006e4730,
// hud_text_draw_color_or_flags @0x006e4734, hud_text_draw_color_alpha @0x006e4738 (text.h text_color.alpha),
// hud_text_draw_color_r/g/b @0x006e473c/4740/4744, hud_text_draw_background_mode @0x006e4748);
// types/tags.h Globals::interface_bitmaps (+0x140) -> GlobalsInterfaceBitmaps::font_terminal
// (its tag_id at +0x1c, same anchor as hud_draw_world_relative_text.c).
// register convention: no register-passed arguments; param_1/param_2 are this function's own
// stack parameters.
// CORRECTED (phase 4 review, objdump 0x461f97..0x461fce): the rasterizer takes only three stack
// arguments (0, 0, text) plus EAX = 0 and ECX = &bounds. The 0x46 / 0x5e / 0x278 constants
// Ghidra prints as extra arguments are int16 fields of that hud_text_bounds block
// (top / bottom / right); `left` is left UNINITIALIZED by this function, which is what the
// binary does. The former "value" sixth argument does not exist -- `value` reaches the
// rasterizer only through the 0x006e4738 draw-state slot.
// UNSURE (historical): chimera__draw_16_bit_text appeared to be called here with SIX arguments (two more than the
// five-argument shape hud_draw_scoreboard_row_text.c established, and with plain small integer
// literals rather than that call's packed 16.16 values); modeled with its own six-argument
// prototype for this file only, per this codebase's established per-call-site convention.
// reconciled: R36 0x006e4738 extern renamed hud_text_draw_color_alpha (float, text.h text_color.alpha); store kept bit-exact

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

extern Globals *global_globals; // 0x00746fa0

extern int32_t hud_text_draw_font_tag_id;     // 0x006e472c
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730
extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, two separate int16 slots in the
extern int16_t hud_text_draw_column;         // 0x006e4736  binary, never one dword
extern float hud_text_draw_color_alpha;       // 0x006e4738, text.h text_color.alpha
extern float hud_text_draw_color_r;           // 0x006e473c
extern float hud_text_draw_color_g;           // 0x006e4740
extern float hud_text_draw_color_b;           // 0x006e4744
extern int16_t hud_text_draw_background_mode; // 0x006e4748

extern int32_t chimera__draw_16_bit_text(int32_t unknown_0, int32_t unknown_1, wchar_t *text);
    // 0x514ab0; blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
    // stack -> (unknown_0, unknown_1, text)

// Draws `text` through the globals terminal font at a fixed screen position, with `value`
// forwarded both into the shared draw state and as the rasterizer's own sixth argument. Resets
// the background mode to none afterward.
void hud_draw_teammate_nameplate_text(wchar_t *text, int32_t value)
{
    hud_text_bounds bounds;
    GlobalsInterfaceBitmaps *interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
        ? (GlobalsInterfaceBitmaps *)0
        : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;

    hud_text_draw_font_tag_id = (int32_t)interface_bitmaps->font_terminal.tag_id.index |
                                 ((int32_t)interface_bitmaps->font_terminal.tag_id.id << 16);
    hud_text_draw_unknown_4730 = 8;
    hud_text_draw_color_r = 0.45882353f; // 0x3eeaeaeb
    *(int32_t *)&hud_text_draw_color_alpha = value; // raw dword store, as the binary does
    hud_text_draw_color_b = 1.0f; // 0x3f800000
    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = 2;
    hud_text_draw_color_g = 0.7294118f; // 0x3f3ababb

    bounds.top = 0x46;
    bounds.bottom = 0x5e;
    bounds.right = 0x278;
    // bounds.left is deliberately NOT written here -- the binary leaves that slot untouched.
    // blam-cc: EAX = 0, ECX = &bounds
    chimera__draw_16_bit_text(0, 0, text);

    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;
    hud_text_draw_background_mode = 0;
}

#if 0
Original Ghidra decompilation (0x461f20), from tools/pack.py 0x461f20:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00461f20(undefined4 param_1,undefined4 param_2)

{
  int iVar1;

  if (*(int *)(DAT_00746fa0 + 0x140) == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = *(int *)(DAT_00746fa0 + 0x144);
  }
  DAT_006e472c = *(undefined4 *)(iVar1 + 0x1c);
  _DAT_006e4730 = 8;
  DAT_006e473c = 0x3eeaeaeb;
  DAT_006e4738 = param_2;
  DAT_006e4744 = 0x3f800000;
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 2;
  DAT_006e4740 = 0x3f3ababb;
  chimera__draw_16_bit_text(0,0,param_1,0x46,0x5e,param_2);
  DAT_006e4734._0_2_ = 0xffff;
  DAT_006e4734._2_2_ = 0;
  _DAT_006e4730 = 0;
  DAT_006e4748 = 0;
  return;
}
#endif
