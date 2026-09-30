// join_game_server_browser_tick  (Ghidra: join_game_server_browser_tick, already named)
// address 0x4b80f0, size 2261 bytes
// name confidence: 0.5   rewrite confidence: 0.15
// evidence: out/phase4/networking_functions.md summary implied by its assigned name; this is
// the per-frame driver server_browser_open.c hands its list panel to; reuses this module's own
// server_list_scroll_page_up/_down, server_list_reset, server_browser_player_list_populate,
// server_browser_selected_variant_description_build, server_browser_ui_refresh,
// join_game_ticker_string_copy, server_list_mutex_try_lock, master_server_list_refresh_request
// and master_server_process_pending_requests; autopatch_download_slot (0x006ef93c, two 0x14
// records) matches types/networking.h exactly; server_list (0x007196bc) and
// server_browser_join_target/_has_password (0x00719450/54) match this module's other files.
// register convention: list-panel widget in ??? (Ghidra recognizes it as a real parameter,
// param_1 -- this is the same widget server_browser_open.c passes as `root->selected_child`).
// This is by far the largest and least certain function in this session's range; it is
// transcribed close to literally (Ghidra's own variable shapes kept recognizable) rather than
// fully idiomatically renamed, because the risk of a silent transcription error is high and
// "same control flow, same arithmetic, same memory writes, same call order" outweighs style
// here. Every UI-widget-tree hop goes through types/networking.h's network_ui_widget, the
// interface module's menu-tree node as this module reads it (this file supplied the
// status_root field at +0x4c when the eight per-file copies were folded into the header).
// UNSURE (many, grouped): FUN_00490b50 is called four times bare with no visible argument,
// most likely four different literal action-id constants that Ghidra could not attribute;
// string_convert_ascii_to_unicode's single call here is likewise bare; string_format_wide_va_bounded's third
// argument (`local_80`, a 128-byte stack buffer never otherwise written) and
// autopatch_download_get_result's exact output semantics were not resolved; mouse_device/
// input_suppressed/mouse_neutral_state/live_mouse_state (a mouse-wheel-ish state pair) and DAT_007196a0/
// DAT_0071948a (a query-mode flag) have no documented names. Two float bit patterns
// (0x3f800000 == 1.0f, 0x3eaa7efa ~ 0.3333f) are written through a `uint32_t*` reinterpretation
// of the `alpha` field to avoid any floating-point literal rounding.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

extern void *server_browser_join_target;        // 0x00719450
extern uint8_t server_browser_join_target_has_password; // 0x00719454
extern uint8_t network_join_target_address;                    // see server_browser_open.c UNSURE
extern uint8_t DAT_007193be;                    // see UNSURE
extern int32_t mouse_device;                    // see UNSURE (mouse-wheel-ish state)
extern uint8_t input_suppressed;                    // see UNSURE
extern int32_t mouse_neutral_state[3];                 // see UNSURE
extern int32_t live_mouse_state[3];                 // see UNSURE
extern void *master_server_query_engine;        // 0x0071946c
extern network_thread_record *server_list_thread; // 0x007196ac
extern int32_t DAT_007196a0;                    // see UNSURE
extern uint8_t server_browser_query_pending;       // 0x0071948a, see UNSURE
extern int32_t server_list_scroll_offset;       // 0x00719478
extern int32_t server_browser_selected_index; // 0x006953f4
extern uint8_t server_browser_player_list_ready;                    // see server_list_reset.c UNSURE
extern int32_t server_browser_last_click_ms;                    // see server_list_reset.c UNSURE
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern uint8_t server_browser_join_requested;   // 0x00719491
extern int32_t master_server_last_result;       // 0x007196a4
extern int32_t DAT_00719484;                    // scroll fade counter, see UNSURE
extern uint32_t master_server_request_flags;    // 0x0071969c
extern int32_t master_server_connection_last_tick_ms; // 0x007196b8, see UNSURE (reused name)
extern int32_t DAT_006b5e88;                    // see server_browser_open.c
extern int32_t DAT_006953fc;                    // see master_server_list_refresh_request.c
extern int32_t DAT_00695420;                    // see server_browser_open.c UNSURE
extern autopatch_download_slot autopatch_download_slots[k_network_autopatch_download_slots]; // 0x006ef93c
extern wchar_t DAT_00719498[0x100];             // ticker label buffer
extern uint8_t DAT_00719696;                    // see server_browser_open.c UNSURE
extern uint8_t server_browser_require_valid_entry; // 0x006953f0
extern int32_t server_browser_total_players;    // 0x00719474
extern heap *widget_memory_pool;                 //  0x006926c4 -- the global holds a POINTER to the heap (mov esi,[0x6926c4] at every call site)
extern char k_empty_string[];                     // shared empty-string default buffer
extern wchar_t empty_string[];                  // see UNSURE, argument to server_browser_list_row_populate

