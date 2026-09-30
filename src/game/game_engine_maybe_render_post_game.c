// game_engine_maybe_render_post_game  (Ghidra: FUN_00461a80; renamed per its summary)
// address 0x461a80, size 69 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/game_functions.md ("Invokes the postgame carnage-report rendering only
// while the game engine is in one of its end-of-game display states"); types/game.h
// game_engine_state (_game_engine_state_ended == 2, _game_engine_state_post_game == 3, both
// > 1); this batch's game_engine_post_rasterize_post_game caller relationship.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;    // 0x0087aa10


extern void widget_draw_split_screen_region(void); // 0x498330, not in this batch

void game_engine_maybe_render_post_game(void)
{
    if (current_game_engine != 0 && 1 < game_engine_state_value) {
        game_engine_post_rasterize_post_game();
        widget_draw_split_screen_region();
    }
}

#if 0
Original Ghidra decompilation (0x461a80), from tools/pack.py 0x461a80:

void FUN_00461a80(void)

{
  if ((DAT_006f1d20 != 0) && (1 < DAT_0087aa10)) {
    game_engine_post_rasterize_post_game();
    FUN_00498330();
  }
  return;
}
#endif
