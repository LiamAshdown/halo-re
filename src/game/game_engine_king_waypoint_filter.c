// game_engine_king_waypoint_filter  (not a Ghidra function; the king game engine definition's +0x80 slot (waypoint_filter); no C existed, so that
//   stored pointer trapped as unlisted_46b7d0)
// address 0x46b7d0, size 21 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x46b7d0..0x46b7e4: true when the player is not in the hill.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t king_hill_player_in_hill[16]; // 0x006b0f40

uint8_t game_engine_king_waypoint_filter(datum_index player)
{
    return (uint8_t)(king_hill_player_in_hill[player & 0xffff] == 0);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
