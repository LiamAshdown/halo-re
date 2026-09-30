// network_session_host_natneg_callback  (not a Ghidra function; no C existed)
// address 0x578160, size 40 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x578160..0x578187: the query/report NAT negotiation callback (cookie): starts
//   NNBeginNegotiationWithSocket on the game socket's SOCKET (read through 0x6175f0, folded with ArrayLength) with
//   that cookie, client index 0, function_do_nothing as the progress callback and
//   network_session_host_natneg_completed.
// blam-cc: cdecl (the qr2 natneg callback)

#include "tags.h"
#include "fn_cseries.h"

extern int32_t network_game_socket; // 0x006f14c4 (GT2Socket)

extern void network_session_host_natneg_completed(int32_t result, uint32_t socket, const uint8_t *remote_address,
    void *user_data); // 0x578120
extern int32_t NNBeginNegotiationWithSocket(uint32_t socket, int32_t cookie, int32_t client_index, void *progress_callback,
    void *completed_callback, void *user_data); // 0x614f30 NNBeginNegotiationWithSocket

void network_session_host_natneg_callback(int32_t cookie)
{
    NNBeginNegotiationWithSocket(*(uint32_t *)network_game_socket, cookie, 0, (void *)function_do_nothing,
        (void *)network_session_host_natneg_completed, 0);
}
