// network_game_start_new_server_with_name_and_password  (Ghidra: already named)
// address 0x4e4150, size 504 bytes
// name confidence: 0.65   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md; types/networking.h network_server_globals
// (listen_channel/flags/password) and network_channel (listening at +0xae0). Disassembly
// (objdump -d -M intel) pins the wide-name destination precisely: `add eax,0x8` right before the
// first wcsncpy, with the forced NUL written at `[esi+0x86]` -- i.e. server+0x008..+0x086, which
// is the leading span of network_game_session (session+0x000..+0x07e) that types/networking.h
// currently records as message_callback/unknown_004/unknown_07e. This function writes a 63-wide-
// character name there directly, which the header's own account of that region (only ever
// zeroed by network_channel_table_initialize, not otherwise resolved) does not cover -- flagged
// UNSURE rather than reinterpreting the header's fields.
// register convention: __cdecl, three recognized parameters.
// UNSURE: unused is never read anywhere in this function's body; forwarded here only because
// network_game_start_new_server_from_profile.c (this batch) passes it through. UNSURE: the
// server+0x008..+0x086 wide-name write described above; also +0x9d5, which this function clears
// but which falls inside network_server_globals::unknown_9bc (no individual field name).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern uint8_t network_server_host_valid; // 0x0071c2dd
extern uint8_t network_game_info_packet_flag; // 0x006894a2, UNSURE name
extern uint8_t network_session_host_flags_byte; // 0x0069fe00, UNSURE name
extern uint8_t network_channels_open_ok; // 0x006869be
extern int32_t message_delta_vector3d_mode; // 0x0069b350, UNSURE name
extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested; // 0x0071c2de
extern int32_t interface_loading_screen_address_a; // 0x0068e680, UNSURE name/owner
extern int32_t interface_loading_screen_address_b; // 0x0068e684, UNSURE name/owner
extern int32_t join_ui_state; // 0x00718f8c, UNSURE name/owner
extern int32_t interface_loading_screen_progress; // 0x00718f90, UNSURE name/owner
extern int16_t progress_screen_text; // 0x006b2f28, UNSURE name/owner
extern int16_t progress_screen_subtext; // 0x006b2f68, UNSURE name/owner
extern int32_t interface_loading_screen_request_id; // 0x0068e688, UNSURE name/owner
extern int32_t game_variant_history_current; // 0x00687b18, UNSURE name/owner
extern int16_t network_game_mode; // 0x00719720
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc
extern int32_t sv_maxplayers_value; // 0x00699584

extern void network_channels_open(void); // this module (earlier batch), 0x441300
extern void network_game_server_host_dispose(network_server_globals *server); // this module (earlier batch), 0x4deda0
extern void network_client_globals_dispose(void); // this module (earlier batch), 0x4dde70
extern uint8_t network_game_server_host_create(void); // this module (earlier batch), 0x4ddd40
extern network_client_globals *network_session_create(void); // this module (earlier batch), 0x4d8a80
extern uint8_t game_engine_ensure_variant_history_has_entry(void); // foreign, UNSURE shape
extern void game_engine_apply_current_custom_variant(void); // foreign
extern void game_engine_sync_variant_defaults(void); // foreign
extern void network_host_round_reset(void); // this module (earlier batch)
extern void widget_close_all(void); // foreign

