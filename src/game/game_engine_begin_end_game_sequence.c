// game_engine_begin_end_game_sequence  (Ghidra: game_engine_begin_end_game_sequence, already
// named)
// address 0x45fd90, size 86 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/game.h globals: 0x00719720 network_game_mode (2 == host), 0x0087aa10
// game_engine_state, 0x0071c2d4 network_server (+0xa0f the end-of-game flag), 0x0087aa08
// game_engine_end_game_timer ("7.0 s, then 5.0 s").
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode;   // 0x00719720
extern game_engine_state game_engine_state_value; // 0x0087aa10, renamed to avoid the enum tag
extern uint8_t *network_server;       // 0x0071c2d4
extern float game_engine_end_game_timer; // 0x0087aa08

extern void game_engine_send_end_game_notification(uint32_t reason); // blam-cc: EAX reason; // 0x4671d0, not in this batch
extern void game_engine_queue_multiplayer_sound(int32_t sound_index, datum_index player, uint8_t broadcast); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast
extern void widget_close_all(void); // 0x498650

// While hosting (network_game_mode == 2) and the end-of-game sequence hasn't started yet, flags
// the network session as ending, starts the 7-second end-of-game countdown, queues the
// end-of-game announcer sound, closes every open UI widget and kicks off whatever game_engine_send_end_game_notification
// does (UNSURE, not in this batch).
void game_engine_begin_end_game_sequence(void)
{
    if (network_game_mode == 2 && game_engine_state_value == _game_engine_state_not_started) {
        *((uint8_t *)network_server + 0xa0f) = 1;
        game_engine_state_value = _game_engine_state_ending;
        game_engine_end_game_timer = 7.0f;
        game_engine_queue_multiplayer_sound(1, 0xffffffff, 0); // 0x45fdb1..0x45fdcf
        widget_close_all();
        game_engine_send_end_game_notification(1); // FIXED 2026-09-28: 0x45fddd loads EAX = 1
    }
}

#if 0
Original Ghidra decompilation (0x45fd90), from tools/pack.py 0x45fd90:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void game_engine_begin_end_game_sequence(void)

{
  if ((DAT_00719720 == 2) && (DAT_0087aa10 == 0)) {
    *(undefined1 *)(DAT_0071c2d4 + 0xa0f) = 1;
    DAT_0087aa10 = 1;
    _DAT_0087aa08 = 0x40e00000;
    game_engine_queue_multiplayer_sound(0);
    widget_close_all();
    FUN_004671d0();
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
