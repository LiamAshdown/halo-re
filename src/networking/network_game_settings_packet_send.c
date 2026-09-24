// network_game_settings_packet_send  (Ghidra: FUN_004d94c0; renamed, no prior name)
// address 0x4d94c0, size 568 bytes
// name confidence: 0.4   rewrite confidence: 0.2 (LOW -- see UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Builds and sends a large
// game-settings/map-data packet (including the player's name) to a newly-joining client whose
// session id doesn't yet match ours"). Word-indexed offsets on `unaff_EBX` (an `undefined2 *`)
// resolve cleanly against types/networking.h's network_client_globals when doubled: word 0x76d
// (byte 0xeda) is state, word 0x76f (byte 0xede) is unknown_ede, word 0x5cc (byte 0xb98) is
// exactly &client->session.server_name, word 0x56e (byte 0xadc) is channel, word 0x771 (byte
// 0xee2) is pad_ee2, word 0x788 (byte 0xf10) is unknown_f10.
// register convention: the client pointer arrives in EBX (unaff_EBX); `param_1` is the sole
// cdecl stack parameter (the incoming request record). // blam-cc: EBX -> client, stack -> request
// UNSURE (major): Ghidra's own decompile carries a "Function: __chkstk replaced with injection:
// alloca_probe" warning, and the local-variable list it recovered cannot be correct: a loop
// copies 0x7ff dwords (8188 bytes) into what it calls the 4-byte local `local_2008`, which is
// far larger than the ~0x1ac-byte frame the other named locals imply. This is modeled here as
// one shared byte buffer (`frame`) sized to hold the largest offset actually touched, with every
// named Ghidra local placed at its byte delta from `local_2098` (the lowest-addressed local);
// `local_1eee`, whose Ghidra offset falls inside the big copy's destination range, is read back
// out of the copied template rather than treated as a separately-initialized variable, since
// nothing in the decompiled body ever writes it directly -- this is the only reading of the
// layout consistent with the code as shown.
// UNSURE: `network_prepare_challenge_packet` is called with no visible argument; since every
// local this function builds (`frame`) is otherwise unused after being populated, `&frame[0]`
// is reconstructed as its argument, matching the same shape as network_session_info_packet_send.c.
// UNSURE: word offset 0x7a6 (byte 0xf4c) is exactly `sizeof(network_client_globals)` -- one byte
// past the end of the struct. Preserved as a raw read at that fixed address (very likely landing
// on whatever global the original linker placed immediately after `network_client_storage`),
// not folded into any named field.
// UNSURE: `FUN_00617c70` (0x617c70, GameSpy-adjacent range, not in this batch) and the globals
// `DAT_007461a8` / `DAT_00712dd8` (a large template table) / `DAT_0068e688` are not declared in
// types/networking.h; named generically below.
// UNSURE: the byte-at-a-time strcmp-shaped loop's sign convention (`(1-less)-(less!=0)`) is
// preserved as literal arithmetic rather than replaced with a library strcmp call, since the
// original never calls one here.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

#include "tags.h"
#include "memory.h"
#include <wchar.h>
#include "math.h"
#include "game.h"
#include "networking.h"

extern void main_queue_map_change_by_name_or_clear(void); // 0x4c87a0
extern int32_t join_ui_state; // 0x00718f8c, per network_client_state_dispatch.c cluster naming
extern int32_t some_global_0068e688; // 0x0068e688, UNSURE name
extern void *game_variant_description_template_source; // 0x007461a8, UNSURE name/type
extern uint8_t game_variant_description_template[0x1ffc]; // 0x00712dd8, UNSURE name/size (0x7ff dwords)
extern void FUN_00617c70(void *a, void *request, uint8_t *out); // 0x617c70, not in this batch
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(int32_t total_bit_count, uint32_t value,
    bit_stream *stream); // 0x4cf8f0, blam-cc: value in EDX, stream in ESI
extern wchar_t *_wcsncpy(wchar_t *dest, const wchar_t *source, int32_t count);

