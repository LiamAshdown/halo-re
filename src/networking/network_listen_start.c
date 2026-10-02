// network_listen_start  (Ghidra: network_listen_start, already named)
// address 0x442170, size 50 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("puts the shared network channel into
// a listening state and installs the incoming-connection-request callback"); the fields
// written (+0x04 data_ready, +0x0c flags, +0x0e last_error) match network_receive_queue; only
// caller is network_channel_new (0x4dc9b0, out of this session's range, out/phase2/
// networking/02.md) which calls this with no visible argument right after constructing the
// listening channel's endpoint queue.
// register convention: network_receive_queue * in ESI (unaff_ESI).
// UNSURE: the final `& 0xffff0000` mask on gt2Listen's return keeps only the high 16
// bits and discards the low 16, which reads backward from a typical "low half of a HRESULT/
// status word" mask; reproduced exactly rather than assumed to be a decompiler artifact.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_game_socket; // 0x006f14c4

extern void gt2SetSocketData(int32_t socket, void *data); // 0x614810 gt2SetSocketData (socket +0x30)
extern int32_t gt2Listen(int32_t socket, void *callback); // foreign, GameSpy library // foreign, GameSpy library
extern void network_listen_connection_request_handler(int32_t listen_handle, int32_t reply_socket,
                                                         uint32_t remote_address, uint32_t remote_port_raw,
                                                         int32_t transport_handle, uint32_t *payload,
                                                         uint32_t payload_length); // 0x442090, this module

// blam-cc: receive-queue pointer in ESI (unaff_ESI)
uint32_t network_listen_start(network_receive_queue *queue)
{
    uint32_t result;

    queue->data_ready = 1;
    gt2SetSocketData(network_game_socket, queue); // FIXED 2026-09-28: 0x442179 also pushes ESI (the queue) as the socket data
    queue->flags = queue->flags | 2;
    result = gt2Listen(network_game_socket, (void *)network_listen_connection_request_handler);
    queue->last_error = 0;
    return result & 0xffff0000;
}

#if 0
Original Ghidra decompilation (0x442170):

uint network_listen_start(void)

{
  uint uVar1;
  int unaff_ESI;

  *(undefined1 *)(unaff_ESI + 4) = 1;
  FUN_00614810(DAT_006f14c4);
  *(byte *)(unaff_ESI + 0xc) = *(byte *)(unaff_ESI + 0xc) | 2;
  uVar1 = thunk_FUN_0061c660(DAT_006f14c4,network_listen_connection_request_handler);
  *(undefined2 *)(unaff_ESI + 0xe) = 0;
  return uVar1 & 0xffff0000;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
