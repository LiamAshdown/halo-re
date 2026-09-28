// network_send_join_request_packet  (Ghidra: network_send_join_request_packet, already named)
// address 0x4d9220, size 279 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues an outgoing
// join-request packet (packet type 0x1e) on the given connection"); shares the channel+0xa8c/
// +0x24/+0x1c/+0x20/+0xa80/+0x2c raw-offset idiom already used (and left unresolved) in
// network_session_info_packet_send.c and src/game/game_engine_send_team_allegiance_message.c.
// register convention: __cdecl, single stack parameter `connection` (channel at +0xadc matches
// network_client_globals, so this is the client). // blam-cc: stack -> connection
// UNSURE: `data_packet_group_encode_packet` is called here with only 4 visible arguments
// (buffer, &capacity, 0x1e, 1), which does not line up positionally with the fuller 10-parameter
// reconstruction in src/memory/data_packet_group_encode_packet.c (built from a *different* call
// site with its own, differently-elided arguments). That file's own header already notes the
// real call-site values "cannot be recovered from the available decompilation"; the same applies
// here, so this file declares its own minimal, call-site-accurate local prototype rather than
// forcing an inconsistent fit to the other reconstruction.
// UNSURE: `network_message_block_build` (0x440350, transport code below this task's range) is called with the
// capacity value and its result reused as the encoded record pointer; declared generically.
// UNSURE: the real return value is `CONCAT31(garbage, result_byte)` in Ghidra throughout (three
// different "extraout"/register-carryover sources feed the garbage byte); simplified to a plain
// 0/1 return, matching the "callers only read the low byte" idiom already documented in
// src/memory/bit_stream_write_bit.c and used elsewhere in this module.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from record; the C passed placeholders or dropped the arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_host_handoff_requested;          // 0x0071c2de, per types/networking.h
extern void chat_close(void); // 0x4aa900
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int16_t **capacity,
    int32_t packet_type, int32_t version); // 0x4d0ae0; UNSURE, this call site's own 4-arg shape
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: stack -> connection
int32_t network_send_join_request_packet(network_client_globals *connection)
{
    uint8_t buffer[1540];
    int16_t *capacity;
    uint16_t *record;
    uint8_t *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char result;
    int32_t encode_ok;

    if (network_server == 0 || ((*(uint8_t *)((uint8_t *)network_server + 6) >> 2 & 1) == 0)) {
        network_host_handoff_requested = 1;
        chat_close();
    }

    capacity = (int16_t *)0x600;
    encode_ok = data_packet_group_encode_packet(buffer, &capacity, 0x1e, 1);
    if ((char)encode_ok == 0) {
        return 0;
    }

    // FIXED in the review pass: network_message_block_build takes the destination block in
    // EAX, the source buffer in ECX, a 2-bit flag value in DL and the byte length on the
    // stack. Every inlined copy of this idiom in the module is the same four operands --
    // `push <encoded length> / mov eax,0x6b7f98 / lea ecx,[<buffer>] / mov dl,3` -- so the
    // first draft's single-argument call was passing the length where the destination goes.
    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)buffer, 3,
                                        (uint32_t)(int32_t)capacity); // 0x4d927a
    if (record == 0) {
        return 0;
    }

    channel = (uint8_t *)connection->channel;
    bits_to_send = (uint32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    result = 1;
    if ((*(uint8_t *)&((network_channel *)channel)->flags & 1) == 0) {
        if (((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                 *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 < total_bits) {
            result = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1);
            if (result == 0) {
                return 0;
            }
        }
        {

            ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + total_bits;
            { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
            ((network_channel *)channel)->outgoing.empty = 0;
            bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(record), bits_to_send);
            ((network_channel *)channel)->outgoing.empty = 0;
        }
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