extern int32_t network_join_request_resolve_host(void); // foreign, outside this session's range, see UNSURE
extern void widget_close_all(void); // 0x498650, outside this session's range
extern int32_t input_get_key_state(void); // foreign, outside this session's range, see UNSURE
extern void server_list_scroll_page_up(uint8_t jump_to_top); // 0x4b7b20, this module
extern void server_list_scroll_page_down(uint8_t jump_to_bottom); // 0x4b7bb0, this module
extern void server_list_reset(void); // 0x4b65f0, this module
extern uint8_t DAT_00719698; // motd/autopatch state, see server_browser_open.c UNSURE
extern void widget_play_sound_effect(void); // 0x498e90, outside this session's range
extern void server_browser_list_row_gather(network_ui_widget *row, uint8_t flag, void *entry); // 0x4b69c0, this module
extern void server_browser_list_row_populate(network_ui_widget *row, uint8_t flag1, uint8_t flag2,
                                               const char *server_name, wchar_t *map_name,
                                               const char *gametype_name, uint8_t flag3,
                                               int32_t count_a, int32_t count_b, int32_t ping); // 0x4b67e0, this module
extern void server_browser_filter_headers_refresh(network_ui_widget *row); // 0x4b7f70, this module, see UNSURE below at its call site
extern void master_server_process_pending_requests(void); // 0x4b5d70, this module
extern server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms); // 0x4ba760, this module
extern void server_browser_result_array_sort(void); // foreign, outside this session's range
extern void server_browser_query_results_ingest(void); // 0x4baae0, outside this session's range
extern int32_t SBServerHasFullKeys(void *entry); // foreign, GameSpy library
extern void server_browser_player_list_populate(void *entry); // 0x4b73e0, this module
extern int32_t server_browser_selected_variant_description_build(void *entry); // 0x4b74e0, this module
extern int32_t ServerBrowserState(void *engine); // foreign, GameSpy library
extern void *heap_reallocate(void *old_payload, uint32_t new_size, heap *self); // 0x4d1f80, memory module
extern void join_game_ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index); // 0x4b6160, this module, see UNSURE
extern void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...); // 0x557910, EDX count
    // the bound (0x1f at the 0x4b84e0 call sites) rides in EDX and is not modeled here // foreign, see UNSURE
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc
extern void ticker_text_buffer_advance(uint8_t *widget, ticker_text_buffer *self); // 0x4b8b40, this
    // module; its `widget` parameter is a raw byte pointer there because the body walks it by
    // literal offset (see that file), so the network_ui_widget is cast at the call sites.
extern ticker_text_buffer server_browser_player_ticker;  // 0x006b5e58
extern ticker_text_buffer server_browser_variant_ticker; // 0x006b5e74
extern void master_server_list_refresh_request(void); // 0x4b6660, this module
extern void server_browser_ui_refresh(void); // 0x4b73a0, this module
extern uint8_t autopatch_download_get_result(int32_t *result_a, int32_t *result_b); // foreign,
    // outside this session's range; the result is tested as `test al,al` at 0x4b88a8
// blam-cc: EAX -> dest, EDI -> dest capacity in BYTES, EBX -> ASCII source.
// Widens an ASCII string into dest and returns dest, or NULL when it does not fit.
extern wchar_t *string_convert_ascii_to_unicode(wchar_t *dest, int32_t dest_bytes, const char *source); // 0x557990
extern wchar_t string_widen_scratch[0x400]; // 0x006b5e90, the 0x800-byte shared target
extern int32_t tag_lookup(const char *path); // foreign, tags module
extern uint16_t *text_string_list_get_string(int32_t tag_index, int32_t string_index); // foreign, see UNSURE

