// network_staged_message_commit  (Ghidra: FUN_004da250; renamed, no prior name)
// address 0x4da250, size 199 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md summary ("Commits a previously-staged outgoing
// message to the send queue while the connection is in state 2; otherwise a no-op"). This
// function has zero callers anywhere in the binary (out/functions.json callers=0); rewritten
// anyway per the task instructions. Shares the challenge/send idiom with the rest of this
// cluster (network_session_info_packet_send.c etc).
// register convention: the client pointer arrives in ECX (in_ECX). // blam-cc: ECX -> client
// UNSURE: every path through this function returns the literal constant 1; preserved exactly
// (not simplified to `void`), matching Ghidra's own recovered `undefined4` return type.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, blam-cc: value in EDX, stream in ESI

// blam-cc: ECX -> client
int32_t network_staged_message_commit(network_client_globals *client)
{
    int32_t *challenge;
    uint32_t challenge_payload[4]; // UNSURE: the caller-frame scratch EDX points at; its
                                   //         contents are not visible in the decompilation
    uint8_t *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    if (client->state != 2) { // UNSURE: live connection-mode value, not padding
        return 1;
    }
    // 0x4da26e: eax = 0x13; 0x4da26a: edx = the staged message scratch.
    challenge = (int32_t *)network_prepare_challenge_packet(0x13, challenge_payload);
    if (challenge != 0) {
        channel = (uint8_t *)client->channel;
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        if ((*(uint8_t *)(channel + 0xa8c) & 1) == 0) {
            if ((((*(int32_t *)(channel + 0x24) + *(int32_t *)(channel + 0x1c) * -8) -
                  *(int32_t *)(channel + 0x20)) + 1 < bits_to_send + 1) &&
                (retransmit_ok = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), retransmit_ok == 0)) {
                return 1;
            }
            {
                uint32_t unaff_write_value;
                bit_stream *unaff_write_stream;

                *(int32_t *)(channel + 0xa80) = *(int32_t *)(channel + 0xa80) + bits_to_send + 1;
                bit_stream_write_bits_chunked(1, unaff_write_value, unaff_write_stream);
                *(uint8_t *)(channel + 0x2c) = 0;
                bit_stream_write_bits_chunked(bits_to_send, unaff_write_value, unaff_write_stream);
                *(uint8_t *)(channel + 0x2c) = 0;
            }
        }
        return 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4da250):

undefined4 FUN_004da250(void)

{
  int iVar1;
  char cVar2;
  ushort *puVar3;
  int in_ECX;
  int iVar4;

  if (*(short *)(in_ECX + 0xeda) != 2) {
    return 1;
  }
  puVar3 = (ushort *)network_prepare_challenge_packet();
  if (puVar3 != (ushort *)0x0) {
    iVar1 = *(int *)(in_ECX + 0xadc);
    iVar4 = (uint)(*puVar3 >> 4) * 8;
    if ((*(byte *)(iVar1 + 0xa8c) & 1) == 0) {
      if ((((*(int *)(iVar1 + 0x24) + *(int *)(iVar1 + 0x1c) * -8) - *(int *)(iVar1 + 0x20)) + 1 <
           iVar4 + 1) && (cVar2 = FUN_004ddb60(iVar1,1), cVar2 == '\0')) {
        return 1;
      }
      *(int *)(iVar1 + 0xa80) = *(int *)(iVar1 + 0xa80) + iVar4 + 1;
      bit_stream_write_bits_chunked(1);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
      bit_stream_write_bits_chunked(iVar4);
      *(undefined1 *)(iVar1 + 0x2c) = 0;
    }
    return 1;
  }
  return 1;
}
#endif
