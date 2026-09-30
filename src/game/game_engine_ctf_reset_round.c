// game_engine_ctf_reset_round  (not a Ghidra function; the ctf game engine definition's +0x20 slot (reset_round); no C existed, so that
//   stored pointer trapped as unlisted_468820)
// address 0x468820, size 23 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x468820..0x468836: queues multiplayer sound 0x16 (player -1, not broadcast).
// blam-cc: cdecl (called through the engine definition)
// FIXED 2026-09-28 (mp sound): 0x46be40 takes ESI sound, EDI player and a stack broadcast byte; the
//   call(s) here now pass all three as the binary loads them (they passed one value before).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>


void game_engine_ctf_reset_round(void)
{
    game_engine_queue_multiplayer_sound(0x16, 0xffffffff, 0);
}