// blam-cc: list-panel widget as param_1
int32_t join_game_server_browser_tick(network_ui_widget *browser_widget)
{
    network_ui_widget *password_panel;
    int32_t clicked;
    int32_t *scroll_target;
    network_ui_widget *list_container;
    network_ui_widget *sort_widget;
    network_ui_widget *up_arrow;
    network_ui_widget *down_arrow;
    network_ui_widget *col_headers[15];
    network_ui_widget *w17;
    network_ui_widget *w18;
    network_ui_widget *status_group;
    network_ui_widget *status_sub;
    network_ui_widget *status_leaf;
    network_ui_widget *w_iter;
    network_ui_widget *row_entry;
    void *entry;
    int32_t i;
    int32_t player_count;
    int32_t idx;
    int32_t scan_index;
    int32_t probe;
    server_list_globals *locked;
    int32_t engine_state;
    large_integer counter;
    int32_t now_ms;
    int32_t page_count;
    int32_t current_page;
    void *label;
    wchar_t scratch_80[64];
    int32_t autopatch_slot;
    int32_t state;
    int32_t got_result;
    int32_t result_a;
    int32_t result_b;
    int32_t tag_idx;
    uint16_t *src;
    int32_t bVar11;
    int32_t bVar12;

    if (server_browser_join_target != 0) {
        if (server_browser_join_target_has_password == 0 || network_join_target_address != 0) {
            clicked = network_join_request_resolve_host();
            if (clicked != 0) {
                widget_close_all();
                return 1;
            }
        } else if (DAT_007193be != 0) {
            password_panel = browser_widget->parent->first_child->next_sibling->next_sibling->next_sibling;
            password_panel->visible = 1;
            password_panel->hidden = 0;
            password_panel->parent->selected_child = password_panel;
            password_panel->first_child->first_child->value = 0x2a;
        }
        network_join_target_address = 0;
        server_browser_join_target_has_password = 0;
        server_browser_join_target = 0;
    }

    scroll_target = 0;
    if (mouse_device != 0) {
        scroll_target = mouse_neutral_state;
        if (input_suppressed == 0) {
            scroll_target = live_mouse_state;
        }
    }
    clicked = input_get_key_state();
    if (clicked == 1) {
        server_list_scroll_page_up(0);
        widget_play_sound_effect();
    }
    clicked = input_get_key_state();
    if (clicked == 1) {
        server_list_scroll_page_up(0);
        widget_play_sound_effect();
    }
    clicked = input_get_key_state();
    if (clicked == 1) {
        server_list_scroll_page_down(0);
        widget_play_sound_effect();
    }
    clicked = input_get_key_state();
    if (clicked == 1) {
        server_list_scroll_page_down(0);
        widget_play_sound_effect();
    }
    if (scroll_target != 0) {
        if (scroll_target[2] < 0) {
            server_list_scroll_page_up(0);
        } else if (0 < scroll_target[2]) {
            server_list_scroll_page_down(0);
        }
    }

    list_container = browser_widget->first_child->first_child;
    sort_widget = browser_widget->first_child->next_sibling;
    up_arrow = sort_widget->next_sibling;
    w_iter = up_arrow->next_sibling;
    for (i = 0; i < 0xf; i++) {
        col_headers[i] = w_iter;
        w_iter = w_iter->next_sibling;
    }
    down_arrow = w_iter;
    w17 = down_arrow->next_sibling;
    w18 = w17->next_sibling;
    status_group = w18->next_sibling;
    status_sub = status_group->first_child->next_sibling;
    status_leaf = status_sub->next_sibling->next_sibling;
    w18->hidden = 1;
    w17->hidden = 1;
    sort_widget->first_child->highlight_flag = 1;

    if (browser_widget->selected_child == status_group) {
        if (status_group->selected_child == 0) {
            status_group->selected_child = status_group->first_child;
        }
    } else {
        status_group->highlight_flag = 0;
        for (w_iter = status_group->first_child; w_iter != 0; w_iter = w_iter->next_sibling) {
            w_iter->highlight_flag = 0;
        }
    }
    if (browser_widget->selected_child == sort_widget) {
        if (sort_widget->selected_child == 0) {
            sort_widget->selected_child = sort_widget->first_child;
        }
    } else if (sort_widget->selected_child != 0) {
        sort_widget->selected_child = 0;
    }
    w_iter = list_container->parent;
    if (browser_widget->selected_child == w_iter && w_iter->selected_child == 0) {
        w_iter->selected_child = list_container;
    }
    // UNSURE: this call is bare in the original (no visible EAX); `sort_widget` is the closest
    // in-scope widget matching server_browser_filter_headers_refresh.c's expected row shape.
    server_browser_filter_headers_refresh(sort_widget);

    if (master_server_query_engine == 0) {
        idx = 0;
        do {
            row_entry = col_headers[idx];
            row_entry->highlight_flag = (row_entry->parent->selected_child == row_entry);
            server_browser_list_row_populate(row_entry, 0, 0, 0, empty_string, 0, 0, 0xffffffff,
                                              0xffffffff, 0xffffffff);
            idx = idx + 1;
            row_entry->hidden = 1;
        } while (idx < 0xf);
    } else {
        DAT_007196a0 = 0;
        if (server_list_thread == 0) {
            master_server_process_pending_requests();
        }
        locked = server_list_mutex_try_lock(0x1e);
        bVar11 = (locked != 0);
        if (locked != 0) {
            if (server_browser_query_pending == 0) {
                if (0x32 < locked->pending_count ||
                    (0 < locked->pending_count && locked->result_count < 100)) {
                    server_browser_result_array_sort();
                }
            } else {
                server_browser_query_results_ingest();
            }
            player_count = locked->result_count;
            idx = 0;
            scan_index = server_list_scroll_offset;
            do {
                if (scan_index < 0 || player_count <= scan_index) {
                    entry = 0;
                } else {
                    entry = locked->list[scan_index];
                }
                server_browser_list_row_gather(col_headers[idx], 0, entry);
                if (server_browser_selected_index == scan_index && server_browser_player_list_ready == 0) {
                    if (entry == 0) {
                        server_browser_selected_index = -1;
                        server_browser_last_click_ms = 0;
                        server_list_reset();
                    } else {
                        probe = SBServerHasFullKeys(entry);
                        if (probe != 0) {
                            server_browser_player_list_ready = 1;
                            server_browser_player_list_populate(entry);
                            server_browser_selected_variant_description_build(entry);
                        }
                    }
                }
                idx = idx + 1;
                scan_index = scan_index + 1;
            } while (idx < 0xf);
            if (server_list_thread != 0) {
                ReleaseMutex(server_list_mutex->handle);
            }
        }
        engine_state = ServerBrowserState(master_server_query_engine);
        if (engine_state == 2 || engine_state == 1) {
            status_sub->value = 2;
        } else {
            status_sub->value = 1;
        }
        if (server_browser_selected_index == -1 || server_browser_join_requested == 0) {
            status_leaf->hidden = 1;
            *(uint32_t *)&status_leaf->alpha = 0x3eaa7efa;
        } else {
            status_leaf->hidden = 0;
            *(uint32_t *)&status_leaf->alpha = 0x3f800000;
        }

        w_iter = browser_widget->status_root->first_child;
        if (bVar11) {
            label = heap_reallocate(0, 0x40, widget_memory_pool);
            w_iter->label_text = (wchar_t *)label;
            if (label != 0) {
                join_game_ticker_string_copy(scratch_80, 0x40, 6);
                string_format_wide_va_bounded(0x1f, w_iter->label_text, L"%s %d", scratch_80, player_count);
                *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
            }
        }
        w_iter = w_iter->next_sibling;
        if (bVar11) {
            label = heap_reallocate(0, 0x40, widget_memory_pool);
            w_iter->label_text = (wchar_t *)label;
            if (label != 0) {
                join_game_ticker_string_copy(scratch_80, 0x40, 7);
                string_format_wide_va_bounded(0x1f, w_iter->label_text, L"%s %d", scratch_80, server_browser_total_players);
                *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
            }
        }
        w_iter = w_iter->next_sibling;
        if (bVar11) {
            label = heap_reallocate(0, 0x40, widget_memory_pool);
            w_iter->label_text = (wchar_t *)label;
            if (label != 0) {
                if (player_count < 1) {
                    w_iter->visible = 0;
                } else {
                    page_count = player_count / 0xf + 1;
                    current_page = (server_list_scroll_offset + 0xe) / 0xf + 1;
                    if (current_page < 1) {
                        current_page = 1;
                    } else if (page_count < current_page) {
                        current_page = page_count;
                    }
                    join_game_ticker_string_copy(scratch_80, 0x40, 8);
                    string_format_wide_va_bounded(0x1f, w_iter->label_text, L"%s %d/%d", scratch_80, current_page, page_count);
                    *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
                    w_iter->visible = 1;
                }
            }
        }
    }

    if (DAT_00719484 < 0) {
        DAT_00719484 = DAT_00719484 + 4;
        if (DAT_00719484 < 1) {
            goto scroll_fade_settled;
        }
    } else if (DAT_00719484 < 1 || (DAT_00719484 = DAT_00719484 - 4, -1 < DAT_00719484)) {
        goto scroll_fade_settled;
    }
    DAT_00719484 = 0;
scroll_fade_settled:
    if (server_list_scroll_offset < 1 || player_count < 1) {
        bVar11 = 0;
    } else {
        bVar11 = 1;
    }
    bVar12 = DAT_00719484 < 0;
    w_iter = up_arrow->parent->selected_child;
    if (bVar11) {
        up_arrow->hidden = 0;
        *(uint32_t *)&up_arrow->alpha = 0x3f800000;
        up_arrow->highlight_flag = (w_iter == up_arrow);
    } else {
        up_arrow->hidden = 1;
        up_arrow->highlight_flag = 0;
        *(uint32_t *)&up_arrow->alpha = 0x3eaa7efa;
    }
    if (bVar12) {
        up_arrow->first_child->highlight_flag = 1;
    } else {
        up_arrow->first_child->highlight_flag = 0;
    }
    if (server_list_scroll_offset + 0xf < player_count && 0 < player_count) {
        bVar11 = 1;
    } else {
        bVar11 = 0;
    }
    w_iter = down_arrow->parent->selected_child;
    if (bVar11) {
        down_arrow->hidden = 0;
        *(uint32_t *)&down_arrow->alpha = 0x3f800000;
        down_arrow->highlight_flag = (w_iter == down_arrow);
    } else {
        down_arrow->hidden = 1;
        down_arrow->highlight_flag = 0;
        *(uint32_t *)&down_arrow->alpha = 0x3eaa7efa;
    }
    if (DAT_00719484 < 1) {
        down_arrow->first_child->highlight_flag = 0;
    } else {
        down_arrow->first_child->highlight_flag = 1;
    }

    password_panel = browser_widget->parent->first_child->next_sibling->next_sibling->next_sibling;
    switch (master_server_last_result) {
    case 1:
    case 2:
    case 3:
    case 4:
        password_panel->hidden = 0;
        password_panel->visible = 1;
        password_panel->parent->selected_child = password_panel;
        password_panel->first_child->first_child->value = 0x28;
        /* fallthrough */
    case 5:
    case 6:
        master_server_request_flags = master_server_request_flags | 4;
        /* fallthrough */
    default:
        master_server_last_result = 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if (DAT_006b5e88 <= (uint32_t)(now_ms - master_server_connection_last_tick_ms)) {
            master_server_connection_last_tick_ms = now_ms;
            ticker_text_buffer_advance((uint8_t *)w17, &server_browser_player_ticker);
            ticker_text_buffer_advance((uint8_t *)w18, &server_browser_variant_ticker);
        }
        if (DAT_006953fc < now_ms) {
            if (server_browser_require_valid_entry == 0 && server_browser_selected_index == -1) {
                master_server_list_refresh_request();
            }
            DAT_006953fc = now_ms + 10000;
        }
        autopatch_slot = DAT_00695420;
        if (DAT_00695420 != -1) {
            state = -1;
            if (-1 < DAT_00695420 && DAT_00695420 < 2) {
                state = autopatch_download_slots[DAT_00695420].state;
            }
            switch (state) {
            case 4:
                got_result = autopatch_download_get_result(&result_a, &result_b);
                if (got_result != 0) {
                    if (result_a == 0 || result_b == 0) {
                        join_game_ticker_string_copy(DAT_00719498, 0x100, 5);
                        autopatch_slot = DAT_00695420;
                    } else {
                        // 0x4b88bc: mov edi,0x200 / mov eax,0x719498, with ebx = result_a
                        string_convert_ascii_to_unicode(DAT_00719498, 0x200, (const char *)(uintptr_t)result_a);
                        DAT_00719696 = 0;
                    }
                    if (server_browser_selected_index == -1) {
                        server_browser_ui_refresh();
                        autopatch_slot = DAT_00695420;
                    }
                }
                if (-1 < autopatch_slot && autopatch_slot < 2 &&
                    autopatch_download_slots[autopatch_slot].request_id != -1) {
                    *((uint8_t *)&autopatch_download_slots[autopatch_slot] + 0x11) = 1;
                }
                DAT_00695420 = -1;
                DAT_00719698 = 3;
                return 1;
            case -1:
            case 0:
            case 5:
                if (-1 < DAT_00695420 && DAT_00695420 < 2 &&
                    autopatch_download_slots[DAT_00695420].request_id != -1) {
                    *((uint8_t *)&autopatch_download_slots[DAT_00695420] + 0x11) = 1;
                }
                DAT_00695420 = -1;
                DAT_00719698 = 2;
                DAT_00719498[0] = 0;
                tag_idx = tag_lookup(
                    "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels");
                if (tag_idx != -1) {
                    src = text_string_list_get_string(tag_idx, 0);
                    wcsncpy(DAT_00719498, src, 0xff);
                    DAT_00719696 = 0;
                }
                if (server_browser_selected_index == -1) {
                    server_browser_ui_refresh();
                }
                break;
            }
        }
        return 1;
    }
}

