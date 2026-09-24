// game_engine_is_inactive  (Ghidra: game_engine_is_inactive, already named)
// address 0x461610, size 23 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Reports whether the game engine is currently inactive
// (no game variant loaded, or not yet started)"); types/game.h current_game_engine
// (0x006f1d20), game_engine_state (0x0087aa10, _game_engine_state_not_started == 0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_engine_state game_engine_state_value;    // 0x0087aa10

uint8_t game_engine_is_inactive(void)
{
    return current_game_engine == 0 || game_engine_state_value == _game_engine_state_not_started;
}

#if 0
Original Ghidra decompilation (0x461610), from tools/pack.py 0x461610:

bool game_engine_is_inactive(void)

{
  return DAT_006f1d20 == 0 || DAT_0087aa10 == 0;
}
#endif
