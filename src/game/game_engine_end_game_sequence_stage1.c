// game_engine_end_game_sequence_stage1  (Ghidra: FUN_004670c0; named per
// out/phase4/game_functions.md: "First stage of a countdown/notification sequence: sets state 1
// with a 7-second timer and triggers an update/sound callback.")
// address 0x4670c0, size 43 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: identical to the already-committed game_engine_begin_end_game_sequence.c (0x45fd90),
//   which performs the exact same four steps inline and reuses its own extern names
//   (game_engine_state_value 0x0087aa10, game_engine_end_game_timer 0x0087aa08 == 7.0) and
//   already calls this function's sibling, game_engine_end_game_sequence_stage3 (there still
//   named FUN_004671d0), confirming these three functions (this one, _stage2 == 0x4670f0,
//   _stage3 == 0x467180) are the state machine types/game.h's game_engine_state comments
//   describe ("_game_engine_state_ending = 1 ... 7.0 s timer", "_game_engine_state_ended = 2 ...
//   set by the second countdown stage").
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_state game_engine_state_value; // 0x0087aa10
extern float game_engine_end_game_timer;          // 0x0087aa08

extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40
extern void widget_close_all(void); // 0x498650

// Enters the "ending" end-of-game state: starts the 7-second countdown, queues the end-of-game
// announcer sound and closes every open UI widget.
void game_engine_end_game_sequence_stage1(void)
{
    game_engine_state_value = _game_engine_state_ending;
    game_engine_end_game_timer = 7.0f;
    game_engine_queue_multiplayer_sound(0);
    widget_close_all();
}

#if 0
Original Ghidra decompilation (0x4670c0), from tools/pack.py 0x4670c0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004670c0(void)

{
  DAT_0087aa10 = 1;
  _DAT_0087aa08 = 0x40e00000;
  game_engine_queue_multiplayer_sound(0);
  widget_close_all();
  return;
}
#endif
