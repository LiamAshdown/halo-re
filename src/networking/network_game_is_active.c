// network_game_is_active  (Ghidra: network_game_is_active, already named)
// address 0x4ddca0, size 27 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/networking_functions.md: "Returns whether a network game is currently
// active by checking that either the network-game globals or the host globals pointer is
// non-null." network_client (0x0071c2d8) and network_server (0x0071c2d4) match
// types/networking.h exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_client_globals *network_client; // 0x0071c2d8
extern network_server_globals *network_server; // 0x0071c2d4

int32_t network_game_is_active(void)
{
    if (network_client == 0 && network_server == 0) {
        return 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ddca0):

int __cdecl network_game_is_active(void)

{
  if ((DAT_0071c2d8 == 0) && (DAT_0071c2d4 == 0)) {
    return 0;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
