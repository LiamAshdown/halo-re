// network_receive_queue_free  (Ghidra: network_receive_queue_free, already named)
// address 0x441c80, size 88 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("destroys a network receive-queue
// object created by network_receive_queue_new: closes its channel, frees the inner buffer and the wrapper,
// and cleans up handles"); called from network_channel_delete (0x4dcae0, out/phase2/
// networking/02.md) as `if (channel->endpoint != 0) network_receive_queue_free();`, which is
// how the EAX-as-queue-pointer convention is confirmed (channel->endpoint is loaded into the
// register the comparison just used, and the call follows immediately).
// register convention: receive-queue pointer in EAX only. The connection id (EBX/ECX for
// network_connection_stats_end) and key (DI) are NOT pass-throughs: they are the results of the
// two socket queries made just before the call (0x6175f0 -> id, 0x6147d0 -> key), so the C computes them here.


// VERIFIED against disassembly 0x441c80..0x441cd8 (2026-09-30): FIXED: connection id/key come from gamespy_array_length / gt2GetRemotePort (ebx/edi set at 0x441c95/0x441ca1), not from the caller
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint32_t gamespy_array_length(int32_t object); // 0x6175f0: returns the uint32 at object+0x00 // foreign, GameSpy library
extern uint16_t gt2GetRemotePort(int32_t object); // 0x6147d0: returns the uint16 at object+0x04 in AX // foreign, GameSpy library
extern void gt2SetConnectionData(int32_t socket, network_receive_queue *queue); // foreign, GameSpy library // foreign, GameSpy library
extern void network_connection_stats_end(int32_t connection_id, int16_t connection_key); // 0x440d20, this module
extern void network_receive_queue_close_socket(network_receive_queue *queue); // 0x442040, this module
extern void network_handle_registry_close_all(void); // 0x441bb0, this module

// blam-cc: EAX -> queue
void network_receive_queue_free(network_receive_queue *queue)
{
    if (queue != 0 && queue->socket != 0) {
        int32_t connection_id = (int32_t)gamespy_array_length(queue->socket);
        int16_t connection_key = (int16_t)gt2GetRemotePort(queue->socket);
        network_connection_stats_end(connection_id, connection_key);
        gt2SetConnectionData(queue->socket, 0);
    }
    network_receive_queue_close_socket(queue);
    GlobalFree(queue->incoming);
    queue->incoming = 0;
    GlobalFree(queue);
    network_handle_registry_close_all();
}

#if 0
Original Ghidra decompilation (0x441c80):

void network_receive_queue_free(void)

{
  int *in_EAX;

  if ((in_EAX != (int *)0x0) && (*in_EAX != 0)) {
    FUN_006175f0(*in_EAX);
    FUN_006147d0(*in_EAX);
    network_connection_stats_end();
    FUN_00614830(*in_EAX,0);
  }
  FUN_00442040();
  GlobalFree((HGLOBAL)in_EAX[4]);
  in_EAX[4] = 0;
  GlobalFree(in_EAX);
  FUN_00441bb0();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
