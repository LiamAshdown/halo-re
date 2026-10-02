// network_listen_reject_pending_connection  (Ghidra: FUN_00442250, still unnamed -> renamed)
// address 0x442250, size 50 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("cancels (rejects) the most recently
// queued pending incoming connection request without accepting it"); mirrors
// network_listen_accept_pending_connection.c's `network_pending_connections[count - 1]`
// derivation from the same `(&DAT_0087bc0c)[count*5]` dword-array indexing.
// register convention: __cdecl; the reject code Ghidra shows only as `&stack0x00000004` (its
// address taken, never its value read directly) is this function's own single stack
// parameter, exactly like network_listen_connection_request_handler.c's `local_1c` reject
// codes -- declared here as an ordinary by-value parameter whose address is what gets sent.
// UNSURE: the final `DAT_006f16d0 & 0xffff0000` return is always 0 for any realistic pending
// count (0..30); kept literally rather than assumed to be dead/miscompiled code.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t network_pending_connection_count; // 0x006f16d0
extern network_pending_connection network_pending_connections[k_network_pending_connection_count]; // 0x0087bc20

extern void gt2Reject(int32_t socket, void *buffer, int32_t length); // foreign, GameSpy library; send reply

// blam-cc: reject_code is an ordinary stack parameter, sent by address
uint32_t network_listen_reject_pending_connection(int32_t reject_code)
{
    if (0 < network_pending_connection_count) {
        gt2Reject(network_pending_connections[network_pending_connection_count - 1].reply_socket,
                            &reject_code, 4);
        network_pending_connection_count = network_pending_connection_count - 1;
    }
    return (uint32_t)network_pending_connection_count & 0xffff0000;
}

#if 0
Original Ghidra decompilation (0x442250):

uint FUN_00442250(void)

{
  if (0 < (int)DAT_006f16d0) {
    thunk_FUN_0061cee0((&DAT_0087bc0c)[DAT_006f16d0 * 5],&stack0x00000004,4);
    DAT_006f16d0 = DAT_006f16d0 - 1;
  }
  return DAT_006f16d0 & 0xffff0000;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
