// network_game_scenario_load_request  (Ghidra: network_game_scenario_load_request, already
// named)
// address 0x4de6d0, size 405 bytes
// name confidence: 0.55   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Prepares and issues a scenario_load() request
// for the network game using the requested map name and seed, then, when hosting, opens a
// channel for every connected machine and returns whether the map is now loaded." session at
// param_1: server_name (+0x84), unknown_19e (+0x19e) and unknown_3ac (+0x3ac, the map-loaded
// flag already established in network_host_shutdown_or_defer.c) all match
// types/networking.h's network_game_session.
// FIXED in the review pass: the first draft of this file declared the staged record with
// map_name at offset 0 and a 132-byte prefix afterwards, which put every field at the wrong
// place. Ghidra's own locals (local_110 dword, local_10c word, local_10a word, local_108 dword,
// local_104[260]) and the two 0x43-dword (0x10c byte) loops that zero and copy the record pin
// the real layout: seed at +0x06, salt at +0x08, map_name at +0x0c, total size 0x10c. The
// struct now lives in types/networking.h as network_scenario_load_request.
// +0x3a4 (the session-relative "salt", also touched by network_game_server_host_create.c) is kept
// as a raw offset; +0x134 is session->variant + 0x30 (the variant's engine index).
// DAT_006b0b80 (main_game_globals) is the scenario_load staging area (request copied to +8).
// register convention: fully recovered cdecl (session is the only parameter).
// reconciled: R33 game_time_globals.unknown_00 -> initialized (uint8 at +0x00, same byte)
// reconciled: R13 network_scenario_load_request.seed (+0x06) -> difficulty (campaign difficulty, lands at game globals +0x0e)

// VERIFIED against disassembly 0x4de6d0..0x4de865 (2026-09-30): FIXED: cache_file_switch_map_by_path(EAX = request.map_name, EBX = 1) x2, game_engine_apply_variant(EDX = &session->variant) and scenario_load(EAX = request.map_name) were called without their register arguments (scenario_load was handed main_game_globals); order of calls, the mode 1/2/3 salt selection, player loop and host tick-record tail compared. The null guard on the shared session is an addition (the original dereferences it unchecked)
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern int16_t network_game_mode; // 0x00719720
extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t *main_game_globals;  // 0x006b0b80, UNSURE identity/type

extern void cache_file_switch_map_by_path(char *path, uint8_t apply_state); // 0x45aea0; blam-cc: EAX -> path, EBX -> apply_state (1 here)
extern void game_unload_map(void); // outside this batch
extern void game_start_new_map(void); // outside this batch
extern void game_stop_current_map(void); // outside this batch
extern void game_engine_reset_all_players(void); // outside this batch
extern void game_engine_apply_variant(const game_variant *variant); // 0x45b990; blam-cc: EDX -> variant
extern void game_engine_init_tick_record_for_mode(void); // outside this batch
extern void main_menu_music_stop(void); // outside this batch
extern int32_t network_channel_key_open(network_player_entry *entry); // 0x4de870, this batch
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, this batch
extern char scenario_load(char *path); // 0x53e6a0; blam-cc: EAX -> path


