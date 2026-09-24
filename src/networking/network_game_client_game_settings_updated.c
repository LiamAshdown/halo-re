// network_game_client_game_settings_updated  (Ghidra: network_game_client_game_settings_updated,
// already named)
// address 0x4df2e0, size 541 bytes
// name confidence: 0.6   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md: "Handles a game-settings update for a new
// network round: resets channel and history tables, applies the current game variant/defaults,
// loads the requested map (network_game_server_load_scenario), and kicks off either the host or client path
// depending on host->flags bit2." Confirmed field matches: host->flags bit2 (+6), the
// session.players[] reset loop at +0x1c7 (== session+0x1bf, byte-for-byte the same pattern as
// network_game_session_reset.c), the machines[] loop at +0x3c6 (== machines[0]+0xe, the flags
// byte, clearing bit2 and the unknown_04/unknown_08/unknown_50 fields byte-by-byte -- collapsed
// here to whole-field writes), and the +0x3b0 round counter shared with
// network_host_round_reset.c.
// UNSURE (significant): this is the largest and least-verified file in the batch. Most of the
// UI/global-state fields (DAT_00718f8c/94/98/a6, DAT_0087aa40/80/84, DAT_006953e8,
// DAT_00712542/44, DAT_0087a478/80) are foreign to types/networking.h and are declared
// generically; the byte-level struct-copy loops are preserved as raw memcpy/offset writes
// rather than typed field assignments where no field mapping could be confirmed.
// register convention: fully recovered cdecl (the host/session container is the only parameter).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern data_array *update_server_globals;  // 0x0087a478, UNSURE identity, see update_server_send_update.c
extern void *variant_defaults_source;      // 0x0087aa40, UNSURE identity: strncpy'd map-name-ish
extern uint8_t variant_defaults_block[0x98]; // 0x0087aa80, UNSURE identity: copied into session+0x10c-ish
extern network_client_globals *network_client; // 0x0071c2d8
extern int32_t unknown_00718f8c; // 0x00718f8c, UNSURE identity (join_ui_state per chimera__on_connect.c)
extern int32_t unknown_00718f94; // 0x00718f94, UNSURE identity: a widget handle
extern int32_t unknown_00718f98; // 0x00718f98, UNSURE identity
extern uint8_t unknown_00718fa6; // 0x00718fa6, UNSURE identity
extern int32_t unknown_006953e8; // 0x006953e8, UNSURE identity
extern uint8_t unknown_00712542; // 0x00712542, UNSURE identity
extern uint8_t unknown_00712544[0x280]; // 0x00712544, UNSURE identity/size

extern void message_delta_parameters_protocol_dump_to_config_file(void); // 0x4ec330, outside this batch
extern void network_stats_summary_log_write(void); // 0x440820, this module
extern void message_delta_protocol_initialize(void); // 0x4ec2f0, outside this batch
extern void network_stats_summary_log_open(void); // 0x440670, this module
extern datum_index datum_get(data_array *array, datum_index index); // 0x4d0680, memory module
extern void game_engine_apply_current_custom_variant(void); // outside this batch
extern void FUN_0045fc80(void); // outside this batch
extern void widget_close(int32_t widget); // outside this batch
extern void FUN_004994b0(void); // outside this batch
extern char network_game_server_load_scenario(void); // 0x4e0720, outside this batch, elided args
extern void network_host_full_state_broadcast(network_server_globals *host); // 0x4df510, this
    // batch; UNSURE: Ghidra types it void but the caller reads its result as if it returned a
    // value (the same leftover-register pattern documented elsewhere in this batch) -- treated
    // as always succeeding (1) here since the real return path could not be determined
extern void network_client_timer_schedule(int32_t a, int32_t b); // 0x4d9ed0, outside this batch

