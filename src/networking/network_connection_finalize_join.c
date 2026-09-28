// network_connection_finalize_join  (Ghidra: network_connection_finalize_join, already named)
// address 0x4d9960, size 892 bytes
// name confidence: 0.5   rewrite confidence: 0.25 (LOW -- very large function, several
// unresolved sub-regions; see UNSURE notes)
// evidence: out/phase4/networking_functions.md summary ("Finalizes a connection's transition
// into the joined/in-game state, sending the final join packet and arming post-join bookkeeping
// timers"). `connection` is `ushort *` per Ghidra's own recovered signature; every offset below
// is a WORD index doubled to a byte offset, matching this module's other word-indexed functions
// (network_client_state_dispatch.c's client+0xeda, etc). connection+0x56e (byte 0xadc) is
// channel; connection+0x58a (byte 0xb14) is &client->session; connection+0x760 (byte 0xec0) is
// session.unknown_3ac; connection+0x766..0x769 (bytes 0xecc, 0xece, 0xed0, 0xed2) span exactly
// unknown_ecc and unknown_ed0.
// register convention: __cdecl, single stack parameter `connection` (the client).
// // blam-cc: stack -> connection
// UNSURE (major): the player-machine search loop (`*connection` compared against
// `players[i].machine_index`, then `connection + i*0x10` recomputed and re-tested against
// `puVar7[0x669]`) re-derives its own cursor from `connection` directly rather than continuing
// from the byte-0xcd2 base the outer scan used, which does not read as ordinary array indexing.
// Transcribed literally as raw word-pointer arithmetic on `connection`, exactly as Ghidra shows,
// rather than reinterpreted into named player_entry accesses, since the second re-derivation
// cannot be reconciled with a single consistent array base.
// UNSURE: `DAT_0087a478` (used here as `DAT_0087a478 + 4 + index*4`, a flat base address, not a
// small fixed-size table as in network_disconnect_notify_dropped_machines.c) and
// `*(int*)(iVar6+0x34)` off `player_data` are not declared in types/memory.h or
// types/networking.h; named generically. The `sVar9` index this gates on is constrained to
// exactly 0 by its own range check (`-1 < v && v < 1`), consistent with
// network_player_entry::machine_player_index always being 0 on the PC build.
// UNSURE: `data_packet_group_encode_packet`'s output buffer is Ghidra's own `&local_610`, an
// 8-byte `LARGE_INTEGER` reused as a byte buffer -- almost certainly another instance of the
// stack-frame modeling problem seen in network_send_join_request_packet.c (whose analogous call
// uses a genuine 1540-byte buffer). Modeled here with an equivalently generously-sized local
// buffer instead of reusing the 8-byte QPC local, to avoid fabricating an out-of-bounds write.
// UNSURE: several globals (DAT_006b7f98/9a/9e, DAT_0068e684, DAT_0068e680) are not declared
// anywhere in types/networking.h; named generically from their read/write shapes only.
// reconciled: R01 0x0087ac06 int16 network_statistics_level -> uint8 debug_log_level (the binary reads a byte)

// FIXED in the review pass: this file's 2-argument guess at network_channel_stream_flush is
// resolved. Every message-send call site in the module is the same three operands --
// `lea esi,[channel+0x10]` (channel->outgoing), `push <channel>`, `push 1` -- e.g. 0x4d9108,
// 0x4d9698, 0x4d9791, 0x4d9bcd, 0x4da0af, 0x4da2b4, 0x4dae96, 0x4dce68 and 0x4de254.

// FIXED 2026-09-28 (send-path audit, from the disassembly): the two bit_stream_write_bits_chunked calls write into
// the channel's outgoing bit stream (channel +0x10, EAX): first the 1-bit item flag (0: a message record) from a local, then
// the encoded bits from &network_join_message_header; the C passed placeholders or dropped the arguments.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t debug_log_level;                  // 0x0087ac06, byte-wide (R01)
extern uint8_t network_statistics_logging_enabled; // 0x006f14b4
extern void *network_summary_log_file;           // 0x006a6140, FILE *
extern char network_build_string[];              // 0x00719879
extern int32_t _fprintf(void *stream, const char *format, ...);
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern int16_t network_game_mode; // 0x00719720
extern char network_game_scenario_load_request(network_game_session *session); // 0x4de6d0
extern data_array *player_data; // 0x0087a480
extern uint8_t *network_local_player_index_table; // 0x0087a478, UNSURE name; flat base address
extern int32_t player_data_iterator_advance(int16_t step_count); // 0x4d98f0
extern char network_player_entry_validate(void); // 0x4de9f0, UNSURE argument (none visible here); not in this batch
extern int32_t data_packet_group_encode_packet(uint8_t *buffer, uint32_t *capacity,
    int32_t packet_type, int32_t version); // 0x4d0ae0; UNSURE, this call site's own 4-arg shape