// blam-cc: EBX -> client, stack -> request
void network_game_settings_packet_send(network_client_globals *client, const uint8_t *request)
{
    // TYPES-GAP: see file header for why this is one shared buffer rather than named locals.
    // Byte deltas below are each local's Ghidra name subtracted from 0x2098 (local_2098's own
    // offset), i.e. the position of that local within this buffer.
    uint8_t frame[0x2100];
    uint8_t *pa, *pb;
    uint8_t a, b;
    int32_t cmp;
    uint32_t *zero_fill;
    int32_t i;
    int32_t *challenge;
    network_channel *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    if ((client->unknown_ede & 2) != 0) {
        return;
    }
    client->state = 2; // UNSURE: live connection-mode value, not padding
    client->unknown_000 = *(uint16_t *)(request + 0xc);

    pa = (uint8_t *)(request + 0x14);
    pb = (uint8_t *)client->session.server_name;
    while (1) {
        a = *pa;
        b = *pb;
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        if (a == 0) {
            cmp = 0;
            break;
        }
        a = pa[1];
        b = pb[1];
        if (a != b) {
            cmp = (1 - (uint32_t)(a < b)) - (uint32_t)((a < b) != 0);
            goto compare_done;
        }
        pa = pa + 2;
        pb = pb + 2;
        if (a == 0) {
            cmp = 0;
            break;
        }
    }
compare_done:
    if (cmp != 0) {
        main_queue_map_change_by_name_or_clear();
        if (join_ui_state != 1) {
            if (join_ui_state != 2 && join_ui_state == 4) {
                some_global_0068e688 = -1;
            }
            join_ui_state = 8;
        }
    }

    zero_fill = (uint32_t *)frame;
    for (i = 0x23; i != 0; i = i - 1) {
        *zero_fill = 0;
        zero_fill = zero_fill + 1;
    }
    *(uint16_t *)zero_fill = 0;

    *(uint32_t *)(frame + 0x00) = *(uint32_t *)((uint8_t *)client + 0xb02);
    *(uint32_t *)(frame + 0x04) = *(uint32_t *)((uint8_t *)client + 0xb06);
    *(uint32_t *)(frame + 0x08) = *(uint32_t *)((uint8_t *)client + 0xb0a);
    *(uint32_t *)(frame + 0x0c) = *(uint32_t *)((uint8_t *)client + 0xb0e);
    *(uint8_t *)&client->pad_ee2 = 0;
    _wcsncpy((wchar_t *)(frame + 0x10), (const wchar_t *)((uint8_t *)client + 0xaf0), 8);
    frame[0x6b] = *((uint8_t *)client + 0xf4c); // UNSURE: one byte past the struct; see file header
    *(uint16_t *)(frame + 0x20) = 0;
    FUN_00617c70(game_variant_description_template_source, (void *)request, frame + 0x22);

    // The big, likely-mis-sized copy; see file header.
    for (i = 0; i < 0x1ffc; i = i + 1) {
        frame[0x90 + i] = game_variant_description_template[i];
    }

    *(uint8_t *)(frame + 0x8a) = request[0xc];
    frame[0x8b] = 0;
    _wcsncpy((wchar_t *)(frame + 0x6e), (const wchar_t *)(frame + 0x92), 0xb);
    frame[0x8c] = *(uint8_t *)&client->unknown_f10;
    *(uint16_t *)(frame + 0x84) = 0;
    *(uint16_t *)(frame + 0x86) = *(uint16_t *)(frame + 0x1aa); // local_2012 = local_1eee (inside the copied template)
    *(uint16_t *)(frame + 0x88) = 0xffff;
    frame[0x8d] = 0xff;
    *(uint8_t *)&client->pad_ee2 = 1;

    // 0x4d9626: eax = 0x0e; 0x4d9622: edx = the staged settings frame.
    challenge = (int32_t *)network_prepare_challenge_packet(0x0e, frame);
    if (challenge != 0) {
        channel = client->channel;
        bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
        if ((channel->flags & 1) == 0) {
            if ((((*(int32_t *)((uint8_t *)channel + 0x24) +
                   *(int32_t *)((uint8_t *)channel + 0x1c) * -8) -
                  *(int32_t *)((uint8_t *)channel + 0x20)) + 1 < bits_to_send + 1) &&
                (retransmit_ok = network_channel_stream_flush(&channel->outgoing, channel, 1), retransmit_ok == 0)) {
                return;
            }
            {
                uint32_t unaff_write_value;
                bit_stream *unaff_write_stream;

                channel->send_budget = channel->send_budget + bits_to_send + 1;
                bit_stream_write_bits_chunked(1, unaff_write_value, unaff_write_stream);
                *((uint8_t *)channel + 0x2c) = 0;
                bit_stream_write_bits_chunked(bits_to_send, unaff_write_value, unaff_write_stream);
                *((uint8_t *)channel + 0x2c) = 0;
            }
        }
        client->unknown_ede = client->unknown_ede | 2;
    }
}

