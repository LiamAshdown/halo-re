// network_channel_new_child  (Ghidra: FUN_004dd430; named per this rewrite)
// address 0x4dd430, size 172 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Allocates and initializes a new child network
// channel object for a freshly-accepted incoming connection, mirroring the smaller-variant setup
// done by the general channel constructor." Same field offsets as network_channel_new.c's plain
// (0xa9c-byte) allocation, with flags hard-coded to k_network_channel_transmit_pending (4)
// instead of being a parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *GlobalAlloc(uint32_t flags, uint32_t bytes);
extern uint32_t GetTickCount(void);

extern circular_buffer *circular_buffer_new(char *name, int32_t requested_size); // 0x4d0170, memory
    // module; redeclared returning a pointer, see network_channel_new.c's UNSURE note
extern void network_channel_delete(network_channel *channel); // 0x4dcae0, this batch
extern void network_channel_record_timestamp(network_channel *channel); // 0x4dd930, this batch
extern void network_channel_stream_init(network_channel_stream *stream); // 0x4dd980, this batch

network_channel *network_channel_new_child(network_receive_queue *endpoint)
{
    network_channel *channel;

    channel = (network_channel *)GlobalAlloc(0x40, 0xa9c);
    if (channel != 0) {
        channel->endpoint = endpoint;
        channel->flags = k_network_channel_transmit_pending;
        channel->incoming = circular_buffer_new("transport-incoming", 0); // UNSURE: requested size elided
        network_channel_record_timestamp(channel);
        channel->reliable_count = 0;
        channel->reliable = 0;
        channel->send_budget = 0xe0;
        channel->rate_index = 0;
        channel->budget_base_tick = GetTickCount();
        network_channel_stream_init(&channel->outgoing);
        network_channel_stream_init(&channel->retransmit);
        if (channel->incoming == 0) {
            network_channel_delete(channel);
            return 0;
        }
    }
    return channel;
}

#if 0
Original Ghidra decompilation (0x4dd430):

int * FUN_004dd430(int param_1)

{
  int *channel;
  int iVar1;
  DWORD DVar2;

  channel = GlobalAlloc(0x40,0xa9c);
  if (channel != (int *)0x0) {
    *channel = param_1;
    channel[0x2a3] = 4;
    iVar1 = circular_buffer_new("transport-incoming");
    channel[3] = iVar1;
    network_channel_record_timestamp((int)channel);
    channel[0x29e] = 0;
    channel[0x29f] = 0;
    channel[0x2a0] = 0xe0;
    channel[0x2a2] = 0;
    DVar2 = GetTickCount();
    channel[0x2a1] = DVar2;
    FUN_004dd980();
    FUN_004dd980();
    if (iVar1 == 0) {
      network_channel_delete(channel);
      return (int *)0x0;
    }
  }
  return channel;
}
#endif
