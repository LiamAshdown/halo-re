// network_client_timer_default_or_disconnect  (Ghidra: FUN_004d9ce0; renamed, no prior name)
// address 0x4d9ce0, size 51 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Either leaves an already-flagged
// session alone or triggers a network disconnect/cleanup after defaulting the retry-limit
// field"). client+0xedc matches types/networking.h's network_client_globals::unknown_edc
// exactly; network_server+6 bit2 matches network_server_globals::flags's documented
// "bit2 stats logging".
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: none.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_host_handoff_requested;          // 0x0071c2de
extern void chat_close(void); // 0x4aa900

// blam-cc: EAX -> client
void network_client_timer_default_or_disconnect(network_client_globals *client)
{
    if (client->disconnect_reason == 0) {
        client->disconnect_reason = 8;
    }
    if (network_server != 0 && (network_server->flags >> 2 & 1) != 0) {
        return;
    }
    network_host_handoff_requested = 1;
    chat_close();
}

#if 0
Original Ghidra decompilation (0x4d9ce0):

void FUN_004d9ce0(void)

{
  int in_EAX;

  if (*(short *)(in_EAX + 0xedc) == 0) {
    *(undefined2 *)(in_EAX + 0xedc) = 8;
  }
  if ((DAT_0071c2d4 != 0) && ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) != 0)) {
    return;
  }
  DAT_0071c2de = 1;
  chat_close();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
