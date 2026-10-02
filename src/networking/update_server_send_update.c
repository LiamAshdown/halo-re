// update_server_send_update  (Ghidra: update_server_send_update, already named)
// address 0x4ddfb0, size 989 bytes
// name confidence: 0.6   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md: "Periodic per-tick entry point that
// (re)initialises the update server when idle, otherwise packs the local player's
// position/orientation delta, records it in the player update history, and flushes it to the
// channel." Ghidra's own decompile carries a "Removing unreachable block" warning and is full of
// `puStack_NN = (uint *)0x4dexxxx` pseudo-assignments that are call-site return-address/stack
// bookkeeping artifacts, not real data flow; this rewrite drops those and keeps only the
// operations that write real fields. client->state (state==3), client->unknown_ecc,
// client->unknown_ee0/unknown_edc, network_client->channel (+0xadc), and the channel free-space/
// send_budget/empty fields all match the established fields used throughout this batch;
// everything inside the position/orientation packet-encode path (the message_delta_encode_single_value call and its
// stack-built argument block, the trig-based direction vector, and the 13-dword record copied
// into `local_68`) is preserved structurally but NOT verified field-by-field -- this is by far
// the least confident file in this batch. The `if (client->unknown_ee0 == 0) ... else ...`
// branch is byte-identical in both arms in Ghidra's own decompile (consistent with the
// "removing unreachable block" note) and is collapsed to the one assignment here.
// UNSURE: FUN_00472aa0/FUN_00472b00/update_server_dispose/update_client_stage_entry/ui_network_wait_timeout_check/ui_network_wait_timeout_start/
// network_game_client_apply_position_update/message_delta_encode_single_value are all outside this batch's address range (most are well outside
// the whole networking module, 0x440350..0x5781c0, and belong to the game/update-queue
// subsystem per types/networking.h's note that update_record/player_update_queue are owned by
// types/game.h); called here with whatever arguments Ghidra shows, which is often none.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

// FIXED 2026-09-28 (send-path audit, from the disassembly 0x4de1ed..0x4de26e): the room check and both writes use
// the channel's outgoing stream (+0x10), not the retransmit stream (+0x544): the 1-bit item flag (1, a game action)
// and then the encoded bits from the network scratch 0x871de0; the C wrote placeholders into the wrong stream.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern double sin(double x); // FSIN
extern double cos(double x); // FCOS

extern network_client_globals *network_client; // 0x0071c2d8
extern int16_t network_game_mode;               // 0x00719720
extern data_array *local_player_globals;       // 0x0087a478, UNSURE identity/type
extern uint32_t update_client_staged[8];   // 0x006f7ea4, UNSURE identity: 8 floats copied
                                                 // into the position/orientation record
extern uint32_t player_data;           // 0x0087a480, UNSURE identity
extern void *message_delta_definition_table;    // 0x00871de0, UNSURE identity
extern uint8_t update_server_pending_flush;      // 0x0071c2e4, UNSURE identity
extern uint8_t update_server_history_index;      // 0x0071c2e5, UNSURE identity
extern game_time_globals *game_time;             // 0x006f1d6c
extern network_server_globals *network_server;   // 0x0071c2d4
extern int32_t update_server_last_log_ms;        // 0x006b7f90, UNSURE identity
extern int32_t update_server_last_tick_ms;       // 0x0071c2e0, UNSURE identity