uint32_t network_game_client_game_settings_updated(network_server_globals *host)
{
    int32_t i;
    network_machine *machine;
    network_player_entry *player;
    datum_index item;
    uint32_t result;
    int is_host;

    // param_1[0x272..0x275] == host+0x9c8..0x9d4 -ish; zeroed unconditionally up front.
    memset((uint8_t *)host + 0x9c8, 0, 0x10);

    is_host = (host->flags >> 2) & 1;
    if (!is_host) {
        if (update_server_globals->maximum_count != (int16_t)-1) { // UNSURE: +4 read as dword
            item = datum_get(update_server_globals, 0);
            if (item != 0) {
                network_client->unknown_f10 = *(int32_t *)((uint8_t *)(uint32_t)item + 0x20);
            }
        }
    } else {
        message_delta_parameters_protocol_dump_to_config_file();
        network_stats_summary_log_write();
        message_delta_protocol_initialize();
        network_stats_summary_log_open();
    }

    *(int32_t *)((uint8_t *)host + 0x3b0) = *(int32_t *)((uint8_t *)host + 0x3b0) + 1; // UNSURE, see host_round_reset
    *(int32_t *)((uint8_t *)host + 0x9b8) = 0;
    *(int32_t *)((uint8_t *)host + 0x9c4) = 0;
    host->unknown_9f9 = 0;
    host->unknown_9fa = 0;
    *(uint8_t *)((uint8_t *)host + 0x9f8) = 0;

    for (i = 0; i < 16; i++) {
        machine = &host->machines[i];
        machine->flags = machine->flags & 0xfb;
        machine->unknown_04 = 0;
        machine->unknown_08 = 0;
        machine->unknown_50 = 0;
    }

    memset((uint8_t *)host + 0x88, 0, 0x21 * 4);  // param_1+0x22 dwords
    memset((uint8_t *)host + 0x10c, 0, 0x26 * 4); // param_1+0x43 dwords, game_variant region
    *(uint16_t *)((uint8_t *)host + 0x1a8) = 0;

    for (i = 0; i < 16; i++) {
        player = &host->session.players[i];
        player->machine_index = -1;
        player->machine_player_index = -1;
        player->unknown_1e = -1;
        player->slot_index = -1;
        player->name[0] = 0;
        player->color_index = -1;
        player->unknown_1a = -1;
    }
    host->unknown_9b8 = 0; // param_1[0xed]; overlaps machines[]-adjacent state, see header
    host->unknown_004 = 0;

    game_engine_apply_current_custom_variant();
    FUN_0045fc80();
    if (!is_host) {
        unknown_00718f8c = 2;
    }
    host->unknown_a0e = 1;
    memcpy((uint8_t *)host + 0x10c, variant_defaults_block, sizeof(variant_defaults_block));
    strncpy((char *)host + 0x8c, (char *)variant_defaults_source, 0x3f);
    *(uint8_t *)((uint8_t *)host + 0xcb) = 0;
    host->unknown_004 = host->unknown_004 | 1;
    *(uint16_t *)((uint8_t *)host + 0x86) = 0;
    *(int32_t *)((uint8_t *)host + 0x88) = 0;
    host->listen_channel->listening = 1;

    if (unknown_00718f94 != 0) {
        widget_close(unknown_00718f94);
    }
    if (unknown_00718f98 != 0) {
        FUN_004994b0();
    }
    unknown_00718fa6 = 0;
    if (unknown_006953e8 != -1) {
        unknown_00712542 = unknown_00712542 & 0xf7;
        memset(unknown_00712544, 0, sizeof(unknown_00712544));
        unknown_006953e8 = -1;
    }
    host->unknown_9bc[0x19] = 0; // 0x9d5

    result = network_game_server_load_scenario();
    if ((char)result == 1) {
        host->unknown_004 = 1;
        if (is_host) {
            network_host_full_state_broadcast(host);
            result = 1; // UNSURE, see extern declaration comment
            host->unknown_9fa = 0;
            return result & 0xffffff00;
        }
        network_client_timer_schedule(0, 0);
    }
    host->unknown_9fa = 0;
    return result & 0xffffff00;
}