#if 0
Original Ghidra decompilation (0x4b80f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 join_game_server_browser_tick(int param_1)

{
  int iVar1;
  DWORD DVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  int iVar6;
  uint uVar7;
  undefined4 uVar8;
  wchar_t *_Source;
  undefined4 *puVar9;
  int iVar10;
  bool bVar11;
  bool bVar12;
  undefined8 uVar13;
  int local_e0;
  int local_d8;
  int local_d4;
  int local_d0;
  int local_cc;
  LARGE_INTEGER local_c8;
  int local_c0;
  DWORD aDStack_bc [15];
  undefined1 local_80 [128];

  if (DAT_00719450 != 0) {
    if ((DAT_00719454 == '\0') || (DAT_00719458 != 0)) {
      cVar3 = FUN_004ba320();
      if (cVar3 != '\0') {
        widget_close_all();
        return 1;
      }
    }
    else if (DAT_007193bc._2_1_ != '\0') {
      iVar10 = *(int *)(*(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x30) + 0x34) + 0x2c) + 0x2c)
                       + 0x2c);
      *(undefined1 *)(iVar10 + 0x10) = 1;
      *(undefined1 *)(iVar10 + 0x12) = 0;
      *(int *)(*(int *)(iVar10 + 0x30) + 0x38) = iVar10;
      *(undefined2 *)(*(int *)(*(int *)(iVar10 + 0x34) + 0x34) + 0x40) = 0x2a;
    }
    DAT_00719458 = 0;
    DAT_00719454 = '\0';
    DAT_00719450 = 0;
  }
  puVar9 = (undefined4 *)0x0;
  if ((DAT_006b1804 != 0) && (puVar9 = &DAT_006b1828, DAT_006b15f9 == '\0')) {
    puVar9 = &DAT_006b180c;
  }
  cVar3 = FUN_00490b50();
  if (cVar3 == '\x01') {
    server_list_scroll_page_up();
    widget_play_sound_effect();
  }
  cVar3 = FUN_00490b50();
  if (cVar3 == '\x01') {
    server_list_scroll_page_up();
    widget_play_sound_effect();
  }
  cVar3 = FUN_00490b50();
  if (cVar3 == '\x01') {
    server_list_scroll_page_down();
    widget_play_sound_effect();
  }
  cVar3 = FUN_00490b50();
  if (cVar3 == '\x01') {
    server_list_scroll_page_down();
    widget_play_sound_effect();
  }
  if (puVar9 != (undefined4 *)0x0) {
    if ((int)puVar9[2] < 0) {
      server_list_scroll_page_up();
    }
    else if (0 < (int)puVar9[2]) {
      server_list_scroll_page_down();
    }
  }
  iVar10 = *(int *)(*(int *)(param_1 + 0x34) + 0x34);
  iVar6 = *(int *)(*(int *)(param_1 + 0x34) + 0x2c);
  local_c0 = *(int *)(iVar6 + 0x2c);
  local_c8.s.LowPart = *(DWORD *)(local_c0 + 0x2c);
  local_e0 = 0;
  iVar4 = 0;
  do {
    aDStack_bc[iVar4] = local_c8.s.LowPart;
    local_c8.s.LowPart = *(DWORD *)(local_c8.s.LowPart + 0x2c);
    iVar4 = iVar4 + 1;
  } while (iVar4 < 0xf);
  local_d4 = *(int *)(local_c8.s.LowPart + 0x2c);
  local_d8 = *(int *)(local_d4 + 0x2c);
  iVar4 = *(int *)(local_d8 + 0x2c);
  local_d0 = *(int *)(*(int *)(iVar4 + 0x34) + 0x2c);
  iVar1 = *(int *)(*(int *)(local_d0 + 0x2c) + 0x2c);
  *(undefined1 *)(local_d8 + 0x12) = 1;
  *(undefined1 *)(local_d4 + 0x12) = 1;
  *(undefined2 *)(*(int *)(iVar6 + 0x34) + 0x58) = 1;
  if (*(int *)(param_1 + 0x38) == iVar4) {
    if (*(int *)(iVar4 + 0x38) == 0) {
      *(undefined4 *)(iVar4 + 0x38) = *(undefined4 *)(iVar4 + 0x34);
    }
  }
  else {
    *(undefined2 *)(iVar4 + 0x58) = 0;
    for (iVar4 = *(int *)(iVar4 + 0x34); iVar4 != 0; iVar4 = *(int *)(iVar4 + 0x2c)) {
      *(undefined2 *)(iVar4 + 0x58) = 0;
    }
  }
  iVar4 = 0;
  if (*(int *)(param_1 + 0x38) == iVar6) {
    if (*(int *)(iVar6 + 0x38) == 0) {
      *(undefined4 *)(iVar6 + 0x38) = *(undefined4 *)(iVar6 + 0x34);
    }
  }
  else if (*(int *)(iVar6 + 0x38) != 0) {
    *(undefined4 *)(iVar6 + 0x38) = 0;
  }
  iVar6 = *(int *)(iVar10 + 0x30);
  if ((*(int *)(param_1 + 0x38) == iVar6) && (*(int *)(iVar6 + 0x38) == 0)) {
    *(int *)(iVar6 + 0x38) = iVar10;
  }
  FUN_004b7f70();
  if (DAT_0071946c == 0) {
    do {
      DVar2 = aDStack_bc[iVar4];
      *(ushort *)(DVar2 + 0x58) = (ushort)(*(DWORD *)(*(int *)(DVar2 + 0x30) + 0x38) == DVar2);
      FUN_004b67e0(&DAT_0065512c,&DAT_00660c34,&DAT_0065512c,0,0xffffffff,0xffffffff,0xffffffff);
      iVar4 = iVar4 + 1;
      *(undefined1 *)(DVar2 + 0x12) = 1;
    } while (iVar4 < 0xf);
  }
  else {
    DAT_007196a0 = 0;
    if (DAT_007196ac == 0) {
      FUN_004b5d70();
    }
    piVar5 = server_list_mutex_try_lock(0x1e);
    bVar11 = piVar5 != (int *)0x0;
    if (piVar5 != (int *)0x0) {
      if (DAT_0071948a == '\0') {
        if ((0x32 < piVar5[3]) || ((0 < piVar5[3] && (piVar5[1] < 100)))) {
          FUN_004ba9c0();
        }
      }
      else {
        server_browser_query_results_ingest();
      }
      local_e0 = piVar5[1];
      local_cc = 0;
      iVar10 = DAT_00719478;
      do {
        iVar6 = local_cc;
        if ((iVar10 < 0) || (local_e0 <= iVar10)) {
          iVar4 = 0;
        }
        else {
          iVar4 = *(int *)(*piVar5 + iVar10 * 4);
        }
        FUN_004b69c0(aDStack_bc[local_cc]);
        if ((DAT_006953f4 == iVar10) && (DAT_00719481 == '\0')) {
          if (iVar4 == 0) {
            DAT_006953f4 = -1;
            DAT_0071947c = 0;
            server_list_reset();
          }
          else {
            iVar4 = FUN_006175d0(iVar4);
            if (iVar4 != 0) {
              DAT_00719481 = '\x01';
              server_browser_player_list_populate();
              FUN_004b74e0();
              iVar6 = local_cc;
            }
          }
        }
        local_cc = iVar6 + 1;
        iVar10 = iVar10 + 1;
      } while (local_cc < 0xf);
      if (DAT_007196ac != 0) {
        ReleaseMutex((HANDLE)*DAT_007196a8);
      }
    }
    iVar10 = FUN_00616ff0(DAT_0071946c);
    if ((iVar10 == 2) || (iVar10 == 1)) {
      *(undefined2 *)(local_d0 + 0x40) = 2;
    }
    else {
      *(undefined2 *)(local_d0 + 0x40) = 1;
    }
    if ((DAT_006953f4 == -1) || (DAT_00719491 == '\0')) {
      *(undefined1 *)(iVar1 + 0x12) = 1;
      *(undefined4 *)(iVar1 + 0x24) = 0x3eaa7efa;
    }
    else {
      *(undefined1 *)(iVar1 + 0x12) = 0;
      *(undefined4 *)(iVar1 + 0x24) = 0x3f800000;
    }
    iVar10 = *(int *)(*(int *)(param_1 + 0x4c) + 0x34);
    if (bVar11) {
      iVar6 = heap_reallocate(0x40);
      *(int *)(iVar10 + 0x3c) = iVar6;
      if (iVar6 != 0) {
        FUN_004b6160(6);
        string_format_wide_va_bounded(*(undefined4 *)(iVar10 + 0x3c),L"%s %d",local_80,local_e0);
        *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x3e) = 0;
      }
    }
    iVar10 = *(int *)(iVar10 + 0x2c);
    if (bVar11) {
      iVar6 = heap_reallocate(0x40);
      *(int *)(iVar10 + 0x3c) = iVar6;
      if (iVar6 != 0) {
        FUN_004b6160(7);
        string_format_wide_va_bounded(*(undefined4 *)(iVar10 + 0x3c),L"%s %d",local_80,DAT_00719474)
        ;
        *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x3e) = 0;
      }
    }
    iVar10 = *(int *)(iVar10 + 0x2c);
    if (bVar11) {
      iVar6 = heap_reallocate(0x40);
      *(int *)(iVar10 + 0x3c) = iVar6;
      if (iVar6 != 0) {
        if (local_e0 < 1) {
          *(undefined1 *)(iVar10 + 0x10) = 0;
        }
        else {
          iVar6 = local_e0 / 0xf + 1;
          iVar4 = (DAT_00719478 + 0xe) / 0xf + 1;
          if (iVar4 < 1) {
            iVar4 = 1;
          }
          else if (iVar6 < iVar4) {
            iVar4 = iVar6;
          }
          FUN_004b6160(8);
          string_format_wide_va_bounded
                    (*(undefined4 *)(iVar10 + 0x3c),L"%s %d/%d",local_80,iVar4,iVar6);
          *(undefined2 *)(*(int *)(iVar10 + 0x3c) + 0x3e) = 0;
          *(undefined1 *)(iVar10 + 0x10) = 1;
        }
      }
    }
  }
  iVar10 = DAT_00719478;
  if (DAT_00719484 < 0) {
    DAT_00719484 = DAT_00719484 + 4;
    if (DAT_00719484 < 1) goto LAB_004b8692;
  }
  else if ((DAT_00719484 < 1) || (DAT_00719484 = DAT_00719484 + -4, -1 < DAT_00719484))
  goto LAB_004b8692;
  DAT_00719484 = 0;
