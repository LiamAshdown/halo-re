// network_game_host_start  (Ghidra: network_game_host_start, already named)
// address 0x495ac0, size 288 bytes
// name confidence: 0.55   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Starts hosting a multiplayer game: tears down any
// prior host, loads the selected variant, shows the [connected pregame] widget"; types/game.h's
// confirmed game_variant_saved_default (0x00714de0, "restored by 0x463b20" ==
// game_engine_ensure_variant_history_has_entry) and game_engine_get_variant_by_name's (name
// [ECX], out [stack]) signature; chimera__load_ui_widget.c's 7-argument signature.
// register convention: cdecl, three stack parameters. objdump 0x495af3/0x495b06: param 1 is
// loaded into EDI for main_queue_map_change_by_name_or_clear (its hidden EDI argument, per
// symbols/functions.txt) and param 2 into ECX for game_engine_get_variant_by_name. Ghidra
// lost both register arguments, so the older reading that params 1 and 2 are unused was wrong.
// The variant is fetched into a 0x98 byte stack copy and then copied with rep movsd (0x26
// dwords) to game_variant_saved_default whatever the lookup returned.
// UNSURE: game_engine_sync_variant_defaults, network_client_globals_dispose, network_session_create and network_game_server_host_
// create/_dispose belong to the networking module and are modeled only from this call site.
// UNSURE: DAT_0071c2d4/_2dd/_2dc/_2d8/_2de/_00687b18/_00719754/_0071973c are not documented by
// any module's types header; named locally by role only.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_game.h"

extern network_server_globals *network_server; // 0x0071c2d4, UNSURE
extern uint8_t network_server_host_valid;  // 0x0071c2dd, UNSURE
extern int16_t network_game_mode; // 0x00719720, types/game.h: 0 local, 1 client, 2 host (word access)
extern game_variant game_variant_saved_default; // 0x00714de0
extern uint8_t game_variant_saved_default_valid; // 0x00714e78
extern uint8_t network_disconnect_timeout_flag; // 0x0071c2dc, UNSURE: caller's disconnect_timeout_flag
extern network_client_globals *network_client; // 0x0071c2d8, UNSURE
extern uint8_t network_host_handoff_requested;            // 0x0071c2de, UNSURE
extern int32_t game_variant_history_current;            // 0x00687b18, UNSURE
extern uint16_t split_screen_quit_prompt_string; // 0x00719754, word stores
extern uint8_t split_screen_quit_prompt_armed;   // 0x00719757
extern uint8_t network_join_error_reason; // 0x0071973c, byte stores only

extern void widget_close_all(void); // 0x498650
extern void network_game_server_host_dispose(void *host); // 0x4deda0
extern void network_client_globals_dispose(void);     // 0x4dde70, UNSURE
extern void main_queue_map_change_by_name_or_clear(char *map_name); // 0x4c87a0; blam-cc: EDI -> map_name

extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)
extern void game_engine_ensure_variant_history_has_entry(void); // 0x463b20
extern uint8_t network_game_server_host_create(void); // 0x4ddd40, UNSURE
extern void *network_session_create(void);              // 0x4d8a80, UNSURE


// Starts hosting a multiplayer game: closes every open UI widget, disposes any previous host
// session, resets the network mode, clears the map-change queue, fetches the default game
// variant into game_variant_saved_default, and opens the "connected pregame" screen. If that
// widget opens successfully, creates the host and network session; on success, applies the
// current custom variant and enters server mode (network_game_mode = 2). On any failure past
// that point, tears the host back down and falls back to a "not connected" status.
void network_game_host_start(char *map_name, char *variant_name, uint8_t disconnect_timeout_flag)
{
    widget_instance *widget;
    game_variant variant;

    widget_close_all();
    if (network_server != (void *)0) {
        network_game_server_host_dispose(network_server);
        network_server = (void *)0;
        network_server_host_valid = 0;
    }
    network_client_globals_dispose();
    network_game_mode = 0;
    main_queue_map_change_by_name_or_clear(map_name);

    game_engine_get_variant_by_name(variant_name, &variant);
    game_variant_saved_default = variant;
    game_variant_saved_default_valid = 1;

    widget = chimera__load_ui_widget(
        "ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen",
        (datum_index)0xffffffff, (widget_instance *)0, 0xffff, (datum_index)0xffffffff,
        (datum_index)0xffffffff, -1);
    if (widget != (widget_instance *)0) {
        game_engine_ensure_variant_history_has_entry();
        network_disconnect_timeout_flag = disconnect_timeout_flag;
        if (network_game_server_host_create() != 0) {
            network_client = network_session_create();
            if (network_client != (void *)0) {
                network_host_handoff_requested = 0;
                game_variant_history_current = -1;
                game_engine_apply_current_custom_variant();
                game_engine_sync_variant_defaults();
                network_game_mode = 2;
                return;
            }
        }
        if (network_server != (void *)0) {
            network_game_server_host_dispose(network_server);
            network_server = (void *)0;
            network_server_host_valid = 0;
        }
        network_client_globals_dispose();
        network_disconnect_timeout_flag = 0;
    }
    split_screen_quit_prompt_string = 0xffff; // objdump 0x495bc3
    network_join_error_reason = 0;
    split_screen_quit_prompt_armed = 1;
}

#if 0
Original Ghidra decompilation (0x495ac0):

void network_game_host_start(undefined4 param_1,undefined4 param_2,undefined1 param_3)

{
  int iVar1;
  undefined4 *puVar2;
  undefined4 *puVar3;
  undefined4 local_98 [38];

  widget_close_all();
  if (DAT_0071c2d4 != (int *)0x0) {
    network_game_server_host_dispose(DAT_0071c2d4);
    DAT_0071c2d4 = (int *)0x0;
    DAT_0071c2dd = 0;
  }
  FUN_004dde70();
  DAT_00719720 = 0;
  main_queue_map_change_by_name_or_clear();
  game_engine_get_variant_by_name(local_98);
  puVar2 = local_98;
  puVar3 = &DAT_00714de0;
  for (iVar1 = 0x26; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar3 = *puVar2;
    puVar2 = puVar2 + 1;
    puVar3 = puVar3 + 1;
  }
  DAT_00714e78 = 1;
  iVar1 = chimera__load_ui_widget
                    ("ui\\shell\\main_menu\\multiplayer_type_select\\connected\\pregame\\connected_pregame_screen"
                     ,0xffffffff,0,0xffffffff,0xffffffff,0xffffffff,0xffffffff);
  if (iVar1 != 0) {
    FUN_00463b20();
    DAT_0071c2dc = param_3;
    iVar1 = network_game_server_host_create();
    if ((char)iVar1 != '\0') {
      DAT_0071c2d8 = network_session_create();
      if (DAT_0071c2d8 != 0) {
        DAT_0071c2de = 0;
        DAT_00687b18 = 0xffffffff;
        game_engine_apply_current_custom_variant();
        FUN_0045fc80();
        DAT_00719720 = 2;
        return;
      }
    }
    if (DAT_0071c2d4 != (int *)0x0) {
      network_game_server_host_dispose(DAT_0071c2d4);
      DAT_0071c2d4 = (int *)0x0;
      DAT_0071c2dd = 0;
    }
    FUN_004dde70();
    DAT_0071c2dc = 0;
  }
  DAT_00719754._0_2_ = 0xffff;
  DAT_0071973c = 0;
  DAT_00719754._3_1_ = 1;
  return;
}
#endif
