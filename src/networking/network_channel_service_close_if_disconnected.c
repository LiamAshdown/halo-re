// network_channel_service_close_if_disconnected  (Ghidra: FUN_004dd3f0; named per this rewrite)
// address 0x4dd3f0, size 60 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "If the channel is in an active
// connected/connecting state, flushes pending output and then performs a follow-up
// teardown/reconnect step." channel->flags (k_network_channel_client /
// k_network_channel_transmit_pending), channel->endpoint and network_receive_queue.flags bit0
// (k... connection-oriented) all match types/networking.h.
// register convention: channel in EAX (in_EAX). blam-cc: EAX -> channel

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char network_channel_transmit(network_channel *channel); // 0x4dd730, this batch
extern void network_receive_queue_close_socket(network_receive_queue *queue); // 0x442040, this module

// blam-cc: EAX -> channel
int32_t network_channel_service_close_if_disconnected(network_channel *channel)
{
    uint32_t flags;

    flags = channel->flags;
    if ((flags & (k_network_channel_client | k_network_channel_transmit_pending)) != 0 &&
        channel->endpoint != 0 && (channel->endpoint->flags & 1) != 0) {
        if (flags & (k_network_channel_client | k_network_channel_transmit_pending)) {
            network_channel_transmit(channel);
        }
        network_receive_queue_close_socket(channel->endpoint);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4dd3f0):

undefined4 FUN_004dd3f0(void)

{
  uint uVar1;
  int *in_EAX;

  uVar1 = in_EAX[0x2a3];
  if (((((uVar1 & 2) != 0) || ((uVar1 & 4) != 0)) && (*in_EAX != 0)) &&
     ((*(byte *)(*in_EAX + 0xc) & 1) != 0)) {
    if (((uVar1 & 2) != 0) || ((uVar1 & 4) != 0)) {
      network_channel_transmit(in_EAX);
    }
    FUN_00442040();
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
