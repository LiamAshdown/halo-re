// network_host_presence_broadcast_tick  (Ghidra: FUN_004dadb0; renamed, no prior name)
// address 0x4dadb0, size 313 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md summary ("Periodically (every second) builds and
// queues a short broadcast-style message carrying a global status string, consistent with a
// hosted-game LAN presence announcement"). client+0xed4 matches types/networking.h's
// network_client_globals::unknown_ed4 exactly.
// register convention: the client pointer arrives in EAX (in_EAX). // blam-cc: EAX -> client
// UNSURE: `cache_file_request_map(1)`'s real signature/argument meaning is not resolved here;
// declared generically from this call site.

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from challenge; the C passed placeholders or dropped the arguments.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include <string.h>
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern char cache_file_request_map(int32_t unknown); // 0x442640, UNSURE argument
extern char network_build_string[]; // 0x00719879
extern uint16_t *network_prepare_challenge_packet(int32_t message_type, void *payload); // 0x4deaf0, this module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits

// blam-cc: EAX -> client
void network_host_presence_broadcast_tick(network_client_globals *client)
{
    large_integer counter;
    int32_t now_ms;
    uint8_t buffer[256];
    int32_t *challenge;
    uint8_t *channel;
    int32_t bits_to_send;
    char retransmit_ok;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);

    if (client->last_presence_broadcast_ms + 1000 < now_ms) {
        client->last_presence_broadcast_ms = now_ms;
        if (cache_file_request_map(1) != 0) { // UNSURE argument
            memset(buffer, 0, sizeof(buffer));
            strncpy((char *)buffer, network_build_string, 0x100);

            // 0x4dae54: eax = 0x15; 0x4dae50: edx = the staged announcement scratch.
            challenge = (int32_t *)network_prepare_challenge_packet(0x15, buffer);
            if (challenge != 0) {
                channel = (uint8_t *)client->channel;
                bits_to_send = (uint32_t)(*(uint16_t *)challenge >> 4) * 8;
                if ((*(uint8_t *)&((network_channel *)channel)->flags & 1) == 0) {
                    if ((((*(int32_t *)&((network_channel *)channel)->outgoing.stream.last_bit + *(int32_t *)&((network_channel *)channel)->outgoing.stream.byte_cursor * -8) -
                          *(int32_t *)&((network_channel *)channel)->outgoing.stream.bit_cursor) + 1 < bits_to_send + 1) &&
                        (retransmit_ok = network_channel_stream_flush((network_channel_stream *)(channel + 0x10), (network_channel *)channel, 1), retransmit_ok == 0)) {
                        return;
                    }
                    {

                        ((network_channel *)channel)->send_budget = ((network_channel *)channel)->send_budget + bits_to_send + 1;
                        { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), &item_flag, 1); }
                        ((network_channel *)channel)->outgoing.empty = 0;
                        bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)channel + 0x10), (const uint32_t *)(challenge), bits_to_send);
                        ((network_channel *)channel)->outgoing.empty = 0;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4dadb0):

void FUN_004dadb0(void)

{
  char cVar1;
  int in_EAX;
  int iVar2;
  undefined4 *puVar3;
  int iVar4;
  undefined8 uVar5;
  LARGE_INTEGER local_110;
  char local_108;
  undefined4 local_107;

  QueryPerformanceCounter(&local_110);
  uVar5 = __allmul(local_110.s.LowPart,local_110.s.HighPart,1000,0);
  iVar2 = __alldiv(uVar5,DAT_006ac8f8,DAT_006ac8fc);
  if (*(int *)(in_EAX + 0xed4) + 1000 < iVar2) {
    *(int *)(in_EAX + 0xed4) = iVar2;
    cVar1 = cache_file_request_map(1);
    if (cVar1 != '\0') {
      local_108 = '\0';
      puVar3 = &local_107;
      for (iVar2 = 0x3f; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar3 = 0;
        puVar3 = puVar3 + 1;
      }
      *(undefined2 *)puVar3 = 0;
      *(undefined1 *)((int)puVar3 + 2) = 0;
      _strncpy(&local_108,&DAT_00719879,0x100);
      local_110.s.LowPart = network_prepare_challenge_packet();
      if ((ushort *)local_110.s.LowPart != (ushort *)0x0) {
        iVar2 = *(int *)(in_EAX + 0xadc);
        iVar4 = (uint)(*(ushort *)local_110.s.LowPart >> 4) * 8;
        if ((*(byte *)(iVar2 + 0xa8c) & 1) == 0) {
          if ((((*(int *)(iVar2 + 0x24) + *(int *)(iVar2 + 0x1c) * -8) - *(int *)(iVar2 + 0x20)) + 1
               < iVar4 + 1) && (cVar1 = FUN_004ddb60(iVar2,1), cVar1 == '\0')) {
            return;
          }
          *(int *)(iVar2 + 0xa80) = *(int *)(iVar2 + 0xa80) + iVar4 + 1;
          bit_stream_write_bits_chunked(1);
          *(undefined1 *)(iVar2 + 0x2c) = 0;
          bit_stream_write_bits_chunked(iVar4);
          *(undefined1 *)(iVar2 + 0x2c) = 0;
        }
      }
    }
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