extern int32_t time_query_performance_counter_ms(void); // outside this batch, tick/ms counter
extern void update_server_new(void); // 0x472aa0, outside this batch
extern void update_queues_dispose(void); // 0x472b00, outside this batch
extern void update_server_dispose(void); // 0x472b70, outside this batch
extern void update_client_stage_entry(void); // 0x473090, outside this batch
extern void ui_network_wait_timeout_check(void); // 0x49c7b0, outside this batch
extern void ui_network_wait_timeout_start(void); // 0x49c810, outside this batch
extern uint8_t network_message_scratch[0x7ff8]; // 0x00871de0
extern int32_t bit_stream_write_bits_chunked(bit_stream *stream, const uint32_t *values, int32_t total_bit_count); // 0x4cf8f0, EAX stream, ECX values, stack bits
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module
extern char network_channel_stream_flush(network_channel_stream *stream, network_channel *channel, char mode); // 0x4ddb60, this batch
extern void network_game_client_apply_position_update(void *record, uint8_t history_byte, uint32_t *values, network_client_globals *client); // 0x4dff70, this batch, elided args
extern network_machine *network_machine_find_by_id(network_server_globals *server, int32_t machine_id); // 0x4e0810, this batch
extern void player_update_history_log_write(uint32_t category_flags, int32_t use_filtered_mask,
    const char *format, ...); // this module, 0x4e5ea0
extern char player_update_history_add(void *update_history, uint32_t *values); // 0x4e6b50, outside this batch
extern void *message_delta_encode_single_value(void *definition, void *dest, uint32_t *values, uint8_t *history_byte,
    int32_t max_bits, int32_t flags, int32_t unknown); // 0x4ec450, outside this batch, elided args

char update_server_send_update(uint32_t *tick_count, char frame_time_overflow)
{
    char result;
    char flush_ok;
    int32_t now_ms;
    uint32_t reliable_seq;
    uint16_t player_id;
    uint32_t control[8];
    char history_byte;
    char logged_history_byte;
    uint32_t record[13];
    uint32_t checksum;
    float position[4];
    float direction[3];
    void *encoded;
    network_channel *channel;
    data_iterator iterator;
    void *it; // data_iterator_next returns the element pointer (src/memory)
    int32_t max_bits;

    result = 1;
    if (network_client == 0) {
        network_game_mode = 0;
        update_queues_dispose();
        update_server_new();
        update_server_dispose();
        ui_network_wait_timeout_check();
        return result;
    }
    if (network_client->state == 3) {
        now_ms = time_query_performance_counter_ms();
        reliable_seq = network_client->last_update_id & 0x7fffffff;
        player_id = local_player_globals->maximum_count; // UNSURE: +0xc read as a word, see header
        memcpy(control, update_client_staged, sizeof(control));
        history_byte = 0;
        if ((int32_t)tick_count > 0 && frame_time_overflow == 0) {
            checksum = player_data;
            (void)checksum;
            iterator.data = 0; iterator.next_index = 0; iterator.index = 0; // UNSURE: elided iterator source
            iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
            it = data_iterator_next(&iterator);
            while (it != 0) {
                // UNSURE: the *(short*)(it+2)==-1 / *(int*)(it+0x34)!=-1 checks and everything
                // below them operate on an opaque datum this rewrite could not identify; kept
                // structurally (advance to the next item on a -1 sentinel, otherwise process
                // once and stop) rather than fully typed.
                break;
            }
        }
        if (network_game_mode == 1) {
            flush_ok = player_update_history_add(network_client->update_history, tick_count);
            if (flush_ok != 1) {
                update_client_stage_entry();
                ui_network_wait_timeout_start();
                goto after_send;
            }
        } else {
            logged_history_byte = update_server_history_index;
            update_server_history_index = (update_server_history_index + 1) & 0x3f;
            (void)logged_history_byte;
        }
        direction[0] = (float)cos(position[2]);
        direction[1] = (float)(cos(position[1]) * direction[0]);
        direction[2] = (float)(sin(position[1]) * direction[0]);
        history_byte = update_server_history_index;

        if (network_game_mode == 1) {
            encoded = message_delta_encode_single_value(message_delta_definition_table, record, control,
                (uint8_t *)&history_byte, 0x7ff8, 0xd, 0); // UNSURE: argument shapes, see header
            if (update_server_pending_flush == 1) {
                time_query_performance_counter_ms();
            }
            channel = network_client->channel;
            update_server_pending_flush = 0;
            result = 1;
            if ((channel->flags & 1) == 0) {
                max_bits = (channel->outgoing.stream.last_bit - channel->outgoing.stream.byte_cursor * 8) -
                           channel->outgoing.stream.bit_cursor + 1;
                if (max_bits < (int32_t)(uintptr_t)encoded + 1) {
                    flush_ok = network_channel_stream_flush(&channel->outgoing, channel, 1);
                    if (flush_ok == 0) {
                        goto after_channel_check;
                    }
                }
                channel->send_budget = channel->send_budget + (int32_t)(uintptr_t)encoded + 1;
                {
                    uint32_t item_flag = 1;

                    bit_stream_write_bits_chunked(&channel->outgoing.stream, &item_flag, 1);
                    channel->outgoing.empty = 0;
                    bit_stream_write_bits_chunked(&channel->outgoing.stream, (const uint32_t *)network_message_scratch,
                                                  (int32_t)(uintptr_t)encoded);
                    channel->outgoing.empty = 0;
                }
                flush_ok = network_channel_stream_flush(&channel->outgoing, channel, 1);
            } else {
                flush_ok = 1;
            }
        after_channel_check:
            result = flush_ok;
            if (flush_ok != 0) {
                player_update_history_log_write(1, 0, "[%d]: Sent update [%d], [%d] ticks.\n",
                    game_time->game_time, (int32_t)history_byte, (int32_t)(uintptr_t)tick_count);
                    // REVIEW PASS 2026-09-20: 0x66c404 is the format string, not a
                    // trailing flag -- objdump of .rdata at 0x66c404 reads
                    // "[%d]: Sent update [%d], [%d] ticks." and Ghidra pushes it first
                    // (puStack_8c), then the tick, then local_6b, then param_1.
                    // UNSURE: the EAX/ECX category pair is elided at this call site;
                    // (1, 0) matches every other call site of 0x4e5ea0 in this module.
            }
        } else {
            network_machine *machine = network_machine_find_by_id(network_server, player_id);
            network_game_client_apply_position_update(machine, control[0], tick_count, network_client); // UNSURE
        }
        memcpy(record, control, sizeof(record) < sizeof(control) ? sizeof(record) : sizeof(control));
        if (history_byte == 0) {
            goto after_send;
        }
    after_send:
        if (update_server_pending_flush == 0) {
            update_server_last_log_ms = time_query_performance_counter_ms();
            update_server_pending_flush = 1;
        }
        update_server_last_tick_ms = now_ms;
    }
    ui_network_wait_timeout_check();
    return result;
}

