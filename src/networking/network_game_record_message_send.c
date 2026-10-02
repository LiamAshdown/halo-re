// network_game_record_message_send  (Ghidra: FUN_004da130; renamed, no prior name)
// address 0x4da130, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Encodes a large (up to 0x600-byte)
// outgoing network-game message (type id 0x12) from an 8-dword source record and queues it for
// transmission"). This function has zero callers anywhere in the binary (out/functions.json
// callers=0); rewritten anyway per the task instructions, since it is not listed as
// misattributed/library code in out/phase4/networking_types_notes.md.
// register convention: client in the sole cdecl stack parameter; the 8-dword source record
// arrives in EDX (in_EDX). // blam-cc: EDX -> source, stack -> client
// The 8-dword source record is copied to a local (byte 30 of it is forced from 0xff to 0), encoded
// into a separate 0x600-byte buffer by data_packet_group_encode_packet (EAX = that buffer, EBX =
// network_game_messages_group 0x6994f8, stack = record, &size, 0x12, 1), and the encoded bytes
// are wrapped by network_message_block_build. Return value is only the low byte (0/1).
// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from record; the C passed placeholders or dropped the arguments.

// VERIFIED against disassembly 0x4da130..0x4da250 (2026-09-30): FIXED: encode call takes EAX = separate 0x600 output buffer and EBX = network_game_messages_group (was missing); block_build now wraps that encoded buffer, not the 32-byte record copy; stream-space/flush/write sequence matches
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, data_packet_group *group, void *payload,
    int32_t *capacity, int32_t message_type, int32_t flag); // 0x4d0ae0; blam-cc: EAX -> buffer, EBX -> group, stack -> payload, capacity, message_type, flag
extern data_packet_group network_game_messages_group; // 0x006994f8
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: EDX -> source, stack -> client
int32_t network_game_record_message_send(network_client_globals *client, const uint32_t *source)
{
    uint8_t record_copy[32];
    uint8_t encoded[0x600];
    uint32_t *dst;
    int32_t i;
    int32_t capacity;
    uint16_t *record;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    uint32_t item_flag;

    dst = (uint32_t *)record_copy;
    for (i = 8; i != 0; i = i - 1) {
        *dst = *source;
        source = source + 1;
        dst = dst + 1;
    }
    if (record_copy[30] == 0xff) {
        record_copy[30] = 0;
    }

    capacity = 0x600;
    if ((char)data_packet_group_encode_packet(encoded, &network_game_messages_group, record_copy, &capacity, 0x12, 1) == 0) {
        return 0;
    }

    // network_message_block_build: EAX = destination block, ECX = the encoded buffer, DL = 3, stack = encoded length
    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)encoded, 3,
                                        (uint32_t)capacity);
    if (record == 0) {
        return 0;
    }

    channel = client->channel;
    bits_to_send = (int32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    if ((channel->flags & 1) == 0) {
        if ((int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1 < total_bits) {
            if (network_channel_stream_flush(&channel->outgoing, channel, 1) == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + total_bits;
        item_flag = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)record, bits_to_send);
        channel->outgoing.empty = 0;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4da130):

uint FUN_004da130(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined3 uVar3;
  undefined3 extraout_var;
  int iVar4;
  undefined4 *in_EDX;
  undefined4 *puVar5;
  int iVar6;
  ushort *local_624;
  undefined4 local_620 [7];
  char local_602;

  puVar5 = local_620;
  for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar5 = *in_EDX;
    in_EDX = in_EDX + 1;
    puVar5 = puVar5 + 1;
  }
  if (local_602 == -1) {
    local_602 = '\0';
  }
  local_624 = (ushort *)0x600;
  uVar2 = data_packet_group_encode_packet(local_620,&local_624,0x12,1);
  if ((char)uVar2 != '\0') {
    local_624 = (ushort *)FUN_00440350(local_624);
    uVar2 = 0;
    if (local_624 != (ushort *)0x0) {
      iVar1 = *(int *)(param_1 + 0xadc);
      uVar3 = (undefined3)((uint)param_1 >> 8);
      iVar6 = (uint)(*local_624 >> 4) * 8;
      iVar4 = iVar6 + 1;
      if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
        if (((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
            iVar4) {
          uVar2 = FUN_004ddb60(iVar1,1);
          if ((char)uVar2 == '\0') {
            return uVar2 & 0xffffff00;
          }
        }
        *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar4;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar6);
        *(undefined1 *)(iVar1 + 0x2c) = 0;
        uVar3 = extraout_var;
      }
      return CONCAT31(uVar3,1);
    }
  }
  return uVar2 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