extern uint16_t network_join_message_header; // 0x006b7f98, UNSURE name
extern uint32_t network_join_message_body[]; // 0x006b7f9a, UNSURE name/size
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this module
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits
extern void widget_close_all(void); // 0x498650
extern void game_engine_init_tick_record_for_mode(void); // 0x470ae0, not in this batch
extern void game_engine_reset_all_players(void); // 0x45b8b0, not in this batch
extern network_server_globals *network_server; // 0x0071c2d4
extern void network_host_full_state_broadcast(network_server_globals *server); // 0x4df510, not in this batch
extern int32_t join_ui_state; // 0x00718f8c
extern int32_t time_query_performance_counter_ms(void); // 0x449210, cseries: current time in milliseconds
extern int32_t some_timestamp_0068e684; // 0x0068e684, UNSURE name
extern int32_t some_deadline_0068e680;  // 0x0068e680, UNSURE name

// blam-cc: stack -> connection
int32_t network_connection_finalize_join(uint16_t *connection)
{
    uint16_t *puVar7, *puVar1;
    uint32_t *puVar2;
    uint32_t uVar3, uVar8;
    char ok;
    int32_t iVar6, iVar12;
    int16_t sVar9;
    large_integer counter;
    int32_t now_ms;
    uint8_t encode_buffer[1540];
    uint32_t capacity;
    int32_t i;

    if (debug_log_level > 2 && network_statistics_logging_enabled != 0 &&
        network_summary_log_file != 0) {
        _fprintf(network_summary_log_file, "%s\t", network_build_string);
    }

    iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc); // channel
    connection[0x76c] = 0xffff; // client->unknown_ed8 = 0xffff

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    *(int32_t *)(iVar6 + 4) = now_ms; // channel->last_activity_ms = now_ms

    if (network_game_mode == 2) {
        *((uint8_t *)connection + 0xec0) = 1; // session.unknown_3ac = 1
    } else {
        ok = network_game_scenario_load_request((network_game_session *)((uint8_t *)connection + 0xb14));
        if (ok != 1) {
            goto tail;
        }
    }

    iVar6 = 0;
    puVar7 = connection + 0x669;
    do {
        if ((int32_t)(int8_t)*puVar7 == (uint32_t)*connection) {
            puVar7 = connection + iVar6 * 0x10;
            iVar6 = (int32_t)(uint32_t)player_data;
            if ((int32_t)(int8_t)puVar7[0x669] == (uint32_t)*connection) {
                goto have_machine;
            }
            break;
        }
        iVar6 = iVar6 + 1;
        puVar7 = puVar7 + 0x10;
    } while (iVar6 < 0x10);
    goto after_search;

    while (1) {
        uVar8 = (uint32_t)player_data_iterator_advance((int8_t)*((uint8_t *)puVar7 + 0xcd5));
        sVar9 = (int16_t)(int8_t)*((uint8_t *)puVar7 + 0xcd3);
        if (-1 < (int8_t)*((uint8_t *)puVar7 + 0xcd3) && sVar9 < 1) {
            puVar2 = (uint32_t *)(network_local_player_index_table + 4 + sVar9 * 4);
            uVar3 = *puVar2;
            if (uVar3 != 0xffffffff) {
                *(uint16_t *)((uVar3 & 0xffff) * 0x200 + 2 + *(int32_t *)((uint8_t *)player_data + 0x34)) = 0xffff;
                iVar6 = (int32_t)(uint32_t)player_data;
            }
            *puVar2 = uVar8;
            if (uVar8 != 0xffffffff) {
                *(int16_t *)((uVar8 & 0xffff) * 0x200 + 2 + *(int32_t *)((uint8_t *)player_data + 0x34)) = sVar9;
            }
        }
        puVar1 = puVar7 + 0x679;
        puVar7 = puVar7 + 0x10;
        if ((int32_t)(int8_t)*puVar1 != (uint32_t)*connection) {
            break;
        }
have_machine:
        ok = network_player_entry_validate(); // UNSURE argument (none visible)
        if (ok == 0) {
            break;
        }
    }

