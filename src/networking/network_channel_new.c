// network_channel_new  (Ghidra: network_channel_new, already named)
// address 0x4dc9b0, size 295 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Allocates and initializes a new network
// transport/channel object (socket, incoming buffer, bookkeeping) sized and configured according
// to the requested flag bits; frees everything and returns NULL on failure." Every dword-indexed
// offset Ghidra shows (channel[0x2b8]=0xae0, channel+0xae1, channel[0x2a7]=0xa9c,
// channel[0x2a3]=0xa8c, channel[0]=0x000, channel[3]=0x00c, channel[0x29e..0x2a2]=0xa78..0xa88)
// matches types/networking.h's network_channel field offsets (listening, child_busy,
// listen_list, flags, endpoint, incoming, reliable_count, reliable, send_budget,
// budget_base_tick, rate_index) exactly, for both the k_network_channel_listening (0xae4-byte)
// and plain (0xa9c-byte) allocation sizes.
// UNSURE: network_channel_stream_init is called twice with no visible argument; reconstructed as
// (&channel->outgoing) then (&channel->retransmit), matching its own summary ("initializes a small
// per-direction bookkeeping record ... for a newly-created channel") and the two
// network_channel_stream sub-objects a channel owns.
// UNSURE: circular_buffer_new's own file declares it void (its pointer result is read back via
// the implicit EAX-return convention that file's header already documents); the extern below
// intentionally redeclares it returning a pointer to match how every caller must use it.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"


extern circular_buffer *circular_buffer_new(char *name, int32_t requested_size); // 0x4d0170, memory
    // module; redeclared returning a pointer, see header UNSURE note


// Allocates the 0xae4-byte listening variant when flags has k_network_channel_listening set, or
// the 0xa9c-byte plain variant when it has k_network_channel_client set; any other combination
// returns NULL. Wires up the receive queue, incoming circular buffer, and (for a listening
// channel) its own child-listen list and OS listen socket, then primes both stream directions.
// Tears everything down and returns NULL if any of these steps fails.
network_channel *network_channel_new(uint32_t flags)
{
    network_channel *channel;
    int ok;

    ok = 1;
    if ((flags & k_network_channel_listening) == 0) {
        if ((flags & k_network_channel_client) == 0) {
            return 0;
        }
        channel = (network_channel *)GlobalAlloc(0x40, 0xa9c);
        if (channel == 0) {
            return 0;
        }
    } else {
        channel = (network_channel *)GlobalAlloc(0x40, 0xae4);
        if (channel == 0) {
            return 0;
        }
        channel->listening = 1;
        channel->child_busy = 0;
        channel->listen_list = network_channel_list_new(0); // UNSURE: requested capacity elided
        if (channel->listen_list == 0) {
            network_channel_delete(channel);
            return 0;
        }
    }
    network_channel_record_timestamp(channel);
    channel->flags = flags;
    channel->endpoint = network_receive_queue_new();
    if (channel->endpoint != 0 &&
        ((flags & k_network_channel_listening) == 0 ||
         (network_listen_start(channel->endpoint) == 0 &&
          network_channel_list_add(channel->endpoint, channel->listen_list) == 0))) {
        channel->incoming = circular_buffer_new("transport-incoming", 0); // UNSURE: requested size elided
        if (channel->incoming != 0) {
            goto primed;
        }
    }
    ok = 0;
primed:
    channel->reliable_count = 0;
    channel->reliable = 0;
    channel->send_budget = 0xe0;
    channel->rate_index = 0;
    channel->budget_base_tick = GetTickCount();
    network_channel_stream_init(&channel->outgoing);
    network_channel_stream_init(&channel->retransmit);
    if (ok) {
        return channel;
    }
    network_channel_delete(channel);
    return 0;
}

#if 0
Original Ghidra decompilation (0x4dc9b0):

int * __cdecl network_channel_new(uint flags)

{
  bool bVar1;
  short sVar2;
  int *channel;
  int iVar3;
  void *pvVar4;
  DWORD DVar5;

  bVar1 = true;
  if ((flags & 1) == 0) {
    if ((flags & 2) == 0) {
      return (int *)0x0;
    }
    channel = GlobalAlloc(0x40,0xa9c);
    if (channel == (int *)0x0) {
      return (int *)0x0;
    }
  }
  else {
    channel = GlobalAlloc(0x40,0xae4);
    if (channel == (int *)0x0) {
      return (int *)0x0;
    }
    *(undefined1 *)(channel + 0x2b8) = 1;
    *(undefined1 *)((int)channel + 0xae1) = 0;
    iVar3 = FUN_00441960();
    channel[0x2a7] = iVar3;
    if (iVar3 == 0) {
      network_channel_delete(channel);
      return (int *)0x0;
    }
  }
  network_channel_record_timestamp((int)channel);
  channel[0x2a3] = flags;
  pvVar4 = network_receive_queue_new();
  *channel = (int)pvVar4;
  if ((pvVar4 != (void *)0x0) &&
     (((flags & 1) == 0 ||
      ((sVar2 = network_listen_start(), sVar2 == 0 &&
       (sVar2 = network_channel_list_add(), sVar2 == 0)))))) {
    iVar3 = circular_buffer_new("transport-incoming");
    channel[3] = iVar3;
    if (iVar3 != 0) goto LAB_004dca7f;
  }
  bVar1 = false;
LAB_004dca7f:
  channel[0x29e] = 0;
  channel[0x29f] = 0;
  channel[0x2a0] = 0xe0;
  channel[0x2a2] = 0;
  DVar5 = GetTickCount();
  channel[0x2a1] = DVar5;
  FUN_004dd980();
  FUN_004dd980();
  if (bVar1) {
    return channel;
  }
  network_channel_delete(channel);
  return (int *)0x0;
}
#endif