// Tears down any existing hosted session, opens the network channels, creates the server host
// and (if not already present) the shared client/session globals, applies default UI/engine
// state, switches network_game_mode to host (2), and writes name/password into the new server.
// Returns 1 on success, 0 on any failure (each of which also tears the partial state back down).
uint8_t network_game_start_new_server_with_name_and_password(uint32_t unused, uint16_t *name,
    uint16_t *password)
{
    uint8_t ok;

    if (network_server != 0) {
        network_game_server_host_dispose(network_server);
        network_server = 0;
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    if (*name == 0) {
        static const uint16_t default_name[] = { 'H', 'a', 'l', 'o', 0 };
        name = (uint16_t *)default_name;
    }
    network_session_host_flags_byte = network_game_info_packet_flag;
    network_channels_open();
    if (network_channels_open_ok == 0) {
        goto fail;
    }
    message_delta_vector3d_mode = (network_game_info_packet_flag == 1);
    ok = network_game_server_host_create();
    if (ok == 1) {
        if (((network_server->flags >> 2) & 1) == 0) {
            network_client = network_session_create();
            ok = 0;
            if (network_client == 0) {
                goto fail_or_dispose;
            }
            network_host_handoff_requested = 0;
            *(int32_t *)((uint8_t *)network_client + 0xf4c) = 4;
        }
        interface_loading_screen_address_a = -1;
        interface_loading_screen_address_b = -1;
        join_ui_state = 0;
        interface_loading_screen_progress = 0;
        progress_screen_text = 0;
        progress_screen_subtext = 0;
        interface_loading_screen_request_id = -1;
        ok = game_engine_ensure_variant_history_has_entry();
        if (ok == 0) {
        fail:
            if (network_server != 0) {
                network_game_server_host_dispose(network_server);
                network_server = 0;
                network_server_host_valid = 0;
            }
            network_client_globals_dispose();
            return 0;
        }
        game_variant_history_current = -1;
        game_engine_apply_current_custom_variant();
        game_engine_sync_variant_defaults();
        network_game_mode = 2;
        network_host_round_reset();
        network_disconnect_timeout_flag = 1;
    } else {
    fail_or_dispose:
        if (ok == 0) {
            goto fail;
        }
    }
    if (network_server == 0) {
        network_client_globals_dispose();
        return 0;
    }
    {
        network_channel *listen_channel = network_server->listen_channel;
        network_server->flags = network_server->flags | 1;
        listen_channel->listening = 1;
        // UNSURE: server+0x9d5 (inside network_server_globals::unknown_9bc); cleared here.
        *((uint8_t *)network_server + 0x9d5) = 0;
        // UNSURE: writes a wide name across server+0x008..+0x086 -- see this file's header note.
        wcsncpy((wchar_t *)((uint8_t *)network_server + 8), (const wchar_t *)name, 0x3f);
        *(uint16_t *)((uint8_t *)network_server + 0x86) = 0;
        wcsncpy((wchar_t *)network_server->password, (const wchar_t *)password, 8);
        network_server->password[8] = 0;
        {
            int32_t max_players = sv_maxplayers_value;
            if (max_players < 0) {
                max_players = 0;
                sv_maxplayers_value = 0;
            } else if (0x10 < max_players) {
                max_players = 0x10;
                sv_maxplayers_value = 0x10;
            }
            network_server->session.maximum_players = (uint8_t)max_players;
        }
        widget_close_all();
        network_server->new_server_pending = 1;
        if (((network_server->flags >> 2) & 1) == 0) {
            join_ui_state = 2;
        }
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x4e4150), from tools/pack.py 0x4e4150:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
network_game_start_new_server_with_name_and_password
          (undefined4 param_1,wchar_t *param_2,wchar_t *param_3)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  bool bVar4;

  if (DAT_0071c2d4 != (int *)0x0) {
    network_game_server_host_dispose(DAT_0071c2d4);
    DAT_0071c2d4 = (int *)0x0;
    DAT_0071c2dd = 0;
  }
  FUN_004dde70();
  if (*param_2 == L'\0') {
    param_2 = L"Halo";
  }
  DAT_0069fe00 = DAT_006894a2;
  network_channels_open();
  if (DAT_006869be == '\0') goto LAB_004e4289;
  DAT_0069b350 = (uint)(DAT_006894a2 == '\x01');
  iVar3 = network_game_server_host_create();
  cVar2 = (char)iVar3;
  if (cVar2 == '\x01') {
    if ((*(byte *)((int)DAT_0071c2d4 + 6) >> 2 & 1) == 0) {
      DAT_0071c2d8 = network_session_create();
      cVar2 = '\0';
      if (DAT_0071c2d8 == 0) goto LAB_004e4285;
      DAT_0071c2de = 0;
      *(undefined4 *)(DAT_0071c2d8 + 0xf4c) = 4;
    }
    DAT_0068e680 = 0xffffffff;
    DAT_0068e684 = 0xffffffff;
    DAT_00718f8c = 0;
    DAT_00718f90 = 0;
    _DAT_006b2f28 = 0;
    _DAT_006b2f68 = 0;
    DAT_0068e688 = 0xffffffff;
    cVar2 = FUN_00463b20();
    if (cVar2 == '\0') {
LAB_004e4289:
      if (DAT_0071c2d4 != (int *)0x0) {
        network_game_server_host_dispose(DAT_0071c2d4);
        DAT_0071c2d4 = (int *)0x0;
        DAT_0071c2dd = 0;
      }
      FUN_004dde70();
      return 0;
    }
    DAT_00687b18 = 0xffffffff;
    game_engine_apply_current_custom_variant();
    FUN_0045fc80();
    DAT_00719720 = 2;
    FUN_004df640();
    DAT_0071c2dc = 1;
  }
  else {
LAB_004e4285:
    if (cVar2 == '\0') goto LAB_004e4289;
  }
  piVar1 = DAT_0071c2d4;
  if (DAT_0071c2d4 != (int *)0x0) {
    iVar3 = *DAT_0071c2d4;
    *(byte *)((int)DAT_0071c2d4 + 6) = *(byte *)((int)DAT_0071c2d4 + 6) | 1;
    *(undefined1 *)(iVar3 + 0xae0) = 1;
    *(undefined1 *)((int)piVar1 + 0x9d5) = 0;
    _wcsncpy((wchar_t *)(piVar1 + 2),param_2,0x3f);
    *(undefined2 *)((int)piVar1 + 0x86) = 0;
    _wcsncpy((wchar_t *)(piVar1 + 0x27f),param_3,8);
    iVar3 = DAT_00699584;
    bVar4 = DAT_00699584 < 0;
    *(undefined2 *)(piVar1 + 0x283) = 0;
    if (bVar4) {
      iVar3 = 0;
      DAT_00699584 = iVar3;
    }
    else if (0x10 < iVar3) {
      iVar3 = 0x10;
      DAT_00699584 = iVar3;
    }
    *(char *)((int)piVar1 + 0x1a5) = (char)iVar3;
    widget_close_all();
    *(undefined1 *)((int)piVar1 + 0x9fa) = 1;
    if ((*(byte *)((int)piVar1 + 6) >> 2 & 1) == 0) {
      DAT_00718f8c = 2;
    }
    return 1;
  }
  FUN_004dde70();
  return 0;
}
#endif
