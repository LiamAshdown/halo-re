// game_engine_get_current_tick  (Ghidra: game_engine_get_current_tick, already named)
// address 0x470cd0, size 9 bytes, cc=__cdecl
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: types/game.h game_time_globals::game_time (+0x0c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern game_time_globals *game_time; // 0x006f1d6c

// Returns the current simulation tick count.
int32_t game_engine_get_current_tick(void)
{
    return game_time->game_time;
}

#if 0
Original Ghidra decompilation (0x470cd0), from tools/pack.py 0x470cd0:

int __cdecl game_engine_get_current_tick(void)

{
  return *(int *)(DAT_006f1d6c + 0xc);
}
#endif
