// network_channel_scan_retransmit_timeouts  (Ghidra: network_channel_scan_retransmit_timeouts,
// already named)
// address 0x4dd9d0, size 368 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Scans the channel's reliable-message buffer
// pool for entries that are overdue based on an estimated delivery rate and retransmits them,
// then clears the pool's active flags." The free-space formula
// (out.stream.last_bit - out.stream.byte_cursor*8 - out.stream.bit_cursor + 1) matches
// types/networking.h's own comment on this exact function almost verbatim, confirming `out`
// (not `in`) is the stream actually used here -- see network_channel_queue_message.c's header
// for the discrepancy with that other function.
// UNSURE: bit_stream_write_bits_chunked's `value` argument is elided at both call sites here;
// this rewrite reads it as a raw 32-bit load from slot->header / slot->body, which is only
// correct for messages whose header/body each fit in 4 bytes -- the same simplification
// network_channel_queue_message.c already documents for the encode side.
// register/parameter convention: fully recovered cdecl (channel is the only parameter).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t network_rate_override; // 0x00710308
extern int32_t network_rate_table[]; // 0x00697edc

extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this batch
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values

void network_channel_scan_retransmit_timeouts(network_channel *channel)
{
    uint32_t now;
    int32_t rate;
    int32_t priority;
    int32_t i;
    network_channel_reliable_slot *slot;
    int32_t message_bits;
    int32_t budget_bits;
    int32_t free_bits;

    now = GetTickCount();
    rate = network_rate_override;
    if (network_rate_override == 0) {
        rate = network_rate_table[channel->rate_index];
    }
    for (priority = 0; priority < 10; priority++) {
        if (channel->reliable_count > 0) {
            slot = channel->reliable;
            for (i = 0; i < channel->reliable_count; i++) {
                if (slot->pending == 1 && slot->priority == priority) {
                    message_bits = slot->header_bits + slot->body_bits;
                    budget_bits = (rate / 1000) * (int32_t)(now - channel->budget_base_tick) -
                                  channel->send_budget;
                    if (budget_bits != message_bits && budget_bits - message_bits > -1) {
                        free_bits = (channel->retransmit.stream.last_bit -
                                     channel->retransmit.stream.byte_cursor * 8) -
                                    channel->retransmit.stream.bit_cursor + 1;
                        if (message_bits <= free_bits ||
                            network_channel_stream_flush(&channel->retransmit, channel, 0) != 0) {
                            bit_stream_write_bits_chunked(&channel->retransmit.stream, (const uint32_t *)slot->header, slot->header_bits); // 0x4ddaba: ECX = slot+0x18, the header bits // UNSURE: value
                            channel->retransmit.empty = 0;
                            bit_stream_write_bits_chunked(&channel->retransmit.stream, (const uint32_t *)slot->body, slot->body_bits); // 0x4ddacc: ECX = slot+0x1c // UNSURE: value
                            channel->retransmit.empty = 0;
                        }
                        channel->send_budget = channel->send_budget + message_bits;
                    }
                }
                slot = slot + 1;
            }
        }
    }
    if (channel->reliable_count > 0) {
        slot = channel->reliable;
        for (i = 0; i < channel->reliable_count; i++) {
            slot->pending = 0;
            slot = slot + 1;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dd9d0):

void __cdecl network_channel_scan_retransmit_timeouts(int channel)

{
  int iVar1;
  char cVar2;
  DWORD DVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_c;
  int local_8;

  iVar1 = channel;
  DVar3 = GetTickCount();
  iVar4 = *(int *)(channel + 0xa84);
  iVar5 = DAT_00710308;
  if (DAT_00710308 == 0) {
    iVar5 = *(int *)(&DAT_00697edc + *(int *)(channel + 0xa88) * 4);
  }
  local_8 = 0;
  do {
    local_c = 0;
    if (0 < *(int *)(iVar1 + 0xa78)) {
      channel = 0;
      do {
        iVar8 = *(int *)(iVar1 + 0xa7c) + channel;
        if (((*(char *)(*(int *)(iVar1 + 0xa7c) + channel) == '\x01') &&
            (*(int *)(iVar8 + 4) == local_8)) &&
           (iVar7 = *(int *)(iVar8 + 0x14) + *(int *)(iVar8 + 0x10),
           iVar6 = (iVar5 / 1000) * (DVar3 - iVar4) - *(int *)(iVar1 + 0xa80),
           iVar6 != iVar7 && -1 < iVar6 - iVar7)) {
          if ((iVar7 <= ((*(int *)(iVar1 + 0x558) + *(int *)(iVar1 + 0x550) * -8) -
                        *(int *)(iVar1 + 0x554)) + 1) ||
             (cVar2 = FUN_004ddb60(iVar1,0), cVar2 != '\0')) {
            bit_stream_write_bits_chunked(*(undefined4 *)(iVar8 + 0x10));
            *(undefined1 *)(iVar1 + 0x560) = 0;
            bit_stream_write_bits_chunked(*(undefined4 *)(iVar8 + 0x14));
            *(undefined1 *)(iVar1 + 0x560) = 0;
          }
          *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar7;
        }
        local_c = local_c + 1;
        channel = channel + 0x20;
      } while (local_c < *(int *)(iVar1 + 0xa78));
    }
    local_8 = local_8 + 1;
  } while (local_8 < 10);
  iVar4 = 0;
  if (0 < *(int *)(iVar1 + 0xa78)) {
    iVar5 = 0;
    do {
      *(undefined1 *)(iVar5 + *(int *)(iVar1 + 0xa7c)) = 0;
      iVar4 = iVar4 + 1;
      iVar5 = iVar5 + 0x20;
    } while (iVar4 < *(int *)(iVar1 + 0xa78));
  }
  return;
}
#endif
