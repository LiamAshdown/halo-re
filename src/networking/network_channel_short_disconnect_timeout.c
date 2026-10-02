// network_channel_short_disconnect_timeout  (Ghidra: FUN_004ddd20; named per this rewrite)
// address 0x4ddd20, size 27 bytes
// name confidence: 0.4   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md: "Returns true when the host globals are present
// and the DAT_0071c2dc flag is clear; used by the connection state machine to choose a shorter
// disconnect timeout." network_server and network_disconnect_timeout_flag (0x0071c2dc, per
// types/networking.h's own name for this address) match exactly.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, UNSURE name; types/networking.h
    // describes this address as "shortens the disconnect timeout when clear"

int32_t network_channel_short_disconnect_timeout(void)
{
    if (network_server != 0 && network_disconnect_timeout_flag == 0) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ddd20):

undefined4 FUN_004ddd20(void)

{
  if ((DAT_0071c2d4 != 0) && (DAT_0071c2dc == '\0')) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
