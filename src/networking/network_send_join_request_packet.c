// network_send_join_request_packet  (Ghidra: network_send_join_request_packet, already named)
// address 0x4d9220, size 279 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues an outgoing
// join-request packet (packet type 0x1e) on the given connection"); shares the channel+0xa8c/
// +0x24/+0x1c/+0x20/+0xa80/+0x2c raw-offset idiom already used (and left unresolved) in
// network_session_info_packet_send.c and src/game/game_engine_send_team_allegiance_message.c.
// register convention: __cdecl, single stack parameter `connection` (channel at +0xadc matches
// network_client_globals, so this is the client). // blam-cc: stack -> connection
// The join request is encoded exactly like network_game_record_message_send: data_packet_group_encode_packet
// (EAX = a 0x600-byte output buffer, EBX = network_game_messages_group 0x6994f8, stack = payload
// pointer, &capacity, 0x1e, 1), then network_message_block_build wraps the encoded bytes, then the
// record is appended to the channel's outgoing bit stream. The payload argument is the address of
// an uninitialized 4-byte local (this message type carries no body). Return value is AL only.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from record; the C passed placeholders or dropped the arguments.

// VERIFIED against disassembly 0x4d9220..0x4d9337 (2026-09-30): FIXED: encoder called with EAX = separate 0x600 buffer, EBX = network_game_messages_group and a payload pointer; block_build wraps the encoded buffer; same send sequence as network_game_record_message_send
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_host_handoff_requested;          // 0x0071c2de, per types/networking.h
extern void chat_close(void); // 0x4aa900
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, data_packet_group *group, void *payload,
    int32_t *capacity, int32_t message_type, int32_t flag); // 0x4d0ae0; blam-cc: EAX -> buffer, EBX -> group, stack -> payload, capacity, message_type, flag
extern data_packet_group network_game_messages_group; // 0x006994f8
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: stack -> connection
int32_t network_send_join_request_packet(network_client_globals *connection)
{
    uint8_t encoded[0x600];
    uint32_t payload; // address passed to the encoder; the original leaves it uninitialized
    int32_t capacity;
    uint16_t *record;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char result;
    uint32_t item_flag;

    if (network_server == 0 || ((*(uint8_t *)((uint8_t *)network_server + 6) >> 2 & 1) == 0)) {
        network_host_handoff_requested = 1;
        chat_close();
    }

    capacity = 0x600;
    if ((char)data_packet_group_encode_packet(encoded, &network_game_messages_group, &payload, &capacity, 0x1e, 1) == 0) {
        return 0;
    }

    // network_message_block_build: EAX = destination block, ECX = the encoded buffer, DL = 3, stack = encoded length
    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)encoded, 3,
                                        (uint32_t)capacity);
    if (record == 0) {
        return 0;
    }

    channel = connection->channel;
    bits_to_send = (int32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    result = 1;
    if ((channel->flags & 1) == 0) {
        if ((int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1 < total_bits) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1);
            if (result == 0) {
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
    return result;
}

#if 0
Original Ghidra decompilation (0x4d9220):

uint __cdecl network_send_join_request_packet(int connection)

{
  int iVar1;
  int iVar2;
  char extraout_AL;
  uint uVar3;
  undefined3 uVar4;
  undefined3 extraout_var;
  undefined3 extraout_var_00;
  int iVar5;
  char local_60a;
  ushort *local_608;
  undefined1 local_604 [1540];

  if ((DAT_0071c2d4 == 0) || ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
    DAT_0071c2de = 1;
    chat_close();
  }
  local_608 = (ushort *)0x600;
  uVar3 = data_packet_group_encode_packet(local_604,&local_608,0x1e,1);
  if ((char)uVar3 != '\0') {
    local_608 = (ushort *)FUN_00440350(local_608);
    uVar3 = 0;
    if (local_608 != (ushort *)0x0) {
      iVar2 = *(int *)(connection + 0xadc);
      uVar4 = (undefined3)((uint)local_608 >> 8);
      iVar5 = (uint)(*local_608 >> 4) * 8;
      iVar1 = iVar5 + 1;
      local_60a = '\x01';
      if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
        if (((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1 <
            iVar1) {
          FUN_004ddb60(iVar2,1);
          uVar4 = extraout_var;
          local_60a = extraout_AL;
          if (extraout_AL == '\0') goto LAB_004d931e;
        }
        *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar2 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar5);
        *(undefined1 *)(iVar2 + 0x2c) = 0;
        uVar4 = extraout_var_00;
      }
LAB_004d931e:
      return CONCAT31(uVar4,local_60a);
    }
  }
  return uVar3 & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