#if 0
Original Ghidra decompilation (0x4df2e0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl network_game_client_game_settings_updated(int *param_1)

{
  byte *pbVar1;
  undefined1 *puVar2;
  uint uVar3;
  int iVar4;
  int *piVar5;
  int *piVar6;
  undefined4 *puVar7;

  param_1[0x272] = 0;
  param_1[0x273] = 0;
  param_1[0x274] = 0;
  param_1[0x275] = 0;
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) == 0) {
    if (*(int *)(DAT_0087a478 + 4) != -1) {
      iVar4 = datum_get();
      if (iVar4 != 0) {
        *(undefined4 *)(DAT_0071c2d8 + 0xf10) = *(undefined4 *)(iVar4 + 0x20);
      }
    }
  }
  else {
    message_delta_parameters_protocol_dump_to_config_file();
    network_stats_summary_log_write();
    message_delta_protocol_initialize();
    network_stats_summary_log_open();
  }
  param_1[0xec] = param_1[0xec] + 1;
  param_1[0x26e] = 0;
  param_1[0x271] = 0;
  *(undefined1 *)((int)param_1 + 0x9f9) = 0;
  *(undefined1 *)((int)param_1 + 0x9fa) = 0;
  *(undefined1 *)(param_1 + 0x27e) = 0;
  pbVar1 = (byte *)((int)param_1 + 0x3c6);
  iVar4 = 0x10;
  do {
    *pbVar1 = *pbVar1 & 0xfb;
    pbVar1[-0xffffffff0000000a] = 0;
    pbVar1[-0xffffffff00000009] = 0;
    pbVar1[-0xffffffff00000008] = 0;
    pbVar1[-0xffffffff00000007] = 0;
    pbVar1[-0xffffffff00000006] = 0;
    pbVar1[-0xffffffff00000005] = 0;
    pbVar1[-0xffffffff00000004] = 0;
    pbVar1[-0xffffffff00000003] = 0;
    pbVar1[0x42] = 0;
    pbVar1 = pbVar1 + 0x60;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  piVar5 = param_1 + 0x22;
  for (iVar4 = 0x21; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar5 = 0;
    piVar5 = piVar5 + 1;
  }
  piVar5 = param_1 + 0x43;
  for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar5 = 0;
    piVar5 = piVar5 + 1;
  }
  *(undefined2 *)(param_1 + 0x6a) = 0;
  puVar2 = (undefined1 *)((int)param_1 + 0x1c7);
  iVar4 = 0x10;
  do {
    puVar2[-1] = 0xff;
    *puVar2 = 0xff;
    puVar2[1] = 0xff;
    puVar2[2] = 0xff;
    *(undefined2 *)(puVar2 + -0x1d) = 0;
    *(undefined2 *)(puVar2 + -5) = 0xffff;
    *(undefined2 *)(puVar2 + -3) = 0xffff;
    puVar2 = puVar2 + 0x20;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  param_1[0xed] = 0;
  *(undefined2 *)(param_1 + 1) = 0;
  game_engine_apply_current_custom_variant();
  FUN_0045fc80();
  if ((*(byte *)((int)param_1 + 6) >> 2 & 1) == 0) {
    DAT_00718f8c = 2;
  }
  *(undefined1 *)((int)param_1 + 0xa0e) = 1;
  piVar5 = &DAT_0087aa80;
  piVar6 = param_1 + 0x43;
  for (iVar4 = 0x26; iVar4 != 0; iVar4 = iVar4 + -1) {
    *piVar6 = *piVar5;
    piVar5 = piVar5 + 1;
    piVar6 = piVar6 + 1;
  }
  _strncpy((char *)(param_1 + 0x23),&DAT_0087aa40,0x3f);
  *(undefined1 *)((int)param_1 + 0xcb) = 0;
  *(ushort *)((int)param_1 + 6) = *(ushort *)((int)param_1 + 6) | 1;
  *(undefined2 *)((int)param_1 + 0x86) = 0;
  param_1[0x22] = 0;
  *(undefined1 *)(*param_1 + 0xae0) = 1;
  if (DAT_00718f94 != 0) {
    widget_close(DAT_00718f94);
  }
  if (DAT_00718f98 != 0) {
    FUN_004994b0();
  }
  _DAT_00718fa6 = 0;
  if (DAT_006953e8 != -1) {
    DAT_00712542 = DAT_00712542 & 0xf7;
    puVar7 = &DAT_00712544;
    for (iVar4 = 0xa0; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar7 = 0;
      puVar7 = puVar7 + 1;
    }
    DAT_006953e8 = -1;
  }
  *(undefined1 *)((int)param_1 + 0x9d5) = 0;
  uVar3 = FUN_004e0720();
  if ((char)uVar3 == '\x01') {
    *(undefined2 *)(param_1 + 1) = 1;
    if ((*(byte *)((int)param_1 + 6) >> 2 & 1) != 0) {
      uVar3 = FUN_004df510(param_1);
      *(undefined1 *)((int)param_1 + 0x9fa) = 0;
      return uVar3 & 0xffffff00;
    }
    uVar3 = FUN_004d9ed0(0,0);
  }
  *(undefined1 *)((int)param_1 + 0x9fa) = 0;
  return uVar3 & 0xffffff00;
}
#endif
