// network_session_player_join_notify  (Ghidra: FUN_004d9700; renamed, no prior name)
// address 0x4d9700, size 218 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("Records a newly-joined player's index
// into the active session state and queues a notification packet announcing the join"). Shares
// the challenge/send idiom (channel+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c) with the other
// functions in this cluster. `in_EAX+0x76d` (word index, byte 0xeda) is client->state;
// `in_EAX+0x56e` (byte 0xadc) is client->channel; server+0x3ac and client+0xeb8 both land at
// session-relative offset 0x3a4 (2 bytes into types/networking.h's opaque
// network_game_session::unknown_3a2[10]), confirming both containers embed the same session.
// register convention: client (word-indexed base) in EAX, a 2-dword source record in ECX.
// // blam-cc: EAX -> client, ECX -> source
// UNSURE: `client->unknown_000` is declared in types/networking.h as initialized to 0xffff by
// network_session_create; here it is overwritten with the validated 0..15 player index, so it
// appears to double as a "pending/just-joined player index" scratch field. Not renamed (header
// not editable here).
// UNSURE: source[0] (the dword copied into session.unknown_3a2+2) and source[1]'s low word (the
// validated player index) have no further-resolved meaning beyond their evidenced roles here.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: EAX -> client, ECX -> source
void network_session_player_join_notify(network_client_globals *client, const uint32_t *source)
{
    int16_t player_index;
    uint32_t group_value;
    int32_t *challenge;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at; its
                                   //         contents are not visible in the decompilation
    network_channel *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    char retransmit_ok;

    player_index = *(int16_t *)(source + 1);
    if (player_index < 0 || player_index >= 0x10) {
        return;
    }
    client->unknown_000 = player_index; // UNSURE: repurposed as the joining player's index here
    group_value = *source;
    client->state = 2; // UNSURE: live connection-mode value, not padding

    if (network_server != 0) {
        *(uint32_t *)&network_server->session.unknown_3a2[2] = group_value;
    }
    if (network_client != 0) {
        *(uint32_t *)&network_client->session.unknown_3a2[2] = group_value;
    }

    // 0x4d974d: eax = 0x11; 0x4d9749: edx = the staged join-notify scratch.
    challenge = (int32_t *)network_prepare_challenge_packet(0x11, challenge_payload);
    if (challenge != 0) {
        channel = client->channel;
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        total_bits = bits_to_send + 1;
        if ((channel->flags & 1) == 0) {
            if (total_bits <= ((*(int32_t *)((uint8_t *)channel + 0x24) +
                                 *(int32_t *)((uint8_t *)channel + 0x1c) * -8) -
                                *(int32_t *)((uint8_t *)channel + 0x20)) + 1 ||
                (retransmit_ok = network_channel_stream_flush(&channel->outgoing, channel, 1), retransmit_ok != 0)) {

                channel->send_budget = channel->send_budget + total_bits;
                { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                *((uint8_t *)channel + 0x2c) = 0;
                bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                *((uint8_t *)channel + 0x2c) = 0;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4d9700):

void FUN_004d9700(void)

{
  int iVar1;
  short sVar2;
  undefined4 uVar3;
  int iVar4;
  char cVar5;
  short *in_EAX;
  ushort *puVar6;
  undefined4 *in_ECX;
  int iVar7;
  bool bVar8;

  sVar2 = *(short *)(in_ECX + 1);
  if ((-1 < sVar2) && (sVar2 < 0x10)) {
    *in_EAX = sVar2;
    iVar1 = DAT_0071c2d4;
    bVar8 = DAT_0071c2d4 != 0;
    in_EAX[0x76d] = 2;
    uVar3 = *in_ECX;
    if (bVar8) {
      *(undefined4 *)(iVar1 + 0x3ac) = uVar3;
    }
    if (DAT_0071c2d8 != 0) {
      *(undefined4 *)(DAT_0071c2d8 + 0xeb8) = uVar3;
    }
    puVar6 = (ushort *)network_prepare_challenge_packet();
    if (puVar6 != (ushort *)0x0) {
      iVar4 = *(int *)(in_EAX + 0x56e);
      iVar7 = (uint)(*puVar6 >> 4) * 8;
      iVar1 = iVar7 + 1;
      if (((*(byte *)(iVar4 + 0xa8c) & 1) == 0) &&
         ((iVar1 <= ((*(int *)(iVar4 + 0x24) + *(int *)(iVar4 + 0x1c) * -8) - *(int *)(iVar4 + 0x20)
                    ) + 1 || (cVar5 = FUN_004ddb60(iVar4,1), cVar5 != '\0')))) {
        *(int *)(iVar4 + 0xa80) = *(int *)(iVar4 + 0xa80) + iVar1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar7);
        *(undefined1 *)(iVar4 + 0x2c) = 0;
      }
    }
  }
  return;
}
#endif
