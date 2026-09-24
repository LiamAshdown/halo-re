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
// UNSURE: the buffer/local_602 stack layout (declared by Ghidra as 7 dwords plus one trailing
// byte, 29 bytes, but filled by an 8-dword/32-byte copy) is modeled as one 32-byte buffer, with
// `local_602` at its byte offset 30 (0x620-0x602), which is the only reading consistent with
// the copy loop's own size.
// UNSURE: the returned "garbage" upper three bytes come from `client` itself here (unlike the
// sibling functions in this cluster, which reuse a decoded-record pointer); simplified to a
// plain 0/1 return per the "callers only read the low byte" idiom used throughout this module.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t data_packet_group_encode_packet(uint8_t *buffer, int16_t **capacity,
    int32_t packet_type, int32_t version); // 0x4d0ae0; UNSURE, this call site's own 4-arg shape
extern uint16_t network_challenge_packet_block[]; // 0x006b7f98, the reused message block
extern uint16_t *network_message_block_build(uint16_t *buffer, uint32_t *source, uint8_t flags, uint32_t length); // 0x440350, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, blam-cc: value in EDX, stream in ESI

// blam-cc: EDX -> source, stack -> client
int32_t network_game_record_message_send(network_client_globals *client, const uint32_t *source)
{
    uint8_t buffer[32];
    uint32_t *dst;
    int32_t i;
    int16_t *capacity;
    uint16_t *record;
    uint8_t *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    int32_t retransmit_ok;

    dst = (uint32_t *)buffer;
    for (i = 8; i != 0; i = i - 1) {
        *dst = *source;
        source = source + 1;
        dst = dst + 1;
    }
    if (buffer[30] == 0xff) {
        buffer[30] = 0;
    }

    capacity = (int16_t *)0x600;
    if ((char)data_packet_group_encode_packet(buffer, &capacity, 0x12, 1) == 0) {
        return 0;
    }

    // FIXED in the review pass: network_message_block_build takes the destination block in
    // EAX, the source buffer in ECX, a 2-bit flag value in DL and the byte length on the
    // stack. Every inlined copy of this idiom in the module is the same four operands --
    // `push <encoded length> / mov eax,0x6b7f98 / lea ecx,[<buffer>] / mov dl,3` -- so the
    // first draft's single-argument call was passing the length where the destination goes.
    record = network_message_block_build(network_challenge_packet_block, (uint32_t *)buffer, 3,
                                        (uint32_t)(int32_t)capacity); // 0x4da18d
    if (record == 0) {
        return 0;
    }

    channel = (uint8_t *)client->channel;
    bits_to_send = (uint32_t)(*record >> 4) * 8;
    total_bits = bits_to_send + 1;
    if ((*(uint8_t *)(channel + 0xa8c) & 1) == 0) {
        if (((*(int32_t *)(channel + 0x24) + *(int32_t *)(channel + 0x1c) * -8) -
                 *(int32_t *)(channel + 0x20)) + 1 < total_bits) {
            retransmit_ok = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1);
            if (retransmit_ok == 0) {
                return 0;
            }
        }
        {
            uint32_t unaff_write_value;
            bit_stream *unaff_write_stream;

            *(int32_t *)(channel + 0xa80) = *(int32_t *)(channel + 0xa80) + total_bits;
            bit_stream_write_bits_chunked(1, unaff_write_value, unaff_write_stream);
            *(uint8_t *)(channel + 0x2c) = 0;
            bit_stream_write_bits_chunked(bits_to_send, unaff_write_value, unaff_write_stream);
            *(uint8_t *)(channel + 0x2c) = 0;
        }
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
