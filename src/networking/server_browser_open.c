// server_browser_open  (Ghidra: server_browser_open, already named)
// address 0x4b75b0, size 870 bytes
// name confidence: 0.5   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md summary implied by its assigned name; reuses
// this module's own master_server_connection_start, server_list_reset,
// join_game_ticker_string_copy and join_game_server_browser_tick; server_browser_join_requested/
// _target/_target_has_password (0x00719491/50/54) match server_browser_latch_join_target.c;
// server_browser_filter_* globals (0x0071948b..0x00719490, 0x006953f8..0x006953fc) match
// types/networking.h's server_browser_filters and server_browser_server_passes_filter.c.
// register convention: root server-browser widget in ECX-ish (Ghidra recognizes it as a real
// parameter, param_1).
// UNSURE: this function operates on the same undocumented UI widget tree as
// server_browser_filter_panel_set_mode.c; declared here with its own local `network_ui_widget`
// network_ui_widget node in types/networking.h (this file contributed its alpha/label_text/
// highlight_flag fields). Every widget-tree hop is transcribed literally by offset; no
// semantic name is claimed for any individual widget beyond what the code makes unambiguous.
// UNSURE: when saved_player_profile_slots_handle != -1, the original copies 0x7ff (2047) dwords from
// &profile_globals_block into a local buffer only declared for ~830 dwords before the individually
// named bytes it actually reads back -- this looks like a latent stack buffer overflow in the
// original binary (only reachable if a saved/legacy config format is present); reproduced
// exactly, not fixed, and marked here rather than silently corrected.
// UNSURE: several call sites (shell_load_localized_string's destination buffer,
// join_game_ticker_string_copy's buffer/capacity, text_string_list_get_string's arguments) are
// shown with fewer visible arguments than their own files' documented signatures require;
// reconstructed with the most plausible buffer (this function's own locals / the shared
// DAT_00719498 ticker-label buffer) and flagged individually below.
// UNSURE: ServerBrowserNew (master-server query engine constructor, 8 arguments) and LAB_004ba660
// (a callback address outside this session's range) are declared only by observed shape.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"
#include <string.h>
#include <wchar.h>

extern uint8_t server_browser_initialized;      // 0x00719470
extern void *master_server_query_engine;        // 0x0071946c
extern uint8_t network_session_start_host_name[];                  // ServerBrowserNew argument, see UNSURE
extern uint8_t network_session_start_map_name[];                  // ServerBrowserNew argument, see UNSURE
extern uint8_t server_browser_join_requested;   // 0x00719491
extern void *server_browser_join_target;        // 0x00719450
extern uint8_t server_browser_join_target_has_password; // 0x00719454
extern uint8_t network_join_target_address;                    // see UNSURE
extern uint8_t server_browser_require_valid_entry; // 0x006953f0
extern uint8_t DAT_00719698;                    // motd/autopatch state, see UNSURE
extern int32_t DAT_00695420;                    // autopatch slot/request id, see UNSURE
extern wchar_t DAT_00719498[0x100];             // ticker label buffer
extern uint8_t DAT_00719696;                    // see UNSURE
extern int32_t DAT_006b5e6c;
extern int32_t DAT_006b5e88;
extern int32_t server_browser_player_ticker;
extern int32_t DAT_006b5e68;
extern int32_t DAT_006b5e64;
extern int32_t DAT_006b5e60;
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74
extern int32_t DAT_006b5e84;
extern int32_t DAT_006b5e80;
extern int32_t DAT_006b5e7c;
extern int32_t saved_player_profile_slots_handle;                    // -1 == "no saved filter config"
extern uint8_t profile_globals_block[];                  // saved filter config blob, see UNSURE
extern uint8_t server_browser_filter_dedicated_only; // 0x0071948b
extern uint8_t server_browser_allow_password;        // 0x006953f9
extern uint8_t server_browser_filter_allow_unknown_map; // 0x0071948d
extern uint8_t server_browser_filter_classic_only;   // 0x0071948c
extern uint8_t server_browser_allow_full;            // 0x006953fb
extern uint8_t server_browser_allow_empty;           // 0x006953fa
extern uint8_t server_browser_filter_teamplay;       // 0x0071948f
extern uint8_t server_browser_filter_gametype;       // 0x0071948e
extern uint8_t server_browser_sort_ascending;        // 0x006953f8
extern uint8_t server_browser_sort_column;           // 0x00719489
extern uint8_t server_browser_filter_ping_limit_index; // 0x00719490
extern uint8_t server_browser_filter_panel_mode;     // 0x007196b4
extern int32_t DAT_006953fc;
extern uint8_t DAT_00719488;