after_search:
    iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc); // channel
    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    *(int32_t *)(iVar6 + 4) = now_ms;

    capacity = 0x600;
    ok = (char)data_packet_group_encode_packet(encode_buffer, &capacity, 0x1a, 1);
    if (ok != 0) {
        uint32_t *src, *dst8;
        uint8_t *src_b, *dst_b;

        network_join_message_header = (((int16_t)capacity + 2) * 0x10) | 0xc;
        src = (uint32_t *)encode_buffer;
        dst8 = network_join_message_body;
        for (i = (capacity & 0xffff) >> 2; i != 0; i = i - 1) {
            *dst8 = *src;
            src = src + 1;
            dst8 = dst8 + 1;
        }
        src_b = (uint8_t *)src;
        dst_b = (uint8_t *)dst8;
        for (i = capacity & 3; i != 0; i = i - 1) {
            *dst_b = *src_b;
            src_b = src_b + 1;
            dst_b = dst_b + 1;
        }

        iVar6 = *(int32_t *)((uint8_t *)connection + 0xadc); // channel
        iVar12 = (uint32_t)(network_join_message_header >> 4) * 8;
        if ((*(uint8_t *)(iVar6 + 0xa8c) & 1) == 0) {
            if ((((*(int32_t *)(iVar6 + 0x24) + *(int32_t *)(iVar6 + 0x1c) * -8) -
                  *(int32_t *)(iVar6 + 0x20)) + 1 < iVar12 + 1) &&
                (ok = network_channel_stream_flush((network_channel_stream *)(iVar6 + 0x10), (network_channel *)iVar6, 1), ok == 0)) {
                goto tail;
            }
            {

                *(int32_t *)(iVar6 + 0xa80) = *(int32_t *)(iVar6 + 0xa80) + iVar12 + 1;
                { uint32_t item_flag = 0; bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)iVar6 + 0x10), &item_flag, 1); }
                *(uint8_t *)(iVar6 + 0x2c) = 0;
                bit_stream_write_bits_chunked((bit_stream *)((uint8_t *)iVar6 + 0x10), (const uint32_t *)(&network_join_message_header), iVar12);
                *(uint8_t *)(iVar6 + 0x2c) = 0;
            }
        }

        connection[0x76d] = 3; // client->state = 3
        connection[0x766] = 0;
        connection[0x767] = 0;
        connection[0x768] = 0;
        connection[0x769] = 0;
        *((uint8_t *)connection + 0xee1) = 0; // unknown_ee1 = 0
        widget_close_all();
        game_engine_init_tick_record_for_mode();
        game_engine_reset_all_players();
        if (network_game_mode == 2 && ((*(uint8_t *)((uint8_t *)network_server + 6) >> 2 & 1) == 0)) {
            network_host_full_state_broadcast(network_server);
        }
        if (join_ui_state != 0) {
            int32_t now2 = time_query_performance_counter_ms();
            uint32_t delay = 0;

            if (some_timestamp_0068e684 != -1 &&
                (uint32_t)(now2 - some_timestamp_0068e684) < 2000 && join_ui_state != 1) {
                delay = (uint32_t)(some_timestamp_0068e684 - now2) + 2000;
                if (delay > 2000) {
                    delay = 2000;
                }
            }
            some_deadline_0068e680 = delay + 0x6d6 + now2;
        }
    }
tail:
    return connection[0x76d] == 3;
}

#if 0
Original Ghidra decompilation (0x4d9960):

int __cdecl network_connection_finalize_join(ushort *connection)

