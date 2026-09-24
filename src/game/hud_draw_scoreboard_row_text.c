// hud_draw_scoreboard_row_text  (Ghidra: FUN_0045d670; named for its only caller,
// game_engine_post_rasterize_post_game, which prints one scoreboard row of text per call)
// address 0x45d670, size 135 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Draws a short piece of HUD text anchored to a screen
// corner using the engine's shared 16-bit text drawing state"); callers in this batch
// (game_engine_post_rasterize_post_game, hud_update_teammate_nameplate_fade) always pass 0 for
// the second parameter and vary the row instead through CX, a register this function reads but
// never receives as a named Ghidra parameter.
// register convention: row index in CX (in_CX, used only as a vertical-spacing multiplier),
// ordered first per the register-then-stack convention; text and column are this function's own
// explicit stack parameters, kept in their original order after it.
//   // blam-cc: CX -> row, stack -> text, column
// UNSURE: DAT_007c3140/3148/314c are some other module's screen-safe-area globals (packed as two
// int16 halves each); their exact roles are not established anywhere in this module's evidence.
// CORRECTED (phase 4 review, objdump --start-address=0x45d670 --stop-address=0x45d6f7): the two
// CONCAT22 values Ghidra prints as the 4th and 5th arguments are NOT stack arguments. The
// function builds them as one 8-byte hud_text_bounds on its own stack and passes its address in
// ECX (`lea ecx,[esp+0xc]` at 0x45d6d2), with EAX = 0 and only three pushed arguments
// (`add esp,0x14` cleans 3 args plus the 8-byte local). The same shape appears at
// hud_draw_world_relative_text's call site, which is what makes the field order readable.
// Also: the `column` argument is stored straight to 0x006e4736 (a separate int16 half of the
// 0x006e4734 dword), which is how the disassembly writes it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern int32_t chimera__draw_16_bit_text(int32_t unknown_0, int32_t unknown_1, wchar_t *text);
    // 0x514ab0; blam-cc: EAX -> unknown (0 here), ECX -> bounds (hud_text_bounds *),
    // stack -> (unknown_0, unknown_1, text)

extern uint16_t hud_text_draw_color_or_flags; // 0x006e4734, always 0xffff here
extern int16_t hud_text_draw_column;          // 0x006e4736, the `column` argument
extern uint32_t hud_text_draw_unknown_4730;   // 0x006e4730, always zeroed here
extern uint32_t screen_safe_area_origin;      // 0x007c3140, high16 subtracted from both corners below
extern uint32_t screen_safe_area_right;       // 0x007c3148, high16 is the right anchor x
extern uint32_t screen_safe_area_bottom;      // 0x007c314c, high16 is the bottom anchor y

// blam-cc: CX -> row, stack -> text, column (original stack order)
// Draws `text` right-aligned near the top of the screen, `row` line-heights (0x12 px) down from
// a 0x1a px top margin, using the shared 16-bit text drawing state (`column` selects which of
// several preset color/size slots that state holds -- see the UNSURE above).
void hud_draw_scoreboard_row_text(int16_t row, wchar_t *text, int16_t column)
{
    int16_t safe_left;
    hud_text_bounds bounds;

    hud_text_draw_color_or_flags = 0xffffu;
    hud_text_draw_column = column;
    hud_text_draw_unknown_4730 = 0;

    safe_left = (int16_t)(screen_safe_area_origin >> 16);
    bounds.top = (int16_t)(row * 0x12);
    bounds.left = (int16_t)((screen_safe_area_right >> 16) - (uint16_t)safe_left);
    bounds.bottom = (int16_t)(row * 0x12 + 0x1a);
    bounds.right = (int16_t)((screen_safe_area_bottom >> 16) - (uint16_t)safe_left);

    // blam-cc: EAX = 0, ECX = &bounds
    chimera__draw_16_bit_text(0, 0, text);
}

#if 0
Original Ghidra decompilation (0x45d670), from tools/pack.py 0x45d670:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0045d670(undefined4 param_1,undefined2 param_2)

{
  short in_CX;

  DAT_006e4734._2_2_ = param_2;
  DAT_006e4734._0_2_ = 0xffff;
  _DAT_006e4730 = 0;
  chimera__draw_16_bit_text
            (0,0,param_1,
             CONCAT22((short)((uint)DAT_007c3148 >> 0x10) + -DAT_007c3140._2_2_,in_CX * 0x12),
             CONCAT22((short)((uint)DAT_007c314c >> 0x10) + -DAT_007c3140._2_2_,in_CX * 0x12 + 0x1a)
            );
  return;
}
#endif