extern void *ServerBrowserNew(void *a, void *b, void *c, int32_t d, int32_t e, int32_t f,
                           void *callback, int32_t h); // foreign, GameSpy library
extern void network_channel_gap_4ba660(void); // callback, outside this session's range
extern void autopatch_download_pool_initialize(void); // 0x576c30, outside this session's range
extern int32_t shell_load_localized_string(int32_t id, char *out_buffer); // foreign, see UNSURE
extern int32_t autopatch_download_start(const char *source); // 0x576e60, outside this session's range

extern int32_t tag_lookup(const char *path); // foreign, tags module
extern uint16_t *text_string_list_get_string(int32_t tag_index, int32_t string_index); // foreign, see UNSURE
extern void server_list_reset(void); // 0x4b65f0, this module
extern void join_game_server_browser_tick(network_ui_widget *root); // 0x4b80f0, this module

// blam-cc: root server-browser widget as param_1
int32_t server_browser_open(network_ui_widget *root)
{
    char motd_string[1020];
    int32_t motd_available;
    int32_t tag_index;
    uint16_t *source;
    uint8_t saved_config[3200]; // see UNSURE: the original copy loop overruns this
    network_ui_widget *w0;
    network_ui_widget *w1;
    network_ui_widget *w2;
    network_ui_widget *w3;
    network_ui_widget *w4;
    network_ui_widget *list_container;
    network_ui_widget *col_headers[15];
    network_ui_widget *w;
    int32_t i;

    network_channels_open();
    if (server_browser_initialized == 0 && master_server_connection_start() == 0) {
        server_browser_join_requested = 1;
        master_server_query_engine = ServerBrowserNew(&network_session_start_host_name, &network_session_start_host_name, &network_session_start_map_name, 0,
                                                    10, 1, (void *)network_channel_gap_4ba660, 0);
    }
    server_browser_join_target = 0;
    server_browser_join_target_has_password = 0;
    network_join_target_address = 0;
    autopatch_download_pool_initialize();
    if (server_browser_require_valid_entry != 0 && DAT_00719698 == 0) {
        motd_available = shell_load_localized_string(0x90, motd_string);
        if (motd_available != 0) {
            DAT_00695420 = autopatch_download_start(motd_string);
            DAT_00719698 = 1;
            join_game_ticker_string_copy(DAT_00719498, 0x100, 4);
        }
    }
    if (DAT_00719498[0] == 0) {
        DAT_00719498[0] = 0;
        tag_index = tag_lookup(
            "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels");
        if (tag_index != -1) {
            source = text_string_list_get_string(tag_index, 0);
            wcsncpy(DAT_00719498, source, 0xff);
            DAT_00719696 = 0;
        }
    }
    DAT_006b5e6c = 100;
    DAT_006b5e88 = 100;
    server_browser_player_ticker = 0;
    DAT_006b5e68 = 0;
    DAT_006b5e64 = 0;
    DAT_006b5e60 = 0;
    server_browser_variant_ticker.text = 0; // 0x4b76ff: mov DWORD PTR ds:0x6b5e74,ebx
    DAT_006b5e84 = 0;
    DAT_006b5e80 = 0;
    DAT_006b5e7c = 0;

    if (saved_player_profile_slots_handle == -1) {
        saved_config[0x1787] = 1;
        saved_config[0x1788] = 3;
        saved_config[0x1786] = 1;
        saved_config[0x1785] = 0;
        saved_config[0x1784] = 0;
        saved_config[0x1783] = 0;
        saved_config[0x1782] = 1;
        saved_config[0x1781] = 1;
        saved_config[0x1780] = 0;
        saved_config[0x177f] = 0;
        saved_config[0x177e] = 0;
    } else {
        memcpy(saved_config, profile_globals_block, 0x7ff * 4); // UNSURE: overruns saved_config, see file header
    }
    server_browser_filter_dedicated_only = saved_config[0x1785];
    server_browser_allow_password = saved_config[0x1786];
    server_browser_filter_allow_unknown_map = saved_config[0x1783];
    server_browser_filter_classic_only = saved_config[0x1784];
    server_browser_allow_full = saved_config[0x1781];
    server_browser_allow_empty = saved_config[0x1782];
    server_browser_filter_teamplay = saved_config[0x177f];
    server_browser_filter_gametype = saved_config[0x1780];
    w = root->first_child->next_sibling->next_sibling->next_sibling;
    server_browser_sort_ascending = saved_config[0x1787];
    server_browser_sort_column = saved_config[0x1788];
    server_browser_filter_ping_limit_index = saved_config[0x177e];
    w->visible = 0;
    w->hidden = 1;
    w->parent->selected_child = w->parent->first_child->next_sibling;
    w->first_child->first_child->value = 0x28;

    w0 = root->first_child;
    w0->visible = 1;
    w0->hidden = 0;
    w1 = w0->next_sibling;
    w1->visible = 1;
    w1->hidden = 0;
    w2 = w1->next_sibling;
    w2->visible = 1;
    w2->hidden = 0;
    w3 = w2->next_sibling;
    w3->visible = 0;
    w3->hidden = 1;
    w4 = w3->next_sibling;
    w4->visible = 0;
    w4->hidden = 1;
    w4->parent->selected_child = w4->parent->first_child->next_sibling->next_sibling;

    server_browser_filter_panel_mode = 0;
    server_browser_initialized = 1;
    server_list_reset();

    w2 = root->first_child->next_sibling->next_sibling;
    root->selected_child = w2;
    list_container = w2->first_child->first_child;
    w = w2->first_child->next_sibling->next_sibling->next_sibling;
    for (i = 0; i < 0xf; i++) {
        col_headers[i] = w;
        w = w->next_sibling;
    }
    w = w->next_sibling->next_sibling;
    w2->selected_child = w;
    motd_available = server_browser_require_valid_entry == 0;
    w->selected_child = w->first_child;
    if (motd_available) {
        root->first_child->first_child->visible = 0;
        root->first_child->first_child->next_sibling->visible = 1;
    } else {
        root->first_child->first_child->visible = 1;
        root->first_child->first_child->next_sibling->visible = 0;
    }
    list_container->parent->visible = 0;
    list_container->parent->hidden = 1;
    join_game_server_browser_tick(root->selected_child);
    DAT_006953fc = 0;
    DAT_00719488 = 0;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4b75b0):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 server_browser_open(int param_1)

