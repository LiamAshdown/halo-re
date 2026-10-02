// network_receive_queue_close_socket  (Ghidra: FUN_00442040, still unnamed -> renamed)
// address 0x442040, size 76 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("tears down a single channel object:
// unregisters it from the listening channel if needed, closes its handle, and resets its
// fields"); the fields it clears (socket at +0x00, flags bit0 at +0x0c) and reads
// (data_ready at +0x04) all match types/networking.h's network_receive_queue; called from
// network_receive_queue_free (0x441c80) right after the connection-oriented cleanup block, and
// from network_listen_accept_pending_connection (0x4421b0) on the rejection path.
// register convention: receive-queue pointer in ESI (unaff_ESI).
// UNSURE: the exact meaning of comparing data_ready to 1 here (rather than testing it as a
// boolean flag) is preserved literally, not reinterpreted.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_game_socket; // 0x006f14c4
extern int32_t gt2Listen(int32_t socket, void *callback); // foreign, GameSpy library // foreign, GameSpy library
extern int32_t gt2GetConnectionState(int32_t socket); // foreign, GameSpy library
extern void gt2CloseConnectionHard(int32_t socket); // foreign, GameSpy library

// blam-cc: receive-queue pointer in ESI (unaff_ESI)
// If the queue was flagged data_ready and the main game socket is open, unregisters that
// socket from it (callback NULL). If the queue itself has a live socket whose GameSpy state is
// 0 or 1, closes it. Always clears the socket field and the "connection oriented" flag (bit0).
void network_receive_queue_close_socket(network_receive_queue *queue)
{
    int32_t state;

    if ((int8_t)queue->data_ready == 1 && network_game_socket != 0) {
        gt2Listen(network_game_socket, 0);
    }
    if (queue->socket != 0) {
        state = gt2GetConnectionState(queue->socket);
        if (state == 1 || state == 0) {
            gt2CloseConnectionHard(queue->socket);
        }
    }
    queue->socket = 0;
    queue->flags = queue->flags & 0xfe;
}

#if 0
Original Ghidra decompilation (0x442040):

void FUN_00442040(void)

{
  int iVar1;
  int *unaff_ESI;

  if (((char)unaff_ESI[1] == '\x01') && (DAT_006f14c4 != 0)) {
    thunk_FUN_0061c660(DAT_006f14c4,0);
  }
  if (*unaff_ESI != 0) {
    iVar1 = FUN_006147a0(*unaff_ESI);
    if ((iVar1 == 1) || (iVar1 == 0)) {
      FUN_00614710(*unaff_ESI);
    }
  }
  *unaff_ESI = 0;
  *(byte *)(unaff_ESI + 3) = *(byte *)(unaff_ESI + 3) & 0xfe;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
