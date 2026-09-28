// game_engine_slayer_reset_round  (not a Ghidra function; the slayer game engine definition's +0x20 slot (reset_round); no C existed, so that
//   stored pointer trapped as unlisted_46f420)
// address 0x46f420, size 47 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46f420..0x46f44e: queues multiplayer sound 0x23 with teams, 0x15 without.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

extern void game_engine_queue_multiplayer_sound(int32_t sound_index); // 0x46be40, blam-cc: ESI sound, EDI player, stack broadcast (the C models only the sound)
extern void *current_game_engine; // 0x006f1d20
extern uint8_t game_engine_teams_enabled_flag; // 0x006f1cbc

void game_engine_slayer_reset_round(void)
{
    uint8_t teams = current_game_engine != 0 ? game_engine_teams_enabled_flag : 0;

    (void)teams;
    game_engine_queue_multiplayer_sound(teams ? 0x23 : 0x15);
}