#if 0
Original Ghidra decompilation (0x4d94c0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

void FUN_004d94c0(int param_1)

{
  byte bVar1;
  char cVar2;
  int iVar3;
  ushort *puVar4;
  undefined2 *unaff_EBX;
  byte *pbVar5;
  undefined4 *puVar6;
  byte *pbVar7;
  undefined4 *puVar8;
  int iVar9;
  bool bVar10;
  undefined4 local_2098 [4];
  wchar_t local_2088 [8];
  undefined2 local_2078;
  undefined1 local_2076 [73];
  undefined1 local_202d;
  wchar_t local_202a [11];
  undefined2 local_2014;
  undefined2 local_2012;
  undefined2 local_2010;
  undefined1 local_200e;
  undefined1 local_200d;
  undefined1 local_200c;
  undefined1 local_200b;
  undefined4 local_2008;
  undefined2 local_1eee;
  undefined4 uStack_c;

  uStack_c = 0x4d94d0;
  if ((*(byte *)(unaff_EBX + 0x76f) & 2) == 0) {
    unaff_EBX[0x76d] = 2;
    *unaff_EBX = *(undefined2 *)(param_1 + 0xc);
    pbVar7 = (byte *)(param_1 + 0x14);
    pbVar5 = (byte *)(unaff_EBX + 0x5cc);
    do {
      bVar1 = *pbVar7;
      bVar10 = bVar1 < *pbVar5;
      if (bVar1 != *pbVar5) {
LAB_004d9524:
        iVar3 = (1 - (uint)bVar10) - (uint)(bVar10 != 0);
        goto LAB_004d9529;
      }
      if (bVar1 == 0) break;
      bVar1 = pbVar7[1];
      bVar10 = bVar1 < pbVar5[1];
      if (bVar1 != pbVar5[1]) goto LAB_004d9524;
      pbVar7 = pbVar7 + 2;
      pbVar5 = pbVar5 + 2;
    } while (bVar1 != 0);
    iVar3 = 0;
LAB_004d9529:
    if ((iVar3 != 0) && (main_queue_map_change_by_name_or_clear(), DAT_00718f8c != 1)) {
      if ((DAT_00718f8c != 2) && (DAT_00718f8c == 4)) {
        DAT_0068e688 = 0xffffffff;
      }
      DAT_00718f8c = 8;
    }
    puVar6 = local_2098;
    for (iVar3 = 0x23; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = 0;
      puVar6 = puVar6 + 1;
    }
    *(undefined2 *)puVar6 = 0;
    local_2098[0] = *(undefined4 *)(unaff_EBX + 0x581);
    local_2098[1] = *(undefined4 *)(unaff_EBX + 0x583);
    local_2098[2] = *(undefined4 *)(unaff_EBX + 0x585);
    local_2098[3] = *(undefined4 *)(unaff_EBX + 0x587);
    *(undefined1 *)(unaff_EBX + 0x771) = 0;
    _wcsncpy(local_2088,unaff_EBX + 0x578,8);
    local_202d = *(undefined1 *)(unaff_EBX + 0x7a6);
    local_2078 = 0;
    FUN_00617c70(DAT_007461a8,param_1,local_2076);
    puVar6 = &DAT_00712dd8;
    puVar8 = &local_2008;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar8 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar8 = puVar8 + 1;
    }
    local_200e = *(undefined1 *)(param_1 + 0xc);
    local_200d = 0;
    _wcsncpy(local_202a,(wchar_t *)((int)&local_2008 + 2),0xb);
    local_200c = *(undefined1 *)(unaff_EBX + 0x788);
    local_2014 = 0;
    local_2012 = local_1eee;
    local_2010 = 0xffff;
    local_200b = 0xff;
    *(undefined1 *)(unaff_EBX + 0x771) = 1;
    puVar4 = (ushort *)network_prepare_challenge_packet();
    if (puVar4 != (ushort *)0x0) {
      iVar3 = *(int *)(unaff_EBX + 0x56e);
      iVar9 = (uint)(*puVar4 >> 4) * 8;
      if ((*(byte *)(iVar3 + 0xa8c) & 1) == 0) {
        if ((((*(int *)(iVar3 + 0x24) + *(int *)(iVar3 + 0x1c) * -8) - *(int *)(iVar3 + 0x20)) + 1 <
             iVar9 + 1) && (cVar2 = FUN_004ddb60(iVar3,1), cVar2 == '\0')) {
          return;
        }
        *(int *)(iVar3 + 0xa80) = *(int *)(iVar3 + 0xa80) + iVar9 + 1;
        bit_stream_write_bits_chunked(1);
        *(undefined1 *)(iVar3 + 0x2c) = 0;
        bit_stream_write_bits_chunked(iVar9);
        *(undefined1 *)(iVar3 + 0x2c) = 0;
      }
      *(byte *)(unaff_EBX + 0x76f) = *(byte *)(unaff_EBX + 0x76f) | 2;
    }
  }
  return;
}
#endif
