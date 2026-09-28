// render_local_view_count  (Ghidra: FUN_004c9220; still unnamed -> renamed)
// address 0x4c9220, size 64 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/main_functions.md summary ("Returns how many local split-screen player
// viewports should currently be rendered (normally 1)"). Every global here is already named by
// src/interface/ui_error_modal_update.c, which contains the IDENTICAL leading condition
// (`DAT_006f1d20 == 0 || DAT_0087aa10 < 2 || DAT_0087aa10 > 3`) over the same
// current_game_engine / game_engine_state_value globals: cinematic_globals_ptr (0x006f187c, byte +9)
// and local_player_globals (0x0087a478, player_globals*, local_player_count at +0xc, the local player
// count per out/phase4/main_types_notes.md).
// register convention: no register-passed arguments (Ghidra recognizes none).
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "game.h"
#include "networking.h"
#include "main.h"
#include "units.h"
#include "cutscene.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20, foreign
extern game_engine_state game_engine_state_value;   // 0x0087aa10, foreign (game module)
extern cinematic_globals *cinematic_globals_ptr; // 0x006f187c
extern player_globals *local_player_globals;        // 0x0087a478, foreign (game module)

// Returns the number of local split-screen viewports to render this frame: normally 1, but the
// raw local player count (local_player_globals->local_player_count) when no multiplayer game engine is
// active (or its end-of-game state is outside the 2..3 "showing results" range), no cinematic is
// suppressing it, and that count is a plausible 1 (this build never has more than one local
// player, per k_maximum_local_players in types/game.h).
int render_local_view_count(void)
{
    int16_t local_player_count_field;

    if ((current_game_engine == 0 || (int32_t)game_engine_state_value < 2 ||
         (int32_t)game_engine_state_value > 3) &&
        cinematic_globals_ptr->in_progress == 0) {
        local_player_count_field = local_player_globals->local_player_count;
        if (local_player_count_field > 0 && local_player_count_field < 2) {
            return local_player_count_field;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4c9220):

int FUN_004c9220(void)

{
  short sVar1;

  if ((((DAT_006f1d20 == 0) || (DAT_0087aa10 < 2)) || (3 < DAT_0087aa10)) &&
     (((*(char *)(DAT_006f187c + 9) == '\0' && (sVar1 = *(short *)(DAT_0087a478 + 0xc), 0 < sVar1))
      && (sVar1 < 2)))) {
    return (int)sVar1;
  }
  return 1;
}
#endif