LAB_004b8692:
  iVar6 = DAT_00719484;
  if ((DAT_00719478 < 1) || (local_e0 < 1)) {
    bVar11 = false;
  }
  else {
    bVar11 = true;
  }
  bVar12 = DAT_00719484 < 0;
  iVar4 = *(int *)(*(int *)(local_c0 + 0x30) + 0x38);
  if (bVar11) {
    *(undefined1 *)(local_c0 + 0x12) = 0;
    *(undefined4 *)(local_c0 + 0x24) = 0x3f800000;
    *(ushort *)(local_c0 + 0x58) = (ushort)(iVar4 == local_c0);
  }
  else {
    *(undefined1 *)(local_c0 + 0x12) = 1;
    *(undefined2 *)(local_c0 + 0x58) = 0;
    *(undefined4 *)(local_c0 + 0x24) = 0x3eaa7efa;
  }
  if (bVar12) {
    *(undefined2 *)(*(int *)(local_c0 + 0x34) + 0x58) = 1;
  }
  else {
    *(undefined2 *)(*(int *)(local_c0 + 0x34) + 0x58) = 0;
  }
  if ((iVar10 + 0xf < local_e0) && (0 < local_e0)) {
    bVar11 = true;
  }
  else {
    bVar11 = false;
  }
  DVar2 = *(DWORD *)(*(int *)(local_c8.s.LowPart + 0x30) + 0x38);
  if (bVar11) {
    *(undefined1 *)(local_c8.s.LowPart + 0x12) = 0;
    *(undefined4 *)(local_c8.s.LowPart + 0x24) = 0x3f800000;
    *(ushort *)(local_c8.s.LowPart + 0x58) = (ushort)(DVar2 == local_c8.s.LowPart);
  }
  else {
    *(undefined1 *)(local_c8.s.LowPart + 0x12) = 1;
    *(undefined2 *)(local_c8.s.LowPart + 0x58) = 0;
    *(undefined4 *)(local_c8.s.LowPart + 0x24) = 0x3eaa7efa;
  }
  if (iVar6 < 1) {
    *(undefined2 *)(*(int *)(local_c8.s.LowPart + 0x34) + 0x58) = 0;
  }
  else {
    *(undefined2 *)(*(int *)(local_c8.s.LowPart + 0x34) + 0x58) = 1;
  }
  iVar10 = *(int *)(*(int *)(*(int *)(*(int *)(*(int *)(param_1 + 0x30) + 0x34) + 0x2c) + 0x2c) +
                   0x2c);
  switch(DAT_007196a4) {
  case 1:
  case 2:
  case 3:
  case 4:
    *(undefined1 *)(iVar10 + 0x12) = 0;
    *(undefined1 *)(iVar10 + 0x10) = 1;
    *(int *)(*(int *)(iVar10 + 0x30) + 0x38) = iVar10;
    *(undefined2 *)(*(int *)(*(int *)(iVar10 + 0x34) + 0x34) + 0x40) = 0x28;
  case 5:
  case 6:
    DAT_0071969c = DAT_0071969c | 4;
  default:
    DAT_007196a4 = 0;
    QueryPerformanceCounter(&local_c8);
    uVar13 = __allmul(local_c8.s.LowPart,local_c8.s.HighPart,1000,0);
    uVar7 = __alldiv(uVar13,DAT_006ac8f8,DAT_006ac8fc);
    if (DAT_006b5e88 <= uVar7 - DAT_007196b8) {
      DAT_007196b8 = uVar7;
      FUN_004b8b40();
      FUN_004b8b40();
    }
    if (DAT_006953fc < uVar7) {
      if ((DAT_006953f0 == '\0') && (DAT_006953f4 == -1)) {
        master_server_list_refresh_request();
      }
      DAT_006953fc = uVar7 + 10000;
    }
    iVar10 = DAT_00695420;
    if (DAT_00695420 != -1) {
      uVar8 = 0xffffffff;
      if ((-1 < DAT_00695420) && (DAT_00695420 < 2)) {
        uVar8 = (&DAT_006ef940)[DAT_00695420 * 5];
      }
      switch(uVar8) {
      case 4:
        cVar3 = autopatch_download_get_result(&local_d8,&local_d4);
        if (cVar3 != '\0') {
          if ((local_d8 == 0) || (local_d4 == 0)) {
            FUN_004b6160(5);
            iVar10 = DAT_00695420;
          }
          else {
            FUN_00557990();
            _DAT_00719696 = 0;
          }
          if (DAT_006953f4 == -1) {
            FUN_004b73a0();
            iVar10 = DAT_00695420;
          }
        }
        if (((-1 < iVar10) && (iVar10 < 2)) && ((&DAT_006ef93c)[iVar10 * 5] != -1)) {
          *(undefined1 *)((int)&DAT_006ef94c + iVar10 * 0x14 + 1) = 1;
        }
        DAT_00695420 = 0xffffffff;
        DAT_00719698 = 3;
        return 1;
      case 0xffffffff:
      case 0:
      case 5:
        if (((-1 < DAT_00695420) && (DAT_00695420 < 2)) && ((&DAT_006ef93c)[DAT_00695420 * 5] != -1)
           ) {
          *(undefined1 *)((int)&DAT_006ef94c + DAT_00695420 * 0x14 + 1) = 1;
        }
        DAT_00695420 = -1;
        DAT_00719698 = 2;
        DAT_00719498 = 0;
        iVar10 = tag_lookup(
                           "ui\\shell\\main_menu\\multiplayer_type_select\\join_game\\join_game_ticker_labels"
                           );
        if (iVar10 != -1) {
          _Source = (wchar_t *)text_string_list_get_string();
          _wcsncpy(&DAT_00719498,_Source,0xff);
          _DAT_00719696 = 0;
        }
        if (DAT_006953f4 == -1) {
          FUN_004b73a0();
        }
      }
    }
    return 1;
  }
}
#endif
