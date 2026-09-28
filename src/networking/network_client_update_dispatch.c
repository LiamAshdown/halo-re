// network_client_update_dispatch  (Ghidra: FUN_004dded0; named per this rewrite)
// address 0x4dded0, size 224 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: out/phase4/networking_functions.md: "Either performs the same host shutdown
// sequence as network_host_shutdown_or_defer (map-state reset and host dispose) when DAT_0071c2de is set, or
// attempts to join/prepare via network_client_state_dispatch/network_client_connect_progress_percent otherwise." network_client_state_dispatch and network_client_connect_progress_percent
// are already named (network_client_state_dispatch, network_client_connect_progress_percent) by
// an earlier batch covering 0x4d8a80..0x4d9050. See network_host_shutdown_or_defer.c for the
// shared shutdown sequence's field evidence.
// UNSURE: DAT_006982e8 is not documented anywhere in types/networking.h; declared generically.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested;  // 0x0071c2de
extern int16_t network_game_mode;               // 0x00719720
extern uint8_t network_server_host_valid;                // 0x0071c2dd, UNSURE identity
extern uint32_t split_screen_quit_prompt_string;               // 0x00719754, UNSURE identity; accessed by byte/word
extern uint32_t network_join_error_reason;               // 0x0071973c, UNSURE identity
extern int32_t unknown_006982e8;                // 0x006982e8, UNSURE identity

extern void main_menu_music_stop(void); // 0x4c8b40, outside this batch
extern void chimera__load_ui_map(char reset); // 0x4c8930, outside this batch
extern void network_client_globals_dispose(void); // 0x4dde70, this batch
extern void network_game_server_host_dispose(network_server_globals *host); // 0x4deda0, this batch
extern char network_client_state_dispatch(void); // 0x4d8bb0, outside this batch
extern int32_t network_client_connect_progress_percent(void); // 0x4d8c10, outside this batch

char network_client_update_dispatch(void)
{
    network_game_session *session;
    char result;
    char dispatch_result;

    result = 1;
    if (network_host_handoff_requested == 1) {
        network_game_mode = 0;
        main_menu_music_stop();
        if (network_server != 0) {
            session = &network_server->session;
        } else if (network_client != 0) {
            session = &network_client->session;
        } else {
            session = 0;
        }
        if (session->unknown_3ac != 0) {
            chimera__load_ui_map(1);
        }
        session->unknown_3ac = 0;
        network_client_globals_dispose();
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        *(uint16_t *)&split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        *((uint8_t *)&split_screen_quit_prompt_string + 3) = 1;
    } else {
        dispatch_result = network_client_state_dispatch();
        result = 0;
        if (dispatch_result != 0) {
            if (network_client->unknown_edc == 0) {
                unknown_006982e8 = network_client_connect_progress_percent();
                return dispatch_result;
            }
            return 0;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4dded0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

char FUN_004dded0(void)

{
  char cVar1;
  int *piVar2;
  char cVar3;

  cVar3 = '\x01';
  if (DAT_0071c2de == '\x01') {
    DAT_00719720 = 0;
    main_menu_music_stop();
    if (DAT_0071c2d4 == (int *)0x0) {
      if (DAT_0071c2d8 == 0) {
        piVar2 = (int *)0x0;
      }
      else {
        piVar2 = (int *)(DAT_0071c2d8 + 0xb14);
      }
    }
    else {
      piVar2 = DAT_0071c2d4 + 2;
    }
    if ((char)piVar2[0xeb] != '\0') {
      chimera__load_ui_map('\x01');
    }
    piVar2[0xeb] = 0;
    FUN_004dde70();
    if (DAT_0071c2d4 != (int *)0x0) {
      network_game_server_host_dispose(DAT_0071c2d4);
      DAT_0071c2d4 = (int *)0x0;
      DAT_0071c2dd = 0;
    }
    DAT_00719754._0_2_ = 0xffff;
    DAT_0071973c = 0;
    DAT_00719754._3_1_ = 1;
  }
  else {
    cVar1 = FUN_004d8bb0();
    cVar3 = '\0';
    if (cVar1 != '\0') {
      if (*(short *)(DAT_0071c2d8 + 0xedc) == 0) {
        _DAT_006982e8 = FUN_004d8c10();
        return cVar1;
      }
      return '\0';
    }
  }
  return cVar3;
}
#endif