char network_game_scenario_load_request(network_game_session *session)
{
    network_scenario_load_request request;
    int32_t i;
    char loaded;
    network_game_session *shared_session;

    memset(&request, 0, sizeof(request));
    request.difficulty = 1;
    request.salt = 0xdeadbeef;
    strncpy(request.map_name, session->server_name, 0x7f);
    request.difficulty = session->difficulty;

    if (network_game_mode > 0) {
        if (network_game_mode < 3) {
            if (network_server != 0) {
                shared_session = &network_server->session;
            } else if (network_client != 0) {
                shared_session = &network_client->session;
            } else {
                shared_session = 0;
            }
            if (shared_session != 0) {
                request.salt = *(uint32_t *)((uint8_t *)shared_session + 0x3a4); // UNSURE
            }
        } else if (network_game_mode == 3) {
            request.salt = *(uint32_t *)((uint8_t *)session + 0x3a4); // UNSURE
        }
    }
    cache_file_switch_map_by_path(request.map_name, 1);
    if (game_time->initialized != 0 && (game_time->active != 0 || game_time->paused != 0)) {
        game_stop_current_map();
        game_unload_map();
    }
    main_menu_music_stop();
    if (*(int32_t *)((uint8_t *)session + 0x134) != 0) {
        game_engine_apply_variant(&session->variant);
    }
    cache_file_switch_map_by_path(request.map_name, 1);
    memcpy(main_game_globals + 8, &request, sizeof(request));
    loaded = scenario_load(request.map_name);
    if (loaded == 0) {
        if (*main_game_globals == 0) {
            return session->map_loaded;
        }
    } else {
        *main_game_globals = 1;
    }
    session->map_loaded = 1;
    game_start_new_map();
    if (network_game_mode == 2) {
        for (i = 0; i < 0x10; i++) {
            if (network_player_entry_validate(&session->players[i]) == 0) {
                break;
            }
            if (network_channel_key_open(&session->players[i]) == 0) {
                session->map_loaded = 0;
                break;
            }
        }
        if (((*(uint8_t *)((uint8_t *)network_server + 6) >> 2) & 1) != 0) {
            game_engine_init_tick_record_for_mode();
            game_engine_reset_all_players();
        }
    }
    return session->map_loaded;
}

#if 0
Original Ghidra decompilation (0x4de6d0):

char __cdecl network_game_scenario_load_request(int param_1)

{
  char cVar1;
  uint uVar2;
  int iVar3;
  undefined4 *puVar4;
  char *pcVar5;
  undefined4 local_110;
  undefined2 local_10c;
  undefined2 local_10a;
  undefined4 local_108;
  char local_104 [260];

  puVar4 = &local_110;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *puVar4 = 0;
    puVar4 = puVar4 + 1;
  }
  local_10c = 0;
  local_10a = 1;
  local_108 = 0xdeadbeef;
  _strncpy(local_104,(char *)(param_1 + 0x84),0x7f);
  local_10a = *(undefined2 *)(param_1 + 0x19e);
  if (0 < DAT_00719720) {
    if (DAT_00719720 < 3) {
      if (DAT_0071c2d4 == 0) {
        if (DAT_0071c2d8 == 0) {
          iVar3 = 0;
        }
        else {
          iVar3 = DAT_0071c2d8 + 0xb14;
        }
      }
      else {
        iVar3 = DAT_0071c2d4 + 8;
      }
      local_108 = *(undefined4 *)(iVar3 + 0x3a4);
    }
    else if (DAT_00719720 == 3) {
      local_108 = *(undefined4 *)(param_1 + 0x3a4);
    }
  }
  FUN_0045aea0();
  if ((*DAT_006f1d6c != '\0') && ((DAT_006f1d6c[1] != '\0' || (DAT_006f1d6c[2] != '\0')))) {
    FUN_0045b370();
    FUN_0045afb0();
  }
  main_menu_music_stop();
  if (*(int *)(param_1 + 0x134) != 0) {
    FUN_0045b990();
  }
  FUN_0045aea0();
  puVar4 = &local_110;
  pcVar5 = DAT_006b0b80 + 8;
  for (iVar3 = 0x43; iVar3 != 0; iVar3 = iVar3 + -1) {
    *(undefined4 *)pcVar5 = *puVar4;
    puVar4 = puVar4 + 1;
    pcVar5 = pcVar5 + 4;
  }
  uVar2 = scenario_load();
  if ((char)uVar2 == '\0') {
    if (*DAT_006b0b80 == '\0') goto LAB_004de854;
  }
  else {
    *DAT_006b0b80 = '\x01';
  }
  *(undefined1 *)(param_1 + 0x3ac) = 1;
  FUN_0045b050();
  if (DAT_00719720 == 2) {
    iVar3 = 0;
    do {
      cVar1 = FUN_004de9f0();
      if (cVar1 == '\0') break;
      cVar1 = FUN_004de870();
      if (cVar1 == '\0') {
        *(undefined1 *)(param_1 + 0x3ac) = 0;
        break;
      }
      iVar3 = iVar3 + 1;
    } while (iVar3 < 0x10);
    if ((*(byte *)(DAT_0071c2d4 + 6) >> 2 & 1) != 0) {
      FUN_00470ae0();
      FUN_0045b8b0();
    }
  }
LAB_004de854:
  return *(char *)(param_1 + 0x3ac);
}
#endif
