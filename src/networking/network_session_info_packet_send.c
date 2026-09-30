// network_session_info_packet_send  (Ghidra: FUN_004d9050; renamed, no prior name)
// address 0x4d9050, size 292 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md summary ("Encodes and queues a session-info
// packet for the connection, when its connection-mode field indicates that one is needed").
// register convention: the 8-dword source record arrives in EAX (in_EAX); client is the sole
// cdecl stack parameter. // blam-cc: EAX -> source, stack -> client
// UNSURE: `network_prepare_challenge_packet` (0x4deaf0, outside this task's range) is called
// with no visible argument right after `source` is copied into an 8-dword local; reconstructed
// as taking that local's address, since nothing else in scope makes sense as its argument.
// UNSURE: the free-space check and the two bit_stream_write_bits_chunked calls read/write
// channel+0xa8c/+0x24/+0x1c/+0x20/+0xa80/+0x2c via raw offsets rather than through
// types/networking.h's named `in`/`out` network_channel_stream sub-objects: offsets 0x1c/0x20/0x24
// land inside `channel->in` (the *receive*-side stream) by the header's own layout, which is
// surprising for what looks like outgoing-bandwidth bookkeeping (send_budget at +0xa80 is
// unambiguously an outgoing-side field). The same unresolved raw-offset pattern, on the same
// four fields plus +0x2c, already appears in src/game/game_engine_send_team_allegiance_message.c
// (reviewed in an earlier session) without being resolved there either, so it is left exactly as
// raw offsets here too rather than guessed into (possibly wrong) named fields; worth revisiting
// in a future types-reconciliation pass together with that file.
// UNSURE: bit_stream_write_bits_chunked's value/stream arguments (EDX/ESI, per its own
// established signature in src/memory/bit_stream_write_bits_chunked.c) are not reloaded
// anywhere in this function's own body; per the same precedent file, they must already be live
// from network_prepare_challenge_packet's return sequence and are modeled as uninitialized
// locals rather than invented.
// UNSURE (major, preserved exactly): mode 0 and 1 explicitly return 0 (not ready), and modes 2/3/4
// with the challenge packet not yet ready also return 0 -- but any *other* mode value falls to
// the switch's `default:` case, which jumps directly past the `result = 0;` reset and returns the
// function's initial value of 1. This asymmetry (unhandled modes reporting "sent" while the two
// explicitly-idle modes report "not sent") is exactly what Ghidra decompiles; not simplified away.

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
#include "fn_memory.h"
#include "fn_networking.h"

extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module


// blam-cc: EAX -> source, stack -> client
char network_session_info_packet_send(const uint32_t *source, network_client_globals *client)
{
    char result;
    uint32_t local_buffer[8];
    uint16_t *challenge;
    uint16_t challenge_word;
    uint8_t *channel;
    int32_t bits_to_send;
    int32_t i;

    result = 1;
    switch (client->state) { // UNSURE: live connection-mode value, not padding
    case 0:
    case 1:
        break;
    case 2:
        for (i = 0; i < 8; i = i + 1) {
            local_buffer[i] = source[i];
        }
        // 0x4d9083: eax = 0x10; 0x4d907f: edx = local_buffer.
        challenge = network_prepare_challenge_packet(0x10, local_buffer);
        if (challenge != 0) {
            challenge_word = *challenge;
            channel = (uint8_t *)client->channel;
            goto have_challenge;
        }
        break;
    case 3:
        goto case_3_or_4;
    case 4:
case_3_or_4:
        for (i = 0; i < 8; i = i + 1) {
            local_buffer[i] = source[i];
        }
        // 0x4d90b3: eax = 0x23; 0x4d90c1: edx = the caller-supplied payload in EDI.
        challenge = network_prepare_challenge_packet(0x23, local_buffer);
        if (challenge != 0) {
            challenge_word = *challenge;
            channel = (uint8_t *)client->channel;
have_challenge:
            bits_to_send = (uint32_t)(challenge_word >> 4) * 8;
            result = 1;
            if (((*(uint8_t *)&((network_channel *)channel)->flags & 1) == 0) &&
                (bits_to_send + 1 <=
                     ((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                      *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 ||
                 (result = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), result != 0))) {

                ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + bits_to_send + 1;
                { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                ((network_channel *)channel)->outgoing.empty = 0;
                bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                ((network_channel *)channel)->outgoing.empty = 0;
            }
            return result;
        }
        break;
    default:
        return result; // UNSURE: preserves the default-case returns-1 asymmetry; see file header
    }
    result = 0;
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
