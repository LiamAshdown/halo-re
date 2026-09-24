// ui_error_modal_update  (Ghidra: FUN_00494ca0, unnamed)
// address 0x494ca0, size 207 bytes
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: out/phase4/interface_functions.md "Updates/shows the shared 'error_modal' UI widget
// for one or more players when an error condition and player-count conditions are met."; cea-pdb
// hints "ui_widgets_active_errors"/"ui_widgets_force_fullscreen" via the "error_modal" string
// (naming this general area, not necessarily this exact address); reuses current_game_engine
// (0x006f1d20, per src/game/game_start_new_map.c), widget_memory_pool_valid and ui_root_widget[0]
// (both per types/interface.h's globals list), and string_replace_all_in_place.c's precedent
// identification of FUN_00625430 as _strstr.
// register convention: none (void).
// TYPES-GAP / UNSURE: DAT_0087aa10 (an int32 player-count-shaped value), DAT_006f187c+9 (a byte
// flag on an unidentified block) and ui_draw_filled_rectangle (called 1-2 times based on
// local_player_globals::local_player_count, zero visible arguments each time) are not documented
// anywhere; named/typed here only from this function's own control flow.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

// Phase-4 review against objdump 0x494ca0..0x494d6e: the two ui_draw_filled_rectangle calls are opaque black
// split-screen divider fills with literal rects, restored here; 0x006f187c is a pointer and
// 0x0087aa10 is game_engine_state_value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value; // 0x0087aa10, types/game.h
extern uint8_t *cinematic_globals; // 0x006f187c, UNSURE name; a pointer (objdump 0x4a9a57 loads it), byte +9 tested
extern uint8_t widget_memory_pool_valid; // 0x00718fc2
extern widget_instance *ui_root_widget[1]; // 0x00718f94
extern player_globals *local_player_globals; // 0x0087a478

extern char *_strstr(char *haystack, const char *needle);
extern void ui_draw_filled_rectangle(uint32_t packed_color, Rectangle2D *rect); // 0x449780, solid rectangle fill
    // blam-cc: EAX -> packed_color, ECX -> rect (objdump call sites 0x494d28, 0x4973f9, 0x498617)

// If there is no game engine active (or the player count is outside 2..3) and a certain block's
// flag byte (+9) is clear: closes/reopens the error_modal widget if the current root widget is
// one (name matched via _strstr), then calls ui_draw_filled_rectangle once or twice depending on
// local_player_globals::local_player_count (once if > 1, twice if > 2) -- presumably showing the modal
// for each additional local player.
void ui_error_modal_update(void)
{
    int16_t player_count_field;

    if ((current_game_engine == (void *)0 || (int32_t)game_engine_state_value < 2 || (int32_t)game_engine_state_value > 3) &&
        cinematic_globals[9] == 0) {
        if (widget_memory_pool_valid != 0 && ui_root_widget[0] != (widget_instance *)0) {
            _strstr(ui_root_widget[0]->name, "error_modal");
        }
        player_count_field = *(int16_t *)((char *)local_player_globals + 0xc);
        // objdump 0x494cfd..0x494d65: opaque black split-screen dividers
        if (player_count_field > 1) {
            Rectangle2D bar;

            bar.top = 0xef;
            bar.left = 0;
            bar.bottom = 0xf1;
            bar.right = 0x280;
            ui_draw_filled_rectangle(0xff000000, &bar); // horizontal bar across the middle
            if (player_count_field > 2) {
                bar.top = (player_count_field == 3) ? 0xf0 : 0;
                bar.left = 0x13f;
                bar.bottom = 0x1e0;
                bar.right = 0x141;
                ui_draw_filled_rectangle(0xff000000, &bar); // vertical bar, lower half only for three players
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x494ca0):

void FUN_00494ca0(void)

{
  short sVar1;

  if ((((DAT_006f1d20 == 0) || (DAT_0087aa10 < 2)) || (3 < DAT_0087aa10)) &&
     (*(char *)(DAT_006f187c + 9) == '\0')) {
    if ((DAT_00718fc2 != '\0') && (DAT_00718f94 != 0)) {
      FUN_00625430(*(undefined4 *)(DAT_00718f94 + 4),"error_modal");
    }
    sVar1 = *(short *)(DAT_0087a478 + 0xc);
    if ((1 < sVar1) && (FUN_00449780(), 2 < sVar1)) {
      FUN_00449780();
    }
  }
  return;
}
#endif
