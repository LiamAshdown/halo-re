// game_engine_race_waypoint_filter  (not a Ghidra function; the race game engine definition's +0x80 slot (waypoint_filter); no C existed, so that
//   stored pointer trapped as unlisted_46e9d0)
// address 0x46e9d0, size 37 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46e9d0..0x46e9f4: when the team's bit is set in the flag mask (0x006b1290),
//   whether its flag is eligible for capture (0x46df30 with the team as both team and flag); else false.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"
#include <wchar.h>

extern uint32_t ctf_globals_live; // 0x006b1290 (ctf_globals, first dword: the team flag mask)


uint8_t game_engine_race_waypoint_filter(datum_index player, int32_t team)
{
    (void)player;
    if ((ctf_globals_live & (1u << (team & 0x1f))) == 0) {
        return 0;
    }
    return game_engine_ctf_is_flag_eligible_for_capture((uint32_t)team, team);
}
