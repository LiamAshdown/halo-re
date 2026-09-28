// network_channel_delete  (Ghidra: network_channel_delete, already named)
// address 0x4dcae0, size 329 bytes
// name confidence: 0.55   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Tears down and frees a network channel object,
// recursively closing/freeing any child channels, its buffers, and associated OS handles before
// freeing the object itself." Every dword-indexed offset (channel[3]=0x00c, channel+0x2a3(byte)=
// 0xa8c, channel+0x2a8=0xaa0, channel[0x2a7]=0xa9c, channel+0xb(byte)=0x02c, channel[4..10]=
// 0x010..0x028, channel+0x158(byte)=0x560, channel[0x151..0x157]=0x544..0x55c,
// channel[0x29e]/[0x29f]/[0x2a0]=0xa78/0xa7c/0xa80) matches types/networking.h's network_channel
// field offsets exactly (incoming, flags, children, listen_list, in.empty, in.stream.*,
// out.empty, out.stream.*, reliable_count, reliable, send_budget), for both the listening and
// plain variants.
// UNSURE: network_receive_queue_free's own file documents connection_id/connection_key as an
// unresolved pass-through this far up the call chain too; supplied here as 0/0 placeholders.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *__stdcall GlobalFree(void *memory);

extern void network_receive_queue_free(network_receive_queue *queue, int32_t connection_id,
    int16_t connection_key); // 0x441c80, this module
extern int32_t network_channel_list_remove(network_receive_queue *entry, network_channel_list *list); // 0x441b00, this module

void network_channel_delete(network_channel *channel)
{
    int i;
    network_channel_reliable_slot *slot;

    if (channel == 0) {
        return;
    }
    if (channel->endpoint != 0) {
        network_receive_queue_free(channel->endpoint, 0, 0); // UNSURE: see header
    }
    if (channel->incoming != 0) {
        GlobalFree(channel->incoming);
    }
    if (channel->flags & k_network_channel_listening) {
        for (i = 0; i < 16; i++) {
            if (channel->children[i] != 0) {
                if (channel->listen_list != 0) {
                    network_channel_list_remove(channel->children[i]->endpoint, channel->listen_list);
                }
                network_channel_delete(channel->children[i]);
            }
        }
        if (channel->listen_list != 0) {
            GlobalFree(channel->listen_list->entries);
            GlobalFree(channel->listen_list);
        }
    }
    channel->outgoing.stream.unknown_00 = (uint32_t)-1;
    channel->outgoing.stream.data = 0;
    channel->outgoing.stream.first_bit = 0;
    channel->outgoing.stream.byte_cursor = 0;
    channel->outgoing.stream.bit_cursor = 0;
    channel->outgoing.stream.last_bit = 0;
    channel->outgoing.capacity_bits = 0;
    channel->outgoing.empty = 1;
    channel->retransmit.stream.unknown_00 = (uint32_t)-1;
    channel->retransmit.stream.data = 0;
    channel->retransmit.stream.first_bit = 0;
    channel->retransmit.stream.byte_cursor = 0;
    channel->retransmit.stream.bit_cursor = 0;
    channel->retransmit.stream.last_bit = 0;
    channel->retransmit.capacity_bits = 0;
    channel->retransmit.empty = 1;
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        for (i = 0; i < channel->reliable_count; i++) {
            GlobalFree(slot[i].header);
            GlobalFree(slot[i].body);
        }
        GlobalFree(channel->reliable);
        channel->reliable = 0;
        channel->reliable_count = 0;
    }
    channel->send_budget = 0;
    GlobalFree(channel);
}

#if 0
Original Ghidra decompilation (0x4dcae0):

void __cdecl network_channel_delete(int *channel)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  code *pcVar4;

  if (channel != (int *)0x0) {
    if (*channel != 0) {
      network_receive_queue_free();
    }
    pcVar4 = GlobalFree_exref;
    if ((HGLOBAL)channel[3] != (HGLOBAL)0x0) {
      GlobalFree((HGLOBAL)channel[3]);
    }
    if ((*(byte *)(channel + 0x2a3) & 1) != 0) {
      piVar1 = channel + 0x2a8;
      if (piVar1 != (int *)0x0) {
        iVar2 = 0x10;
        do {
          if ((undefined4 *)*piVar1 != (undefined4 *)0x0) {
            if (channel[0x2a7] != 0) {
              network_channel_list_remove(*(undefined4 *)*piVar1);
            }
            network_channel_delete((int *)*piVar1);
          }
          piVar1 = piVar1 + 1;
          iVar2 = iVar2 + -1;
          pcVar4 = GlobalFree_exref;
        } while (iVar2 != 0);
      }
      iVar2 = channel[0x2a7];
      if (iVar2 != 0) {
        (*pcVar4)(*(undefined4 *)(iVar2 + 0x104));
        (*pcVar4)(iVar2);
      }
    }
    *(undefined1 *)(channel + 0xb) = 1;
    channel[5] = 0;
    channel[6] = 0;
    channel[7] = 0;
    channel[8] = 0;
    channel[9] = 0;
    channel[10] = 0;
    channel[4] = -1;
    *(undefined1 *)(channel + 0x158) = 1;
    channel[0x151] = -1;
    channel[0x152] = 0;
    channel[0x153] = 0;
    channel[0x154] = 0;
    channel[0x155] = 0;
    channel[0x156] = 0;
    channel[0x157] = 0;
    if (0 < channel[0x29e]) {
      iVar2 = 0;
      if (0 < channel[0x29e]) {
        iVar3 = 0;
        do {
          GlobalFree(*(HGLOBAL *)(channel[0x29f] + 0x18 + iVar3));
          GlobalFree(*(HGLOBAL *)(channel[0x29f] + 0x1c + iVar3));
          iVar2 = iVar2 + 1;
          iVar3 = iVar3 + 0x20;
          pcVar4 = GlobalFree_exref;
        } while (iVar2 < channel[0x29e]);
      }
      (*pcVar4)(channel[0x29f]);
      channel[0x29f] = 0;
      channel[0x29e] = 0;
    }
    channel[0x2a0] = 0;
    (*pcVar4)(channel);
  }
  return;
}
#endif
