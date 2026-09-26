// sv_map_reset  (Ghidra: sv_map_reset, already named)
// address 0x4e2aa0, size 125 bytes
// name confidence: 0.8   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; the literal "Map reset.", "Cannot restart the
// map when the game is over." and "sv_map_reset is a server-only function!" strings;
// game_engine_state_value (0x0087aa10, types/game.h) already named by src/game's own rewrites.
// register convention: __cdecl, no arguments.
// UNSURE: FUN_00466cb0 (game_engine_player_profile_cache_sync_all) is called here with only one
// of its own file's two established arguments; declared with a local single-argument prototype
// matching this call site instead, per the established precedent for this kind of mismatch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int16_t network_game_mode; // 0x00719720, 2 == host
extern game_engine_state game_engine_state_value; // 0x0087aa10

extern void widget_close_all(void); // 0x498650, other module
extern void game_engine_reset_round_objects(void); // 0x468260, game module
extern void game_engine_send_round_reset_message(void); // 0x4682c0, game module
extern void game_engine_player_profile_cache_sync_all(int32_t commit); // 0x466cb0, game
    // module, called here with only its first argument (UNSURE)
extern void *console_color_006851fc; // 0x006851fc, a ColorARGB * the original loads into EAX
extern void *console_color_00685218; // 0x00685218, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: restarts the current map, refusing when not hosting or when the round is
// already ending/over.
void sv_map_reset(void)
{
    if (network_game_mode != 2) {
        chimera__console_out((ColorARGB *)0, "sv_map_reset is a server-only function!");
        return;
    }
    widget_close_all();
    if (network_game_mode == 2) {
        if (game_engine_state_value == _game_engine_state_not_started) {
            game_engine_reset_round_objects();
            game_engine_send_round_reset_message();
            game_engine_player_profile_cache_sync_all(-1);
            chimera__console_out((ColorARGB *)console_color_00685218, "Map reset.");
            return;
        }
        chimera__console_out((ColorARGB *)0, "Cannot restart the map when the game is over.");
    }
    chimera__console_out((ColorARGB *)console_color_006851fc, "Map reset.");
}

#if 0
Original Ghidra decompilation (0x4e2aa0), from tools/pack.py 0x4e2aa0:

void sv_map_reset(void)

{
  if (DAT_00719720 != 2) {
    chimera__console_out("sv_map_reset is a server-only function!");
    return;
  }
  widget_close_all();
  if (DAT_00719720 == 2) {
    if (DAT_0087aa10 == 0) {
      FUN_00468260();
      FUN_004682c0();
      FUN_00466cb0(0xffffffff);
      chimera__console_out("Map reset.");
      return;
    }
    chimera__console_out("Cannot restart the map when the game is over.");
  }
  chimera__console_out("Map reset.");
  return;
}
#endif
