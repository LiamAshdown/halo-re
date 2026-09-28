// hud_update_dispatch  (Ghidra: FUN_004a9990, renamed)
// address 0x4a9990, size 93 bytes
// name confidence: 0.3 (chosen)   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4a9990..0x4a99ec.)
// evidence: phase-4 summary "Per-frame HUD update dispatcher that drives the motion sensor,
// meters, and messaging sub-systems"; types/interface.h hud_messaging_globals::next_sequence
// (offset 0x465 matches exactly).
// UNSURE: all four callees (0x4b1740, 0x4b0110, 0x4af320, 0x4afee0) and the four globals
// gating the last call are out of this module's assigned range and were not analyzed; declared
// with best-guess (void) signatures, calls preserved exactly as compiled.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern hud_messaging_globals *hud_messaging; // 0x006b3a40

extern void hud_weapon_interface_state_update(void); // 0x4b1740
extern void hud_unit_meters_update(void); // 0x4b0110
extern void hud_waypoints_update(void); // 0x4af320
extern void hud_unit_sounds_update(player *p, uint8_t hud_enabled); // 0x4afee0, blam-cc: EAX player
extern data_array *player_data; // 0x0087a480

extern game_engine_definition *current_game_engine; // 0x006f1d20, UNSURE
extern int32_t game_engine_state_value; // 0x0087aa10, UNSURE
extern player_globals *local_player_globals; // 0x0087a478, established in src/game/

// Runs the weapon HUD, unit meter and waypoint per-frame updates, resets the messaging
// sequence counter, and conditionally drives a fourth update when a local-player-count-like
// gate and this build's one local player slot both look valid.
void hud_update_dispatch(void)
{
    hud_weapon_interface_state_update();
    hud_unit_meters_update();
    hud_waypoints_update();
    hud_messaging->next_sequence = 0;

    if (current_game_engine != 0 && game_engine_state_value > 1 && game_engine_state_value < 4 &&
        local_player_globals->local_players[0] != (datum_index)-1) {
        // s2 part 2 review: EAX is the player record of local player 0 (objdump 0x4a99d1..0x4a99e6)
        hud_unit_sounds_update((player *)((uint8_t *)player_data->data +
                                          (local_player_globals->local_players[0] & 0xffff) * 0x200), 0);
    }
}

#if 0
Original Ghidra decompilation (0x4a9990):

void FUN_004a9990(void)

{
  FUN_004b1740();
  FUN_004b0110();
  FUN_004af320();
  *(undefined1 *)(DAT_006b3a40 + 0x465) = 0;
  if ((((DAT_006f1d20 != 0) && (1 < DAT_0087aa10)) && (DAT_0087aa10 < 4)) &&
     (*(int *)(DAT_0087a478 + 4) != -1)) {
    FUN_004afee0(0);
  }
  return;
}
#endif
