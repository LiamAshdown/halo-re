// game_engine_ctf_reset_round  (not a Ghidra function; the ctf game engine definition's +0x20 slot (reset_round); no C existed, so that
//   stored pointer trapped as unlisted_468820)
// address 0x468820, size 23 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x468820..0x468836: queues multiplayer sound 0x16 (player -1, not broadcast).
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast (the C models only the sound)

void game_engine_ctf_reset_round(void)
{
    game_engine_queue_multiplayer_sound(0x16);
}