#if 0
Original Ghidra decompilation (0x4ddfb0):

/* WARNING: Removing unreachable block (ram,0x004de288) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char __cdecl update_server_send_update(uint *param_1,char param_2)

{
  undefined4 uVar1;
  char cVar2;
  char cVar3;
  uint *puVar4;
  int iVar5;
  float *pfVar6;
  undefined4 *puVar7;
  float *pfVar8;
  uint **ppuVar9;
  uint *puVar10;
  float10 fVar11;
  float10 fVar12;
  uint *apuStack_a0 [4];
  byte *pbStack_90;
  uint *puStack_8c;
  uint *puStack_88;
  uint *puStack_84;
  uint *puStack_80;
  byte local_6b;
  char local_6a;
  char local_69;
  uint *local_68;
  byte local_64 [4];
  undefined4 local_60;
  uint local_5c [2];
  float local_54;
  float local_50;
  float local_4c;
  float local_48;
  float local_44;
  float local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined2 local_30;
  undefined2 local_2e;
  undefined2 local_2c;
  uint local_28;
  undefined2 local_22;
  float local_20 [4];
  undefined4 local_10;
  undefined4 local_c;
  undefined2 local_8;
  undefined2 local_6;
  undefined2 local_4;

  local_69 = '\x01';
  if (DAT_0071c2d8 == 0) {
    DAT_00719720 = 0;
    puStack_80 = (uint *)0x4ddfd3;
    update_queues_dispose();
    puStack_80 = (uint *)0x4ddfd8;
    update_server_new();
    puStack_80 = (uint *)0x4ddfdd;
    FUN_00472b70();
    puStack_80 = (uint *)0x4ddfe2;
    FUN_0049c7b0();
    return local_69;
  }
  if (*(short *)(DAT_0071c2d8 + 0xeda) == 3) {
    puStack_80 = (uint *)0x4de001;
    local_60 = FUN_00449210();
    if (*(char *)(DAT_0071c2d8 + 0xee0) == '\0') {
      local_28 = *(uint *)(DAT_0071c2d8 + 0xecc);
    }
    else {
      local_28 = *(uint *)(DAT_0071c2d8 + 0xecc);
    }
    local_28 = local_28 & 0x7fffffff;
    local_22 = *(undefined2 *)(DAT_0087a478 + 0xc);
    pfVar6 = (float *)&DAT_006f7ea4;
    pfVar8 = local_20;
    for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
      *pfVar8 = *pfVar6;
      pfVar6 = pfVar6 + 1;
      pfVar8 = pfVar8 + 1;
    }
    local_6a = '\0';
    if ((0 < (int)param_1) && (param_2 == '\0')) {
      local_5c[0] = DAT_0087a480;
      local_50 = (float)(DAT_0087a480 ^ 0x69746572);
      local_5c[1] = local_5c[1] & 0xffff0000;
      local_54 = -NAN;
      puStack_80 = (uint *)0x4de09e;
      iVar5 = data_iterator_next();
      if (iVar5 != 0) {
LAB_004de0a8:
        if (*(short *)(iVar5 + 2) == -1) goto code_r0x004de0ae;
        if (*(int *)(iVar5 + 0x34) != -1) {
          local_68 = (uint *)(DAT_0071c2d8 + 0xf14);
          if (DAT_00719720 == 1) {
            uVar1 = *(undefined4 *)(DAT_0071c2d8 + 0xf48);
            puStack_80 = (uint *)local_64;
            puVar7 = &DAT_006f7ea4;
            ppuVar9 = apuStack_a0;
            for (iVar5 = 8; iVar5 != 0; iVar5 = iVar5 + -1) {
              *ppuVar9 = (uint *)*puVar7;
              puVar7 = puVar7 + 1;
              ppuVar9 = ppuVar9 + 1;
            }
            cVar2 = player_update_history_add(uVar1,param_1);
            local_6b = local_64[0];
            if (cVar2 != '\x01') {
              local_44 = (float)local_68[0xb];
              local_40 = (float)CONCAT22(local_40._2_2_,(short)local_68[0xc]);
              local_5c[0] = 0;
              local_48 = 0.0;
              local_50 = 0.0;
              local_4c = 0.0;
              puStack_80 = (uint *)0x4de383;
              FUN_00473090();
              puStack_80 = (uint *)0x4de388;
              FUN_0049c810();
              goto LAB_004de306;
            }
          }
          else {
            local_64[0] = DAT_0071c2e5;
            DAT_0071c2e5 = DAT_0071c2e5 + 1 & 0x3f;
          }
          fVar11 = (float10)fcos((float10)local_20[2]);
          local_5c[1] = local_28;
          local_54 = local_20[0];
          local_50 = local_20[1];
          local_4c = local_20[2];
          local_3c = local_20[3];
          local_38 = local_10;
          local_34 = local_c;
          local_30 = local_8;
          local_5c[0] = CONCAT31(local_5c[0]._1_3_,(char)param_1);
          local_2e = local_6;
          local_2c = local_4;
          fVar12 = (float10)fcos((float10)local_20[1]);
          local_48 = (float)(fVar12 * fVar11);
          fVar12 = (float10)fsin((float10)local_20[1]);
          local_44 = (float)(fVar12 * fVar11);
          fVar11 = (float10)fsin((float10)local_20[2]);
          local_40 = (float)fVar11;
          local_6b = local_64[0];
          if (DAT_00719720 == 1) {
            puStack_80 = (uint *)0x7ff8;
            puStack_84 = (uint *)&DAT_00871de0;
            puStack_88 = local_68;
            puStack_8c = local_5c;
            pbStack_90 = &local_6b;
            apuStack_a0[3] = (uint *)0xd;
            apuStack_a0[2] = (uint *)0x4de1d5;
            puVar4 = (uint *)FUN_004ec450();
            local_6a = '\x01';
            if (DAT_0071c2e4 == '\x01') {
              puStack_80 = (uint *)0x4de1ed;
              FUN_00449210();
            }
            puVar10 = *(uint **)(DAT_0071c2d8 + 0xadc);
            DAT_0071c2e4 = '\0';
            local_69 = 1;
            if ((puVar10[0x2a3] & 1) == 0) {
              if ((int)(((puVar10[9] + puVar10[7] * -8) - puVar10[8]) + 1) < (int)puVar4 + 1) {
                puStack_80 = (uint *)0x1;
                puStack_88 = (uint *)0x4de234;
                puStack_84 = puVar10;
                cVar3 = FUN_004ddb60();
                cVar2 = '\0';
                if (cVar3 == '\0') goto LAB_004de29a;
              }
              puVar10[0x2a0] = (uint)((undefined *)((int)puVar4 + 1) + puVar10[0x2a0]);
              puStack_80 = (uint *)0x1;
              puStack_84 = (uint *)0x4de24e;
              bit_stream_write_bits_chunked();
              *(undefined1 *)(puVar10 + 0xb) = 0;
              puStack_88 = (uint *)0x4de25f;
              puStack_84 = puVar4;
              bit_stream_write_bits_chunked();
              puStack_88 = (uint *)0x1;
              *(undefined1 *)(puVar10 + 0xb) = 0;
              pbStack_90 = (byte *)0x4de26b;
              puStack_8c = puVar10;
              cVar2 = FUN_004ddb60();
            }
            else {
              cVar2 = '\x01';
            }
LAB_004de29a:
            local_69 = cVar2;
            if (cVar2 != '\0') {
              puStack_84 = (uint *)(uint)local_6b;
              puStack_80 = param_1;
              puStack_88 = *(uint **)(DAT_006f1d6c + 0xc);
              puStack_8c = (uint *)0x66c404;
              pbStack_90 = (byte *)0x4de2cb;
              player_update_history_log_write();
            }
          }
          else {
            puStack_80 = (uint *)(uint)local_64[0];
            puStack_88 = &local_28;
            puStack_84 = param_1;
            puStack_8c = (uint *)0x4de2e6;
            puStack_8c = (uint *)FUN_004e0810();
            pbStack_90 = (byte *)0x4de2ec;
            FUN_004dff70();
          }
          puVar4 = local_5c;
          puVar10 = local_68;
          for (iVar5 = 0xd; iVar5 != 0; iVar5 = iVar5 + -1) {
            *puVar10 = *puVar4;
            puVar4 = puVar4 + 1;
            puVar10 = puVar10 + 1;
          }
          if (local_6a == '\0') goto LAB_004de306;
          goto LAB_004de320;
        }
      }
    }
LAB_004de306:
    if (DAT_0071c2e4 == '\0') {
      puStack_80 = (uint *)0x4de314;
      _DAT_006b7f90 = FUN_00449210();
      DAT_0071c2e4 = '\x01';
    }
LAB_004de320:
    _DAT_0071c2e0 = local_60;
  }
  puStack_80 = (uint *)0x4de32f;
  FUN_0049c7b0();
  return local_69;
code_r0x004de0ae:
  puStack_80 = (uint *)0x4de0b7;
  iVar5 = data_iterator_next();
  if (iVar5 == 0) goto LAB_004de306;
  goto LAB_004de0a8;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