{
  int iVar1;
  char cVar2;
  int extraout_EAX;
  int iVar3;
  wchar_t *_Source;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  undefined4 *puVar7;
  bool bVar8;
  int aiStack_2444 [15];
  undefined4 local_2408 [800];
  undefined1 local_1788;
  undefined1 local_1787;
  undefined1 local_1786;
  undefined1 local_1785;
  undefined1 local_1784;
  undefined1 local_1783;
  undefined1 local_1782;
  undefined1 local_1781;
  undefined1 local_1780;
  undefined1 local_177f;
  undefined1 local_177e;
  undefined1 local_408 [1020];
  undefined4 uStack_c;

  uStack_c = 0x4b75c0;
  network_channels_open();
  if ((DAT_00719470 == '\0') && (cVar2 = master_server_connection_start(), cVar2 == '\0')) {
    DAT_00719491 = 1;
    DAT_0071946c = FUN_00616eb0(&DAT_00722798,&DAT_00722798,&DAT_007227a0,0,10,1,&LAB_004ba660,0);
  }
  DAT_00719450 = 0;
  DAT_00719454 = 0;
  DAT_00719458 = 0;
  autopatch_download_pool_initialize();
  if ((DAT_006953f0 != '\0') && (DAT_00719698 == 0)) {
    shell_load_localized_string(0x90);
    if (extraout_EAX != 0) {
      DAT_00695420 = autopatch_download_start(local_408);
      DAT_00719698 = 1;
      FUN_004b6160(4);
    }
  }
  if (DAT_00719498 == 0) {
    DAT_00719498 = 0;
    iVar3 = tag_lookup(
                      "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels"
                      );
    if (iVar3 != -1) {
      _Source = (wchar_t *)text_string_list_get_string();
      _wcsncpy(&DAT_00719498,_Source,0xff);
      _DAT_00719696 = 0;
    }
  }
  _DAT_006b5e6c = 100;
  DAT_006b5e88 = 100;
  _DAT_006b5e58 = 0;
  _DAT_006b5e68 = 0;
  _DAT_006b5e64 = 0;
  _DAT_006b5e60 = 0;
  _DAT_006b5e74 = 0;
  _DAT_006b5e84 = 0;
  _DAT_006b5e80 = 0;
  _DAT_006b5e7c = 0;
  if (DAT_00714dd4 == -1) {
    local_1787 = 1;
    local_1788 = 3;
    local_1786 = 1;
    local_1785 = 0;
    local_1784 = 0;
    local_1783 = 0;
    local_1782 = 1;
    local_1781 = 1;
    local_1780 = 0;
    local_177f = 0;
    local_177e = 0;
  }
  else {
    puVar6 = &DAT_00712dd8;
    puVar7 = local_2408;
    for (iVar3 = 0x7ff; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar7 = *puVar6;
      puVar6 = puVar6 + 1;
      puVar7 = puVar7 + 1;
    }
  }
  DAT_0071948b = local_1785;
  DAT_006953f9 = local_1786;
  DAT_0071948d = local_1783;
  DAT_0071948c = local_1784;
  DAT_006953fb = local_1781;
  DAT_006953fa = local_1782;
  DAT_0071948f = local_177f;
  DAT_0071948e = local_1780;
  iVar3 = *(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x2c) + 0x2c) + 0x2c);
  DAT_006953f8 = local_1787;
  DAT_00719489 = local_1788;
  DAT_00719490 = local_177e;
  *(undefined1 *)(iVar3 + 0x10) = 0;
  *(undefined1 *)(iVar3 + 0x12) = 1;
  *(undefined4 *)(*(int *)(iVar3 + 0x30) + 0x38) =
       *(undefined4 *)(*(int *)(*(int *)(iVar3 + 0x30) + 0x34) + 0x2c);
  *(undefined2 *)(*(int *)(*(int *)(iVar3 + 0x34) + 0x34) + 0x40) = 0x28;
  iVar3 = *(int *)(param_1 + 0x34);
  *(undefined1 *)(iVar3 + 0x10) = 1;
  *(undefined1 *)(iVar3 + 0x12) = 0;
  iVar3 = *(int *)(iVar3 + 0x2c);
  *(undefined1 *)(iVar3 + 0x10) = 1;
  *(undefined1 *)(iVar3 + 0x12) = 0;
  iVar3 = *(int *)(iVar3 + 0x2c);
  *(undefined1 *)(iVar3 + 0x10) = 1;
  *(undefined1 *)(iVar3 + 0x12) = 0;
  iVar3 = *(int *)(iVar3 + 0x2c);
  *(undefined1 *)(iVar3 + 0x10) = 0;
  *(undefined1 *)(iVar3 + 0x12) = 1;
  iVar3 = *(int *)(iVar3 + 0x2c);
  *(undefined1 *)(iVar3 + 0x10) = 0;
  *(undefined1 *)(iVar3 + 0x12) = 1;
  *(undefined4 *)(*(int *)(iVar3 + 0x30) + 0x38) =
       *(undefined4 *)(*(int *)(*(int *)(*(int *)(iVar3 + 0x30) + 0x34) + 0x2c) + 0x2c);
  DAT_007196b4 = 0;
  DAT_00719470 = 1;
  server_list_reset();
  iVar3 = *(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x2c) + 0x2c);
  *(int *)(param_1 + 0x38) = iVar3;
  iVar1 = *(int *)(*(int *)(iVar3 + 0x34) + 0x34);
  iVar4 = *(int *)(*(int *)(*(int *)(*(int *)(iVar3 + 0x34) + 0x2c) + 0x2c) + 0x2c);
  iVar5 = 0;
  do {
    aiStack_2444[iVar5] = iVar4;
    iVar4 = *(int *)(iVar4 + 0x2c);
    iVar5 = iVar5 + 1;
  } while (iVar5 < 0xf);
  iVar4 = *(int *)(*(int *)(iVar4 + 0x2c) + 0x2c);
  *(int *)(iVar3 + 0x38) = iVar4;
  bVar8 = DAT_006953f0 == '\0';
  *(undefined4 *)(iVar4 + 0x38) = *(undefined4 *)(iVar4 + 0x34);
  if (bVar8) {
    *(undefined1 *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x10) = 0;
    *(undefined1 *)(*(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x2c) + 0x10) = 1;
  }
  else {
    *(undefined1 *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x10) = 1;
    *(undefined1 *)(*(int *)(*(int *)(*(int *)(param_1 + 0x34) + 0x34) + 0x2c) + 0x10) = 0;
  }
  *(undefined1 *)(*(int *)(iVar1 + 0x30) + 0x10) = 0;
  *(undefined1 *)(*(int *)(iVar1 + 0x30) + 0x12) = 1;
  join_game_server_browser_tick(*(undefined4 *)(param_1 + 0x38));
  DAT_006953fc = 0;
  DAT_00719488 = 0;
  return 1;
}
#endif
