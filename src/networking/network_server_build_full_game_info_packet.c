// network_server_build_full_game_info_packet  (Ghidra: FUN_004e0bd0, unnamed)
// address 0x4e0bd0, size 281 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Builds and queues a larger 'full game info'
// packet (message type 7) for a connecting machine, marking the request as answered."
// param_1[0]/+0xe match network_machine::channel/flags; the encode+flush sequence mirrors
// network_server_build_game_info_packet.c's (message group 4 there, group 7 here), confirming
// channel+0xa98/+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c as ::connected/::flags/the outgoing
// bit_stream's last_bit/byte_cursor/bit_cursor/::send_budget/::outgoing.empty.
// register convention: stack = machine (network_machine *).
// blam-cc: stack -> machine
// UNSURE: the `channel->connected != 1` gate is the literal condition (not `== 1`); when the
// channel *is* already connected, this function reports success without sending anything,
// which is the polarity actually written rather than what "connected" suggests -- preserved
// as-is, as with the similar oddities elsewhere in this batch.
// UNSURE: bit 0x10 of network_machine::flags (the "answered" mark) is not one of the
// enumerated network_machine_flags values.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from encoded_buffer; the C passed placeholders or dropped the arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char data_packet_group_encode_packet(void *header, uint32_t *size_in_out, int32_t group, int32_t message_type); // 0x4d0ae0
extern uint16_t *network_message_block_build(uint32_t size); // 0x440350, this module (UNSURE)
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// Encodes a type-7 "full game info" message into a 0x600-byte scratch record, then -- unless
// machine's channel already reports connected -- queues it onto that channel's outgoing
// stream (flushing first if there isn't room, and again after queuing). Marks the machine's
// flags bit 0x10 ("answered") on success.
char network_server_build_full_game_info_packet(network_machine *machine)
{
    uint8_t record[1540];
    uint32_t size;
    char encoded;
    uint16_t *encoded_buffer;
    int32_t bit_len;
    char ok;
    network_channel *channel;

    size = 0x600;
    encoded = data_packet_group_encode_packet(record, &size, 7, 1);
    if (encoded == 0) {
        return 0;
    }
    encoded_buffer = network_message_block_build(size);
    if (encoded_buffer == 0) {
        return 0;
    }
    bit_len = (int32_t)(*encoded_buffer >> 4) * 8;
    ok = 0;

    channel = (machine != 0) ? machine->channel : 0;
    if (machine != 0 && channel != 0 && channel->connected != 1) {
        int32_t total_bits;

        total_bits = bit_len + 1;
        if ((channel->flags & 0x01) == 0) {
            int32_t free_bits;

            free_bits = (int32_t)(channel->outgoing.stream.last_bit -
                                  channel->outgoing.stream.byte_cursor * 8) -
                        (int32_t)channel->outgoing.stream.bit_cursor + 1;
            if (free_bits < total_bits) {
                char flushed;

                flushed = network_channel_stream_flush(&channel->outgoing, channel, 1);
                ok = 0;
                if (flushed == 0) {
                    goto done;
                }
            }
            channel->send_budget = channel->send_budget + total_bits;
            { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
            channel->outgoing.empty = 0;
            bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(encoded_buffer), bit_len);
            channel->outgoing.empty = 0;
            ok = network_channel_stream_flush(&channel->outgoing, channel, 1);
        } else {
            ok = 1;
        }
    }
done:
    if (ok == 0) {
        return 0;
    }
    machine->flags |= 0x10; // UNSURE: not an enumerated network_machine_flags bit
    return ok;
}

#if 0
Original Ghidra decompilation (0x4e0bd0):

char FUN_004e0bd0(int *param_1)

{
  int iVar1;
  int iVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  ushort *local_608;
  undefined1 local_604 [1540];

  local_608 = (ushort *)0x600;
  cVar3 = data_packet_group_encode_packet(local_604,&local_608,7,1);
  if (cVar3 == '\0') {
    return '\0';
  }
  local_608 = (ushort *)FUN_00440350(local_608);
  if (local_608 == (ushort *)0x0) {
    return '\0';
  }
  iVar5 = (uint)(*local_608 >> 4) * 8;
  cVar3 = '\0';
  if ((((param_1 != (int *)0x0) && (iVar2 = *param_1, iVar2 != 0)) &&
      (*(char *)(iVar2 + 0xa98) != '\x01')) && (iVar2 != 0)) {
    iVar1 = iVar5 + 1;
    if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
      if (((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1 <
          iVar1) {
        cVar4 = FUN_004ddb60(iVar2,1);
        cVar3 = '\0';
        if (cVar4 == '\0') goto LAB_004e0ccb;
      }
      *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar1;
      FUN_004cf8f0(1);
      *(undefined1 *)(iVar2 + 0x2c) = 0;
      FUN_004cf8f0(iVar5);
      *(undefined1 *)(iVar2 + 0x2c) = 0;
      cVar3 = FUN_004ddb60(iVar2,1);
    }
    else {
      cVar3 = '\x01';
    }
  }
LAB_004e0ccb:
  if (cVar3 == '\0') {
    return '\0';
  }
  *(byte *)((int)param_1 + 0xe) = *(byte *)((int)param_1 + 0xe) | 0x10;
  return cVar3;
}
#endif
