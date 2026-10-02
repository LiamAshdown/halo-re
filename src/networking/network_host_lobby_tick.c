// network_host_lobby_tick  (Ghidra: FUN_004daef0; renamed, no prior name)
// address 0x4daef0, size 138 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md: "Per-tick update for a hosted game: broadcasts
// presence, services the network channel, and drains incoming messages while the host's socket
// is healthy." It is case 2 of network_client_state_dispatch (0x4d8bb0), which sits between the
// two join-side states (0, 1) and the in-game states (3 = network_game_client_update, 4 =
// network_host_channel_service_tick), and it is the only caller of
// network_host_presence_broadcast_tick (0x4dadb0, the once-a-second LAN announcement), so this
// is the state a host runs in while sitting in the pre-game lobby.
// This function is a near-twin of network_host_channel_service_tick (0x4db100): same channel
// guard, same network_channel_service/network_game_process_incoming_messages pair, same
// network_join_error_code fallback. It differs only by the extra presence broadcast and by
// requiring the endpoint to be live before doing any work.
// register convention: the client pointer arrives in ESI (unaff_ESI). // blam-cc: ESI -> client
// evidence for the call shapes, from objdump -d -M intel (0x4daef0):
//   4daf1c  mov eax,esi / call 0x4dadb0        -> broadcast tick takes the client in EAX
//   4daf23  mov edi,[esi+0xadc]                -> channel in EDI for the service call
//   4daf29  push 0 / mov eax,0x3a98 / call 0x4dd110
//                                              -> network_channel_service(EDI=channel,
//                                                 EAX=0x3a98=15000 ms, stack arg 0)
//   4daf3c  push esi / call 0x4db180           -> process_incoming_messages(client) on the stack
//   4daf63  cmp WORD PTR ds:0x718fa4,0xffff    -> the error code is a 16-bit field, not 32-bit

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void network_host_presence_broadcast_tick(network_client_globals *client); // 0x4dadb0
extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern int32_t network_game_process_incoming_messages(network_client_globals *client); // 0x4db180
extern int16_t network_join_error_code; // 0x00718fa4, WORD-sized; -1 means "none pending"

// blam-cc: ESI -> client
char network_host_lobby_tick(network_client_globals *client)
{
    network_channel *channel;

    channel = client->channel;
    if ((~(uint8_t)(channel->flags >> 4) & 1) != 0 &&
        (channel->flags & (k_network_channel_client | k_network_channel_transmit_pending)) != 0 &&
        channel->endpoint != 0 &&
        (channel->endpoint->flags & 1) != 0) {
        network_host_presence_broadcast_tick(client);
        if (network_channel_service(client->channel, 15000, 0) != 0 &&
            network_game_process_incoming_messages(client) != 0) {
            return 1;
        }
    }
    // Both the guard-failed path and the service/decode-failed path converge here.
    if ((~(uint8_t)(client->channel->flags >> 4) & 1) == 0 && network_join_error_code == -1) {
        network_join_error_code = 4;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4daef0):

undefined1 FUN_004daef0(void)

{
  int *piVar1;
  char cVar2;
  bool bVar3;
  int unaff_ESI;

  piVar1 = *(int **)(unaff_ESI + 0xadc);
  if (((((~(byte)((uint)piVar1[0x2a3] >> 4) & 1) != 0) && ((*(byte *)(piVar1 + 0x2a3) & 6) != 0)) &&
      (*piVar1 != 0)) && ((*(byte *)(*piVar1 + 0xc) & 1) != 0)) {
    FUN_004dadb0();
    cVar2 = FUN_004dd110(0);
    if ((cVar2 != '\0') && (bVar3 = network_game_process_incoming_messages(unaff_ESI), bVar3)) {
      return 1;
    }
  }
  if (((~(byte)(*(uint *)(*(int *)(unaff_ESI + 0xadc) + 0xa8c) >> 4) & 1) == 0) &&
     (DAT_00718fa4 == -1)) {
    DAT_00718fa4 = 4;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
