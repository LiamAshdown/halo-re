// network_host_shutdown_or_defer  (Ghidra: FUN_004ddd90; named per this rewrite)
// address 0x4ddd90, size 184 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "If a network host is active and not in the
// special team-sync case, resets the host's map-load flag and history state and disposes the
// host globals; otherwise defers to network_host_update_tick." session->unknown_3ac (the map-loaded flag)
// matches types/networking.h's network_game_session exactly when reached through
// network_server->session or network_client->session directly (unlike
// network_game_server_host_create.c's host-relative offset, which lands elsewhere -- see that
// file's UNSURE note).
// UNSURE: DAT_0071c2dd, DAT_00719754 and DAT_0071973c are not documented anywhere in
// types/networking.h and are not touched by any other file in this batch; declared generically
// below and left unrenamed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern network_server_globals *network_server; // 0x0071c2d4
extern network_client_globals *network_client; // 0x0071c2d8
extern uint8_t network_host_handoff_requested;  // 0x0071c2de
extern int16_t network_game_mode;               // 0x00719720
extern uint8_t network_server_host_valid;                // 0x0071c2dd, UNSURE identity
extern uint32_t split_screen_quit_prompt_string;               // 0x00719754, UNSURE identity; accessed by byte/word
extern uint32_t network_join_error_reason;               // 0x0071973c, UNSURE identity

extern void main_menu_music_stop(void); // 0x4c8b40, outside this batch
extern void chimera__load_ui_map(char reset); // 0x4c8930, outside this batch
extern void network_client_globals_dispose(void); // 0x4dde70, this batch
extern void network_game_server_host_dispose(network_server_globals *host); // 0x4deda0, this batch
extern char network_host_update_tick(network_server_globals *host); // 0x4def80, this module

int32_t network_host_shutdown_or_defer(void)
{
    network_game_session *session;

    if (network_server != 0) {
        if (network_host_handoff_requested != 1 || ((network_server->flags >> 2) & 1) == 0) {
            // 0x4dde3f calls 0x4def80 with EAX still holding network_server (loaded at 0x4ddd91).
            return network_host_update_tick(network_server);
        }
        network_game_mode = 0;
        main_menu_music_stop();
        if (network_server != 0) {
            session = &network_server->session;
        } else if (network_client != 0) {
            session = &network_client->session;
        } else {
            session = 0;
        }
        if (session->map_loaded != 0) {
            chimera__load_ui_map(1);
        }
        session->map_loaded = 0;
        network_client_globals_dispose();
        if (network_server != 0) {
            network_game_server_host_dispose(network_server);
            network_server = 0;
            network_server_host_valid = 0;
        }
        *(uint16_t *)&split_screen_quit_prompt_string = 0xffff;
        network_join_error_reason = 0;
        *((uint8_t *)&split_screen_quit_prompt_string + 3) = 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4ddd90):

undefined4 FUN_004ddd90(void)

{
  int *piVar1;
  undefined4 uVar2;

  if (DAT_0071c2d4 != (int *)0x0) {
    if ((DAT_0071c2de != '\x01') || ((*(byte *)((int)DAT_0071c2d4 + 6) >> 2 & 1) == 0)) {
      uVar2 = FUN_004def80();
      return uVar2;
    }
    DAT_00719720 = 0;
    main_menu_music_stop();
    if (DAT_0071c2d4 == (int *)0x0) {
      if (DAT_0071c2d8 == 0) {
        piVar1 = (int *)0x0;
      }
      else {
        piVar1 = (int *)(DAT_0071c2d8 + 0xb14);
      }
    }
    else {
      piVar1 = DAT_0071c2d4 + 2;
    }
    if ((char)piVar1[0xeb] != '\0') {
      chimera__load_ui_map('\x01');
    }
    piVar1[0xeb] = 0;
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
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