{
  ushort *puVar1;
  uint *puVar2;
  uint uVar3;
  char cVar4;
  undefined4 uVar5;
  int iVar6;
  ushort *puVar7;
  uint uVar8;
  short sVar9;
  undefined4 *puVar10;
  undefined4 *puVar11;
  int iVar12;
  undefined8 uVar13;
  LARGE_INTEGER local_610;
  uint local_604;
  undefined4 local_600 [384];

  if (((2 < DAT_0087ac06) && (DAT_006f14b4 != '\0')) && (DAT_006a6140 != (FILE *)0x0)) {
    _fprintf(DAT_006a6140,"%s\t",&DAT_00719879);
  }
  iVar6 = *(int *)(connection + 0x56e);
  connection[0x76c] = 0xffff;
  QueryPerformanceCounter(&local_610);
  uVar13 = __allmul(local_610.s.LowPart,local_610.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar6 + 4) = uVar5;
  if (DAT_00719720 == 2) {
    *(undefined1 *)(connection + 0x760) = 1;
  }
  else {
    cVar4 = network_game_scenario_load_request((int)(connection + 0x58a));
    if (cVar4 != '\x01') goto LAB_004d9cc4;
  }
  iVar6 = 0;
  puVar7 = connection + 0x669;
  do {
    if ((int)(char)*puVar7 == (uint)*connection) {
      puVar7 = connection + iVar6 * 0x10;
      iVar6 = DAT_0087a480;
      if ((int)(char)puVar7[0x669] == (uint)*connection) goto LAB_004d9a50;
      break;
    }
    iVar6 = iVar6 + 1;
    puVar7 = puVar7 + 0x10;
  } while (iVar6 < 0x10);
  goto LAB_004d9ae0;
  while( true ) {
    uVar8 = FUN_004d98f0((int)*(char *)((int)puVar7 + 0xcd5));
    sVar9 = (short)*(char *)((int)puVar7 + 0xcd3);
    if ((-1 < *(char *)((int)puVar7 + 0xcd3)) && (sVar9 < 1)) {
      puVar2 = (uint *)(DAT_0087a478 + 4 + sVar9 * 4);
      uVar3 = *puVar2;
      if (uVar3 != 0xffffffff) {
        *(undefined2 *)((uVar3 & 0xffff) * 0x200 + 2 + *(int *)(iVar6 + 0x34)) = 0xffff;
        iVar6 = DAT_0087a480;
      }
      *puVar2 = uVar8;
      if (uVar8 != 0xffffffff) {
        *(short *)((uVar8 & 0xffff) * 0x200 + 2 + *(int *)(iVar6 + 0x34)) = sVar9;
      }
    }
    puVar1 = puVar7 + 0x679;
    puVar7 = puVar7 + 0x10;
    if ((int)(char)*puVar1 != (uint)*connection) break;
LAB_004d9a50:
    cVar4 = FUN_004de9f0();
    if (cVar4 == '\0') break;
  }
LAB_004d9ae0:
  iVar6 = *(int *)(connection + 0x56e);
  QueryPerformanceCounter(&local_610);
  uVar13 = __allmul(local_610.s.LowPart,local_610.s.HighPart,1000,0);
  uVar5 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
  *(undefined4 *)(iVar6 + 4) = uVar5;
  local_610.s.LowPart = 0;
  local_604 = 0x600;
  cVar4 = data_packet_group_encode_packet(&local_610,&local_604,0x1a,1);
  if (cVar4 != '\0') {
    DAT_006b7f98 = ((short)local_604 + 2) * 0x10 | 0xc;
    puVar10 = local_600;
    puVar11 = &DAT_006b7f9a;
    for (uVar8 = (local_604 & 0xffff) >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
      *puVar11 = *puVar10;
      puVar10 = puVar10 + 1;
      puVar11 = puVar11 + 1;
    }
    for (uVar8 = local_604 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *(undefined1 *)puVar11 = *(undefined1 *)puVar10;
      puVar10 = (undefined4 *)((int)puVar10 + 1);
      puVar11 = (undefined4 *)((int)puVar11 + 1);
    }
    iVar6 = *(int *)(connection + 0x56e);
    iVar12 = (uint)(DAT_006b7f98 >> 4) * 8;
    if ((*(byte *)(iVar6 + 0xa8c) & 1) == 0) {
      if ((((*(int *)(iVar6 + 0x24) + *(int *)(iVar6 + 0x1c) * -8) - *(int *)(iVar6 + 0x20)) + 1 <
           iVar12 + 1) && (cVar4 = FUN_004ddb60(iVar6,1), cVar4 == '\0')) goto LAB_004d9cc4;
      *(int *)(iVar6 + 0xa80) = *(int *)(iVar6 + 0xa80) + iVar12 + 1;
      bit_stream_write_bits_chunked(1);
      *(undefined1 *)(iVar6 + 0x2c) = 0;
      bit_stream_write_bits_chunked(iVar12);
      *(undefined1 *)(iVar6 + 0x2c) = 0;
    }
    connection[0x76d] = 3;
    connection[0x766] = 0;
    connection[0x767] = 0;
    connection[0x768] = 0;
    connection[0x769] = 0;
    *(undefined1 *)((int)connection + 0xee1) = 0;
    widget_close_all();
    FUN_00470ae0();
    FUN_0045b8b0();
    if ((DAT_00719720 == 2) && ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
      FUN_004df510(DAT_0071c2d4);
    }
    if (DAT_00718f8c != 0) {
      iVar6 = FUN_00449210();
      uVar8 = 0;
      if ((((DAT_0068e684 != -1) && ((uint)(iVar6 - DAT_0068e684) < 2000)) && (DAT_00718f8c != 1))
         && (uVar8 = (DAT_0068e684 - iVar6) + 2000, 2000 < uVar8)) {
        uVar8 = 2000;
      }
      DAT_0068e680 = uVar8 + 0x6d6 + iVar6;
    }
  }
LAB_004d9cc4:
  return (uint)(connection[0x76d] == 3);
}
#endif
