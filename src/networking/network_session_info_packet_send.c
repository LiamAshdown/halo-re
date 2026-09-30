// network_session_info_packet_send  (Ghidra: FUN_004d9050; renamed, no prior name)
// address 0x4d9050, size 292 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues a session-info
// packet for the connection, when its connection-mode field indicates that one is needed").
// register convention: the 8-dword source record arrives in EAX (in_EAX); client is the sole
// cdecl stack parameter. // blam-cc: EAX -> source, stack -> client
// Dispatch on client->state (jump table at 0x4d9174): 0 and 1 return 0, 2 builds message type
// 0x10, 3 builds 0x1d, 4 builds 0x23, and any other state returns 1 without sending. The 8-dword
// source record is copied to a local whose address is the payload (EDX) of
// network_prepare_challenge_packet (EAX = message type); the resulting record is appended to the
// channel's outgoing bit stream exactly as in network_game_record_message_send. Return is AL only.

// VERIFIED against disassembly 0x4d9050..0x4d9174 (2026-09-30): FIXED: state 3 builds message type 0x1d (was 0x23); states 0/1 -> 0, >4 -> 1 per the jump table at 0x4d9174; send sequence compared
#include "game.h"
#include "networking.h"

extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: EAX -> source, stack -> client
char network_session_info_packet_send(const uint32_t *source, network_client_globals *client)
{
    char result;
    uint32_t local_buffer[8];
    uint16_t *challenge;
    network_channel *channel;
    int32_t bits_to_send;
    int32_t message_type;
    int32_t i;
    uint32_t item_flag;

    switch (client->state) {
    case 0:
    case 1:
        return 0;
    case 2:
        message_type = 0x10;
        break;
    case 3:
        message_type = 0x1d;
        break;
    case 4:
        message_type = 0x23;
        break;
    default:
        return 1;
    }

    for (i = 0; i < 8; i = i + 1) {
        local_buffer[i] = source[i];
    }
    challenge = network_prepare_challenge_packet(message_type, local_buffer);
    if (challenge == 0) {
        return 0;
    }

    channel = client->channel;
    bits_to_send = (int32_t)(*challenge >> 4) * 8;
    result = 1;
    if ((channel->flags & 1) == 0) {
        if (bits_to_send + 1 >
            (int32_t)(channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8 -
                      channel->outgoing.stream.bit_cursor) + 1) {
            result = network_channel_stream_flush(&channel->outgoing, channel, 1);
            if (result == 0) {
                return 0;
            }
        }
        channel->send_budget = channel->send_budget + bits_to_send + 1;
        item_flag = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
        channel->outgoing.empty = 0;
        bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)challenge, bits_to_send);
        channel->outgoing.empty = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d9050):

char FUN_004d9050(int param_1)

{
  ushort uVar1;
  char cVar2;
  undefined4 *in_EAX;
  ushort *puVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 local_20 [8];

  cVar2 = '\x01';
  switch(*(undefined2 *)(param_1 + 0xeda)) {
  case 0:
  case 1:
    break;
  case 2:
    puVar6 = local_20;
    for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *in_EAX;
      in_EAX = in_EAX + 1;
      puVar6 = puVar6 + 1;
    }
    puVar3 = (ushort *)network_prepare_challenge_packet();
    if (puVar3 != (ushort *)0x0) {
      uVar1 = *puVar3;
      iVar4 = *(int *)(param_1 + 0xadc);
      goto LAB_004d90e5;
    }
    break;
  case 3:
    goto LAB_004d90b8;
  case 4:
LAB_004d90b8:
    puVar6 = local_20;
    for (iVar4 = 8; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar6 = *in_EAX;
      in_EAX = in_EAX + 1;
      puVar6 = puVar6 + 1;
    }
    puVar3 = (ushort *)network_prepare_challenge_packet();
    if (puVar3 != (ushort *)0x0) {
      uVar1 = *puVar3;
      iVar4 = *(int *)(param_1 + 0xadc);
LAB_004d90e5:
      iVar5 = (uint)(uVar1 >> 4) * 8;
      cVar2 = '\x01';
      if (((*(byte *)(iVar4 + 0xa8c) & 1) == 0) &&
         ((iVar5 + 1 <=
           ((*(int *)(iVar4 + 0x24) + *(int *)(iVar4 + 0x1c) * -8) - *(int *)(iVar4 + 0x20)) + 1 ||
          (cVar2 = FUN_004ddb60(iVar4,1), cVar2 != '\0')))) {
        *(int *)(iVar4 + 0xa80) = *(int *)(iVar4 + 0xa80) + iVar5 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar5);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
      }
      return cVar2;
    }
    break;
  default:
    goto switchD_004d906f_default;
  }
  cVar2 = '\0';
switchD_004d906f_default:
  return cVar2;
}
#endif
