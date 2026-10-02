// network_game_settings_ack_send  (Ghidra: FUN_004d9f50; renamed, no prior name)
// address 0x4d9f50, size 455 bytes
// name confidence: 0.4   rewrite confidence: 0.25 (LOW -- same stack-frame caveat as
// network_game_settings_packet_send.c)
// evidence: out/phase4/networking_functions.md summary ("Builds and queues a small
// acknowledgement message while the local connection is in state 2 or 3 (used right after
// receiving the game-settings/map message); a no-op otherwise"); called from
// network_game_settings_packet_receive.c (0x4d9800, same task batch) the first time a
// game-settings packet is applied.
// register convention: `param_1` (client, byte-offset based here) and `param_2` (a template
// table row index) are both genuine cdecl stack parameters this time (Ghidra recovered them).
// // blam-cc: stack -> client, template_row
// UNSURE (major): same "Function: __chkstk replaced with injection: alloca_probe" stack-frame
// caveat as network_game_settings_packet_send.c applies here too. The named Ghidra locals are
// modeled as one shared byte buffer (`frame`), with each local's offset computed as
// 0x204e (local_204e's own suffix, the lowest-addressed named local) minus that local's suffix;
// this makes the later "copy 8 dwords starting at local_2048 into local_2028" and "local_2030 =
// local_1eee" statements consistent contiguous-memory operations instead of nonsensical
// small-to-small copies, which is the only reading that fits the byte math.
// UNSURE: `DAT_00712dd8` is the same template table referenced (without an index) in
// network_game_settings_packet_send.c; here it is indexed by `template_row * 0x801` dwords
// (0x801, one more than the 0x7ff-dword copy size, implying each row carries one extra trailing
// dword this function never reads).

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t profile_globals_block[]; // 0x00712dd8, UNSURE name; row stride 0x801 dwords
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: stack -> client, template_row
char network_game_settings_ack_send(uint8_t *client, int16_t template_row)
{
    // TYPES-GAP: one shared buffer; see file header for the offset derivation.
    uint8_t frame[0x2070];
    uint32_t *dst, *src;
    int32_t i;
    int32_t mode;
    int32_t *challenge;
    uint8_t *channel;
    int32_t bits_to_send;
    int32_t total_bits;
    int32_t free_bits;
    char result;

    src = &profile_globals_block[(uint32_t)template_row * 0x801];
    dst = (uint32_t *)(frame + 0x46);
    for (i = 0x7ff; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }
    *(uint8_t *)(frame + 0x23) = (uint8_t)template_row;
    *(uint8_t *)(frame + 0x22) = *client;
    wcsncpy((wchar_t *)(frame + 0x6), (const wchar_t *)(frame + 0x48), 0xb);
    *(uint16_t *)(frame + 0x1e) = *(uint16_t *)(frame + 0x160);
    *(uint16_t *)(frame + 0x20) = 0xffff;
    *(uint8_t *)(frame + 0x24) = 0xff;
    *(uint8_t *)(frame + 0x25) = 0xff;
    *(uint16_t *)(frame + 0x1c) = 0;

    mode = *(int16_t *)(client + 0xeda); // UNSURE: live connection-mode value, not padding
    switch (mode) {
    case 0:
    case 1:
    case 4:
        return 0;
    case 2:
    case 3:
        for (i = 0; i < 8; i = i + 1) {
            ((uint32_t *)(frame + 0x26))[i] = ((uint32_t *)(frame + 0x6))[i];
        }
        // 0x4d9ff2: eax = 0x0f; 0x4d9fee: edx = frame + 0x26. (The 0x4da05f sibling path in
        // this same function uses type 0x1c with the same payload.)
        challenge = (int32_t *)network_prepare_challenge_packet(0x0f, frame + 0x26);
        if (challenge == 0) {
            return 1;
        }
        channel = *(uint8_t **)(client + 0xadc);
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        total_bits = bits_to_send + 1;
        if ((*(uint8_t *)&((network_channel *)channel)->flags & 1) != 0) {
            return 1;
        }
        free_bits = ((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                     *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1;
        break;
    default:
        return 1;
    }

    result = 1;
    if (total_bits <= free_bits || (result = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), result != 0)) {

        ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + bits_to_send + 1;
        { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
        ((network_channel *)channel)->outgoing.empty = 0;
        bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
        ((network_channel *)channel)->outgoing.empty = 0;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4d9f50):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

char FUN_004d9f50(undefined1 *param_1,short param_2)

{
  int iVar1;
  ushort *puVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  wchar_t *pwVar7;
  undefined4 *puVar8;
  int iVar9;
  bool bVar10;
  bool bVar11;
  char local_204e;
  wchar_t local_2048 [11];
  undefined2 local_2032;
  undefined2 local_2030;
  undefined2 local_202e;
  undefined1 local_202c;
  undefined1 local_202b;
  undefined1 local_202a;
  undefined1 local_2029;
  undefined4 local_2028 [8];
  undefined4 local_2008;
  undefined2 local_1eee;
  undefined4 uStack_c;

  uStack_c = 0x4d9f60;
  puVar6 = &DAT_00712dd8 + param_2 * 0x801;
  puVar8 = &local_2008;
  for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar8 = *puVar6;
    puVar6 = puVar6 + 1;
    puVar8 = puVar8 + 1;
  }
  local_202b = (undefined1)param_2;
  local_202c = *param_1;
  _wcsncpy(local_2048,(wchar_t *)((int)&local_2008 + 2),0xb);
  local_2030 = local_1eee;
  local_202e = 0xffff;
  local_202a = 0xff;
  local_2029 = 0xff;
  local_2032 = 0;
  switch(*(undefined2 *)(param_1 + 0xeda)) {
  case 0:
  case 1:
  case 4:
    return '\0';
  case 2:
    pwVar7 = local_2048;
    puVar6 = local_2028;
    for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *(undefined4 *)pwVar7;
      pwVar7 = pwVar7 + 2;
      puVar6 = puVar6 + 1;
    }
    puVar2 = (ushort *)network_prepare_challenge_packet();
    if (puVar2 == (ushort *)0x0) {
      return '\x01';
    }
    iVar9 = *(int *)(param_1 + 0xadc);
    iVar5 = (uint)(*puVar2 >> 4) * 8;
    iVar3 = iVar5 + 1;
    if ((*(byte *)(iVar9 + 0xa8c) & 1) != 0) {
      return '\x01';
    }
    iVar4 = ((*(int *)(iVar9 + 0x24) + *(int *)(iVar9 + 0x1c) * -8) - *(int *)(iVar9 + 0x20)) + 1;
    bVar11 = SBORROW4(iVar3,iVar4);
    iVar1 = iVar3 - iVar4;
    bVar10 = iVar3 == iVar4;
    break;
  case 3:
    pwVar7 = local_2048;
    puVar6 = local_2028;
    for (iVar3 = 8; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *(undefined4 *)pwVar7;
      pwVar7 = pwVar7 + 2;
      puVar6 = puVar6 + 1;
    }
    puVar2 = (ushort *)network_prepare_challenge_packet();
    if (puVar2 == (ushort *)0x0) {
      return '\x01';
    }
    iVar9 = *(int *)(param_1 + 0xadc);
    iVar5 = (uint)(*puVar2 >> 4) * 8;
    iVar3 = iVar5 + 1;
    if ((*(byte *)(iVar9 + 0xa8c) & 1) != 0) {
      return '\x01';
    }
    iVar4 = ((*(int *)(iVar9 + 0x24) + *(int *)(iVar9 + 0x1c) * -8) - *(int *)(iVar9 + 0x20)) + 1;
    bVar11 = SBORROW4(iVar3,iVar4);
    iVar1 = iVar3 - iVar4;
    bVar10 = iVar3 == iVar4;
    break;
  default:
    return '\x01';
  }
  local_204e = '\x01';
  if ((bVar10 || bVar11 != iVar1 < 0) || (local_204e = FUN_004ddb60(iVar9,1), local_204e != '\0')) {
    *(int *)(iVar9 + 0xa80) = *(int *)(iVar9 + 0xa80) + iVar5 + 1;
    bit_stream_write_bits_chunked(1);
    *(undefined1 *)(iVar9 + 0x2c) = 0;
    bit_stream_write_bits_chunked(iVar5);
    *(undefined1 *)(iVar9 + 0x2c) = 0;
  }
  return local_204e;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
