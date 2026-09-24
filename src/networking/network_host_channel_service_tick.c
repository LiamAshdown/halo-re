// network_host_channel_service_tick  (Ghidra: FUN_004db100; renamed, no prior name)
// address 0x4db100, size 120 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Per-tick network update used while
// acting as host/server: services the channel, drains incoming messages, updates per-machine
// state, and sends a periodic ping"). Shares the channel-flags bit4 ("not dead") test with
// network_host_update_tick.c.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none beyond the shared cluster-wide notes on network_channel_service's argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_channel_service(network_channel *channel, int32_t timeout_ms, network_channel **out_new_child); // 0x4dd110
extern int32_t network_game_process_incoming_messages(network_client_globals *client); // 0x4db180
extern char network_client_identity_tick(network_client_globals *client); // 0x4db310
extern void network_connection_send_keepalive(network_client_globals *client); // 0x4d9400
extern int16_t network_join_error_code; // 0x00718fa4, WORD-sized (0x4db15f: cmp WORD PTR)

// blam-cc: EAX -> client
char network_host_channel_service_tick(network_client_globals *client)
{
    char ok;

    // 0x4db105: edi = client->channel; 0x4db10b: push 0; 0x4db10d: eax = 0x3a98 (15000 ms).
    ok = network_channel_service(client->channel, 15000, 0);
    if (ok != 0) {
        if (network_game_process_incoming_messages(client)) {
            ok = network_client_identity_tick(client); // 0x4db130: push esi
            if (ok != 0) {
                network_connection_send_keepalive(client);
                return ok;
            }
        }
    }
    if ((~(uint8_t)(client->channel->flags >> 4) & 1) != 0) {
        return 0;
    }
    if (network_join_error_code == -1) {
        network_join_error_code = 4;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4db100):

char FUN_004db100(void)

{
  char cVar1;
  bool bVar2;
  int in_EAX;

  cVar1 = FUN_004dd110(0);
  if (cVar1 != '\0') {
    bVar2 = network_game_process_incoming_messages(in_EAX);
    if ((bVar2) && (cVar1 = FUN_004db310(), cVar1 != '\0')) {
      FUN_004d9400();
      return cVar1;
    }
  }
  if ((~(byte)(*(uint *)(*(int *)(in_EAX + 0xadc) + 0xa8c) >> 4) & 1) != 0) {
    return '\0';
  }
  if (DAT_00718fa4 == -1) {
    DAT_00718fa4 = 4;
  }
  return '\0';
}
#endif
