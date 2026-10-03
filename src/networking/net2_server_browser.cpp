/**
 * @file src/networking/net2_server_browser.cpp
 * Server browser filters, sorting, list rows and join latch.
 */
#include "tags.h"
#include "halo/core/cstring.hpp"
#include "halo/core/ui_tag_paths.hpp"
#include "halo/game/variant_flags.hpp"
#include "halo/core/tag_groups.hpp"
#include "halo/game/constants.hpp"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include "crt.h"
#include "win32.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include "halo/networking/browser_state.hpp"
#include "halo/networking/net2_server_browser.hpp"
#include "halo/networking/server_sort.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/input/api.hpp"
#include "halo/cseries/api.hpp"
#include "halo/saved_games/api.hpp"
#include "halo/shell/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/game/api.hpp"
#include "halo/interface/api.hpp"

extern "C" {
extern uint8_t playlist_profiles_need_defaults;
extern void playlist_profile_create_default_profiles_on_disk(void);
extern void saved_game_enumerate_by_type(int32_t type, int32_t *out_ids, int32_t flag);
extern uint8_t saved_game_get_variant(int32_t saved_game_id, uint16_t *out_name);
extern void * console_color_00685214;
extern void * actor_mode_default_look_weights;
extern void * server_browser_join_target;
extern uint8_t server_browser_join_target_has_password;
extern uint16_t network_join_target_address[128];
extern int32_t mouse_device;
extern int32_t mouse_neutral_state[3];
extern int32_t live_mouse_state[3];
extern void * master_server_query_engine;
extern network_thread_record * server_list_thread;
extern uint8_t server_browser_query_pending;
extern int32_t server_list_scroll_offset;
extern int32_t server_browser_selected_index;
extern uint8_t server_browser_player_list_ready;
extern int32_t server_browser_last_click_ms;
extern network_mutex_record * server_list_mutex;
extern uint8_t server_browser_join_requested;
extern int32_t master_server_last_result;
extern uint32_t master_server_request_flags;
extern int32_t master_server_connection_last_tick_ms;
extern autopatch_download_slot autopatch_download_slots[k_network_autopatch_download_slots];
extern uint8_t server_browser_require_valid_entry;
extern int32_t server_browser_total_players;
extern char k_empty_string[];
extern wchar_t empty_string[];
extern void widget_close_all(void);
extern uint8_t input_get_key_state(int16_t key_index);
extern void widget_play_sound_effect(int16_t effect_id);
extern int32_t SBServerHasFullKeys(void *entry);
extern int32_t ServerBrowserState(void *engine);
extern ticker_text_buffer server_browser_player_ticker;
extern ticker_text_buffer server_browser_variant_ticker;
extern wchar_t string_widen_scratch[0x400];
extern char server_browser_custom_options_text[];
extern int32_t sprintf(char *buffer, const char *format, ...);
extern int32_t sscanf(const char *buffer, const char *format, ...);
extern uint8_t server_browser_sort_column;
extern uint8_t server_browser_sort_ascending;
extern uint8_t server_browser_allow_password;
extern uint8_t server_browser_filter_dedicated_only;
extern uint8_t server_browser_filter_classic_only;
extern uint8_t server_browser_allow_empty;
extern uint8_t server_browser_allow_full;
extern uint8_t server_browser_filter_allow_unknown_map;
extern uint8_t server_browser_filter_gametype;
extern uint8_t server_browser_filter_teamplay;
extern uint8_t server_browser_filter_ping_limit_index;
extern uint8_t server_browser_filter_panel_mode;
extern int32_t server_browser_query_elapsed_ms;
extern server_list_globals server_list;
extern int32_t network_host_edit_field_00719410;
extern int32_t SBServerGetBoolValue(void *entry, const char *key, int32_t default_value);
extern char * SBServerGetStringValue(void *entry, const char *key, const char *default_value);
extern int32_t SBServerGetIntValue(void *entry, const char *key, int32_t default_value);
extern int32_t SBServerGetPing(void *entry);
extern const wchar_t PTR_s_parameter_handles_0063fff0_0x35_006607a0[];
extern uint8_t server_browser_initialized;
extern uint8_t network_session_start_host_name[];
extern uint8_t network_session_start_map_name[];
extern uint8_t profile_globals_block[];
extern void * ServerBrowserNew(void *a, void *b, void *c, int32_t d, int32_t e, int32_t f,
                           void *callback, int32_t h);
extern wchar_t hud_text_unbound[];
extern char * SBServerGetPlayerStringValue(void *entry, int32_t index, const char *key, const char *default_value);
extern int32_t ServerBrowserGetServer(void *query_engine, int32_t index);
extern int32_t ServerBrowserCount(void *engine);
extern int32_t SBServerHasBasicKeys(int32_t record);
extern uint8_t server_browser_skip_reselect;
extern int32_t server_browser_ping_limits[];
extern int32_t map_list_find_known_map_index(const char *mapname);

}

static network_ui_widget *ui_widget_find_control(network_ui_widget *container)
{
    network_ui_widget *w;
    for (w = container->first_child; w != 0 && w->type != 2; w = w->next_sibling) {
    }
    return w;
}
static int32_t address_order(const void *a, const void *b)
{
    if (server_browser_sort_ascending != 0) {
        return (uint32_t)b > (uint32_t)a ? -1 : ((uint32_t)b < (uint32_t)a ? 1 : 0);
    }
    return (uint32_t)b > (uint32_t)a ? 1 : ((uint32_t)b < (uint32_t)a ? -1 : 0);
}
static int32_t key_order(const void *a, const void *b, const char *key)
{
    const char *string_b = SBServerGetStringValue(*(void **)b, key, "");
    const char *string_a = SBServerGetStringValue(*(void **)a, key, "");
    int32_t result = _stricmp(string_a, string_b);

    return server_browser_sort_ascending != 0 ? result : -result;
}

namespace halo::networking {

void ServerBrowser::matching_substring(uint32_t argument_count, char **arguments)
{
    uint16_t filter[32];
    int32_t saved_game_ids[100];
    uint16_t saved_game_capacity = 100;
    int32_t i;

    filter[0] = 0;
    if (0 < (int32_t)argument_count) {
        uint16_t *p;
        halo::text::string_format_wide_va_bounded(0x1f, filter, reinterpret_cast<const uint16_t *>(L"%S"), arguments[0]);
        for (p = filter; *p != 0; p = p + 1) {
            *p = towlower(*p);
        }
    }
    halo::interface::chimera__console_out((ColorARGB *)console_color_00685214, halo::mutable_literal("Game types matching substring \"%ls\" :"), filter);
    if (playlist_profiles_need_defaults == 1) {
        halo::saved_games::playlist_profile_create_default_profiles_on_disk();
        playlist_profiles_need_defaults = 0;
    }
    for (i = 0; i < 100; i = i + 1) {
        saved_game_ids[i] = -1;
    }
    halo::saved_games::saved_game_enumerate_by_type(1, saved_game_ids, 1, &saved_game_capacity);
    i = 0;
    do {
        char line[256];
        int32_t on_line = 0;

        line[0] = 0;
        while (i <= 99 && on_line < 2) {
            if (saved_game_ids[i] == -1) {
                halo::game::game_engine_apply_current_custom_variant();
            } else {
                uint16_t variant_name[64];
                if (halo::saved_games::saved_game_get_variant(saved_game_ids[i], (game_variant *)variant_name) != 0) {
                    uint16_t lowered[64];
                    wcsncpy((wchar_t *)lowered, (const wchar_t *)variant_name, 0x3f);
                    lowered[0x3f] = 0;
                    for (uint16_t *p = lowered; *p != 0; p = p + 1) {
                        *p = towlower(*p);
                    }
                    if (filter[0] == 0 || wcsstr((wchar_t *)lowered, (const wchar_t *)filter) != 0) {
                        char formatted[64];
                        sprintf(formatted, "%-36ls ", variant_name);
                        strcat(line, formatted);
                        on_line = on_line + 1;
                    }
                }
            }
            i = i + 1;
        }
        if (line[0] != 0) {
            halo::interface::chimera__console_out((ColorARGB *)actor_mode_default_look_weights, line);
        }
        if (99 < i) {
            return;
        }
    } while (1);
}

int32_t ServerBrowser::server_browser_tick(network_ui_widget *browser_widget)
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
        if (server_browser_join_target_has_password == 0 || network_join_target_address[0] != 0) {
            clicked = (uint8_t)halo::networking::network_join_request_resolve_host();
            if (clicked != 0) {
                halo::interface::widget_close_all();
                return 1;
            }
        } else if (halo::interface::globals().virtual_keyboard.committed != 0) {
            password_panel = browser_widget->parent->first_child->next_sibling->next_sibling->next_sibling;
            password_panel->visible = 1;
            password_panel->hidden = 0;
            password_panel->parent->selected_child = password_panel;
            password_panel->first_child->first_child->value = 0x2a;
        }
        network_join_target_address[0] = 0;
        server_browser_join_target_has_password = 0;
        server_browser_join_target = 0;
    }

    scroll_target = 0;
    if (mouse_device != 0) {
        scroll_target = mouse_neutral_state;
        if (halo::input::globals().suppressed == 0) {
            scroll_target = live_mouse_state;
        }
    }

    if (halo::input::input_get_key_state(0x53) == 1) {
        halo::networking::server_list_scroll_page_up(0);
        halo::interface::widget_play_sound_effect(2);
    }
    if (halo::input::input_get_key_state(0x52) == 1) {
        halo::networking::server_list_scroll_page_up(1);
        halo::interface::widget_play_sound_effect(2);
    }
    if (halo::input::input_get_key_state(0x56) == 1) {
        halo::networking::server_list_scroll_page_down(0);
        halo::interface::widget_play_sound_effect(2);
    }
    if (halo::input::input_get_key_state(0x55) == 1) {
        halo::networking::server_list_scroll_page_down(1);
        halo::interface::widget_play_sound_effect(2);
    }
    if (scroll_target != 0) {
        if (scroll_target[2] < 0) {
            halo::networking::server_list_scroll_page_up(0);
        } else if (0 < scroll_target[2]) {
            halo::networking::server_list_scroll_page_down(0);
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

    halo::networking::server_browser_filter_headers_refresh(sort_widget);

    player_count = 0;
    if (master_server_query_engine == 0) {
        idx = 0;
        do {
            row_entry = col_headers[idx];
            row_entry->highlight_flag = (row_entry->parent->selected_child == row_entry);
            halo::networking::server_browser_list_row_populate(row_entry, 0, 0, k_empty_string, empty_string, k_empty_string, 0,
                                              halo::k_dword_none, halo::k_dword_none, halo::k_dword_none);
            idx = idx + 1;
            row_entry->hidden = 1;
        } while (idx < 0xf);
    } else {
        browser_state::tick_reset_flag = 0;
        if (server_list_thread == 0) {
            halo::networking::master_server_process_pending_requests();
        }
        locked = halo::networking::server_list_mutex_try_lock(0x1e);
        bVar11 = (locked != 0);
        if (locked != 0) {
            if (server_browser_query_pending == 0) {
                if (0x32 < locked->pending_count ||
                    (0 < locked->pending_count && locked->result_count < 100)) {
                    halo::networking::server_browser_result_array_sort(locked);
                }
            } else {
                halo::networking::server_browser_query_results_ingest(locked);
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
                halo::networking::server_browser_list_row_gather(col_headers[idx], (uint8_t)(server_browser_selected_index == scan_index), entry);
                if (server_browser_selected_index == scan_index && server_browser_player_list_ready == 0) {
                    if (entry == 0) {
                        server_browser_selected_index = -1;
                        server_browser_last_click_ms = 0;
                        halo::networking::server_list_reset((uint8_t *)locked);
                    } else {
                        probe = SBServerHasFullKeys(entry);
                        if (probe != 0) {
                            server_browser_player_list_ready = 1;
                            halo::networking::server_browser_player_list_populate(entry);
                            halo::networking::server_browser_selected_variant_description_build(entry);
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
            label = halo::memory::heap_reallocate(w_iter->label_text, 0x40, halo::interface::globals().widget_memory_pool);
            w_iter->label_text = (uint16_t *)label;
            if (label != 0) {
                halo::networking::join_game_ticker_string_copy((uint16_t *)scratch_80, 0x40, 6);
                halo::text::string_format_wide_va_bounded(0x1f, w_iter->label_text, (const uint16_t *)L"%s %d", scratch_80, player_count);
                *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
            }
        }
        w_iter = w_iter->next_sibling;
        if (bVar11) {
            label = halo::memory::heap_reallocate(w_iter->label_text, 0x40, halo::interface::globals().widget_memory_pool);
            w_iter->label_text = (uint16_t *)label;
            if (label != 0) {
                halo::networking::join_game_ticker_string_copy((uint16_t *)scratch_80, 0x40, 7);
                halo::text::string_format_wide_va_bounded(0x1f, w_iter->label_text, (const uint16_t *)L"%s %d", scratch_80, server_browser_total_players);
                *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
            }
        }
        w_iter = w_iter->next_sibling;
        if (bVar11) {
            label = halo::memory::heap_reallocate(w_iter->label_text, 0x40, halo::interface::globals().widget_memory_pool);
            w_iter->label_text = (uint16_t *)label;
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
                    halo::networking::join_game_ticker_string_copy((uint16_t *)scratch_80, 0x40, 8);
                    halo::text::string_format_wide_va_bounded(0x1f, w_iter->label_text, (const uint16_t *)L"%s %d/%d", scratch_80, current_page, page_count);
                    *(uint16_t *)((uint8_t *)w_iter->label_text + 0x3e) = 0;
                    w_iter->visible = 1;
                }
            }
        }
    }

    if (browser_state::scroll_arrow_flash < 0) {
        browser_state::scroll_arrow_flash = browser_state::scroll_arrow_flash + 4;
        if (browser_state::scroll_arrow_flash < 1) {
            goto scroll_fade_settled;
        }
    } else if (browser_state::scroll_arrow_flash < 1 || (browser_state::scroll_arrow_flash = browser_state::scroll_arrow_flash - 4, -1 < browser_state::scroll_arrow_flash)) {
        goto scroll_fade_settled;
    }
    browser_state::scroll_arrow_flash = 0;
scroll_fade_settled:
    if (server_list_scroll_offset < 1 || player_count < 1) {
        bVar11 = 0;
    } else {
        bVar11 = 1;
    }
    bVar12 = browser_state::scroll_arrow_flash < 0;
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
    if (browser_state::scroll_arrow_flash < 1) {
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

    case 5:
    case 6:
        master_server_request_flags = master_server_request_flags | 4;

    default:
        master_server_last_result = 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / halo::cseries::globals().performance_frequency);
        if ((uint32_t)server_browser_variant_ticker.scroll_delay_ms <= (uint32_t)(now_ms - master_server_connection_last_tick_ms)) {
            master_server_connection_last_tick_ms = now_ms;
            halo::networking::ticker_text_buffer_advance((uint8_t *)w17, &server_browser_player_ticker);
            halo::networking::ticker_text_buffer_advance((uint8_t *)w18, &server_browser_variant_ticker);
        }
        if ((uint32_t)browser_state::next_auto_refresh_ms < (uint32_t)now_ms) {
            if (server_browser_require_valid_entry == 0 && server_browser_selected_index == -1) {
                halo::networking::master_server_list_refresh_request();
            }
            browser_state::next_auto_refresh_ms = now_ms + 10000;
        }
        autopatch_slot = browser_state::motd_download_slot;
        if (browser_state::motd_download_slot != -1) {
            state = -1;
            if (-1 < browser_state::motd_download_slot && browser_state::motd_download_slot < 2) {
                state = autopatch_download_slots[browser_state::motd_download_slot].state;
            }
            switch (state) {
            case 4:
                got_result = halo::networking::autopatch_download_get_result((void **)&result_a, &result_b, autopatch_slot);
                if (got_result != 0) {
                    if (result_a == 0 || result_b == 0) {
                        halo::networking::join_game_ticker_string_copy((uint16_t *)browser_state::ticker_message, 0x100, 5);
                        autopatch_slot = browser_state::motd_download_slot;
                    } else {

                        halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(browser_state::ticker_message), 0x200, (const char *)(uintptr_t)result_a);
                        browser_state::ticker_message_terminator = 0;
                    }
                    if (server_browser_selected_index == -1) {
                        halo::networking::server_browser_ui_refresh();
                        autopatch_slot = browser_state::motd_download_slot;
                    }
                }
                if (-1 < autopatch_slot && autopatch_slot < 2 &&
                    autopatch_download_slots[autopatch_slot].request_id != -1) {
                    *((uint8_t *)&autopatch_download_slots[autopatch_slot] + 0x11) = 1;
                }
                browser_state::motd_download_slot = -1;
                browser_state::motd_download_state = 3;
                return 1;
            case -1:
            case 0:
            case 5:
                if (-1 < browser_state::motd_download_slot && browser_state::motd_download_slot < 2 &&
                    autopatch_download_slots[browser_state::motd_download_slot].request_id != -1) {
                    *((uint8_t *)&autopatch_download_slots[browser_state::motd_download_slot] + 0x11) = 1;
                }
                browser_state::motd_download_slot = -1;
                browser_state::motd_download_state = 2;
                browser_state::ticker_message[0] = 0;
                tag_idx = halo::cache::tag_lookup(0x75737472, halo::tag_paths::join_game_ticker_labels);
                if (tag_idx != -1) {
                    src = halo::text::text_string_list_get_string(tag_idx, 5);
                    wcsncpy(browser_state::ticker_message, (const wchar_t *)src, 0xff);
                    browser_state::ticker_message_terminator = 0;
                }
                if (server_browser_selected_index == -1) {
                    halo::networking::server_browser_ui_refresh();
                }
                break;
            }
        }
        return 1;
    }
}

void ServerBrowser::ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index)
{
    int32_t tag_index;
    uint16_t *source;

    *buffer = 0;
    tag_index = halo::cache::tag_lookup(halo::groups::unicode_string_list,
        halo::tag_paths::join_game_ticker_labels);
    if (tag_index != -1) {
        source = halo::text::text_string_list_get_string(tag_index, (int16_t)string_index);
        wcsncpy((wchar_t *)buffer, (const wchar_t *)source, capacity - 1);
        buffer[capacity - 1] = 0;
    }
}

void ServerBrowser::map_list_matching_substring(uint32_t argument_count, char **arguments)
{
    char filter[64];
    int32_t i;

    filter[0] = 0;
    if (0 < (int32_t)argument_count) {
        char *p;
        strncpy(filter, arguments[0], 0x3f);
        filter[0x3f] = 0;
        for (p = filter; *p != 0; p = p + 1) {
            *p = (char)tolower((uint8_t)*p);
        }
    }
    halo::interface::chimera__console_out((ColorARGB *)console_color_00685214, halo::mutable_literal("Maps matching substring \"%s\" :"), filter);
    i = 0;
    while (i < halo::interface::globals().map_list_count) {
        char line[256];
        int32_t on_line = 0;

        line[0] = 0;
        while (i < halo::interface::globals().map_list_count && on_line < 2) {
            map_list_entry *entry = &halo::interface::globals().map_list[i];
            if (entry->cache_file_exists != 0 && entry->path != 0 &&
                (filter[0] == 0 || strstr(entry->path, filter) != 0)) {
                char formatted[64];
                sprintf(formatted, "%-36s ", entry->path);
                strcat(line, formatted);
                on_line = on_line + 1;
            }
            i = i + 1;
        }
        if (line[0] != 0) {
            halo::interface::chimera__console_out((ColorARGB *)actor_mode_default_look_weights, line);
        }
    }
}

void ServerBrowser::column_header_update(network_ui_widget *header, int32_t sort_direction)
{
    network_ui_widget *icon;
    network_ui_widget *up_arrow;
    network_ui_widget *down_arrow;

    icon = header->first_child;
    if (header->parent->selected_child == header) {
        header->selected_child = icon;
        header->highlight_flag = 1;
    } else {
        header->selected_child = 0;
        header->highlight_flag = 0;
    }
    *(uint32_t *)&icon->alpha = 0x3f000000;
    if (sort_direction == 0) {
        icon->visible = 0;
        return;
    }
    up_arrow = icon->first_child;
    icon->visible = 1;
    up_arrow->highlight_flag = 1;
    up_arrow->visible = (0 < sort_direction);
    down_arrow = up_arrow->next_sibling;
    down_arrow->highlight_flag = 1;
    down_arrow->visible = (sort_direction < 0);
}

char * ServerBrowser::custom_options_pack(server_browser_custom_options *options)
{
    uint32_t low;
    uint32_t high;
    uint32_t extra;

    low = 0x40000000;
    if (options->lives_per_round != 0) {
        if (options->lives_per_round == 1) {
            low = 0x40000001;
        } else if (options->lives_per_round == 3) {
            low = 0x40000002;
        } else if (options->lives_per_round == 5) {
            low = 0x40000003;
        }
    }
    if (options->health_bits != 0x3f000000) {
        if (options->health_bits == halo::game::k_float_one_bits) {
            low = low | 4;
        } else if (options->health_bits == 0x3fc00000) {
            low = low | 8;
        } else if (options->health_bits == 0x40000000) {
            low = low | 0xc;
        } else if (options->health_bits == 0x40400000) {
            low = low | 0x10;
        } else if (options->health_bits == 0x40800000) {
            low = low | 0x14;
        }
    }
    low = low ^ (options->flags * 4 & 0x20);
    if (options->respawn_time != 0) {
        if (options->respawn_time == halo::game::seconds_to_ticks(5)) {
            low = low | 0x40;
        } else if (options->respawn_time == 300) {
            low = low | 0x80;
        } else if (options->respawn_time == halo::game::k_ticks_per_fifteen_seconds) {
            low = low | 0xc0;
        }
    }
    if (options->respawn_time_growth != 0) {
        if (options->respawn_time_growth == halo::game::seconds_to_ticks(5)) {
            low = low | 0x100;
        } else if (options->respawn_time_growth == 300) {
            low = low | 0x200;
        } else if (options->respawn_time_growth == halo::game::k_ticks_per_fifteen_seconds) {
            low = low | 0x300;
        }
    }
    {
        uint32_t bit4 = options->flags & 0x10;
        uint32_t bit3 = (uint32_t)(options->odd_man_out != 0) << 3;
        high = (bit3 | bit4) << 7 | low;
        if (options->suicide_penalty == 0) {
            high = (bit3 | bit4) << 7 | low;
        } else if (options->suicide_penalty == halo::game::seconds_to_ticks(5)) {
            high = ((bit3 | bit4) << 7 | low) | 0x1000;
        } else if (options->suicide_penalty == 300) {
            high = ((bit3 | bit4) << 7 | low) | 0x2000;
        } else if (options->suicide_penalty == halo::game::k_ticks_per_fifteen_seconds) {
            high = high | 0x3000;
        }
    }
    high = high ^ ((options->flags & 4) << 0xc);
    if ((int32_t)options->weapon_set < 0xe) {
        high = high ^ ((options->weapon_set & 0xf) << 0xf);
    }
    high = high ^ ((options->flags & 0x20) << 0xe);
    if ((int32_t)options->objective_indicator < 3) {
        high = high ^ ((options->objective_indicator & 3) << 0x14);
    }
    high = ((((options->flags & 2) << 1 | (options->flags & 1)) << 5 | (options->flags & 0x40)) << 0x11) | high;
    if (options->friendly_fire < 4) {
        high = high ^ ((uint32_t)(options->friendly_fire & 3) << 0x19);
    }
    if (options->betrayal_penalty != 0) {
        if (options->betrayal_penalty == halo::game::seconds_to_ticks(5)) {
            high = high | 0x8000000;
        } else if (options->betrayal_penalty == 300) {
            high = high | 0x10000000;
        } else if (options->betrayal_penalty == halo::game::k_ticks_per_fifteen_seconds) {
            high = high | 0x18000000;
        }
    }

    extra = 0;
    if (options->vehicle_respawn_time == 0) {
        extra = 0;
    } else if (options->vehicle_respawn_time == 900) {
        extra = 1;
    } else if (options->vehicle_respawn_time == halo::game::k_ticks_per_minute) {
        extra = 2;
    } else if (options->vehicle_respawn_time == halo::game::seconds_to_ticks(90)) {
        extra = 3;
    } else if (options->vehicle_respawn_time == halo::game::seconds_to_ticks(120)) {
        extra = 4;
    } else if (options->vehicle_respawn_time == halo::game::seconds_to_ticks(180)) {
        extra = 5;
    } else if (options->vehicle_respawn_time == 9000) {
        extra = 6;
    }
    {
        uint32_t nibble1 = options->red_vehicle_set & 0xf;
        if (nibble1 < 9) {
            extra = extra | (nibble1 << 3);
        }
    }
    {
        uint32_t nibble2 = options->blue_vehicle_set & 0xf;
        if (nibble2 < 9) {
            extra = (nibble2 << 7) | extra;
        }
    }

    sprintf(server_browser_custom_options_text, "%d,%d",
            high ^ ((uint32_t)(options->team_autobalance != 0) << 0x1d), extra);
    return server_browser_custom_options_text;
}

void ServerBrowser::custom_options_unpack(char *text, server_browser_custom_options *out)
{
    uint32_t low;
    uint32_t high;

    sscanf(text, "%d,%d", &low, &high);

    switch (low & 3) {
    case 1: out->lives_per_round = 1; break;
    case 2: out->lives_per_round = 3; break;
    case 3: out->lives_per_round = 5; break;
    default: out->lives_per_round = 0; break;
    }

    switch ((low >> 2) & 7) {
    case 0: out->health_bits = 0x3f000000; break;
    case 2: out->health_bits = 0x3fc00000; break;
    case 3: out->health_bits = 0x40000000; break;
    case 4: out->health_bits = 0x40400000; break;
    case 5: out->health_bits = 0x40800000; break;
    default: out->health_bits = halo::game::k_float_one_bits; break;
    }

    if ((low & 0x20) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::shields_disabled);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::shields_disabled);
    }

    switch ((low >> 6) & 3) {
    case 1: out->respawn_time = halo::game::seconds_to_ticks(5); break;
    case 2: out->respawn_time = 300; break;
    case 3: out->respawn_time = halo::game::k_ticks_per_fifteen_seconds; break;
    default: out->respawn_time = 0; break;
    }

    switch ((low >> 8) & 3) {
    case 1: out->respawn_time_growth = halo::game::seconds_to_ticks(5); break;
    case 2: out->respawn_time_growth = 300; break;
    case 3: out->respawn_time_growth = halo::game::k_ticks_per_fifteen_seconds; break;
    default: out->respawn_time_growth = 0; break;
    }

    out->odd_man_out = (uint8_t)((low >> 10) & 1);

    if ((low & 0x800) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::invisible_players);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::invisible_players);
    }

    switch ((low >> 0xc) & 3) {
    case 1: out->suicide_penalty = halo::game::seconds_to_ticks(5); break;
    case 2: out->suicide_penalty = 300; break;
    case 3: out->suicide_penalty = halo::game::k_ticks_per_fifteen_seconds; break;
    default: out->suicide_penalty = 0; break;
    }

    if ((low & 0x4000) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::maximum_grenades);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::maximum_grenades);
    }

    {
        uint32_t nibble = (low >> 0xf) & 0xf;
        out->weapon_set = (nibble < 0xe) ? nibble : 0;
    }

    if ((low & 0x80000) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::loadout_override);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::loadout_override);
    }

    {
        uint32_t two_bits = (low >> 0x14) & 3;
        out->objective_indicator = (two_bits <= 2) ? two_bits : 0;
    }

    if ((low & 0x400000) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::individual_scoring);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::individual_scoring);
    }
    if ((low & 0x800000) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::hide_radar_blips);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::hide_radar_blips);
    }
    if ((low & 0x1000000) == 0) {
        out->flags = out->flags & ~halo::to_bits(halo::game::game_variant_flags::reserved_1);
    } else {
        out->flags = out->flags | halo::to_bits(halo::game::game_variant_flags::reserved_1);
    }

    {
        uint8_t two_bits = (uint8_t)((low >> 0x19) & 3);
        out->friendly_fire = (two_bits < 4) ? two_bits : 0;
    }

    switch ((low >> 0x1b) & 3) {
    case 1: out->betrayal_penalty = halo::game::seconds_to_ticks(5); break;
    case 2: out->betrayal_penalty = 300; break;
    case 3: out->betrayal_penalty = halo::game::k_ticks_per_fifteen_seconds; break;
    default: out->betrayal_penalty = 0; break;
    }

    out->team_autobalance = (low & 0x20000000) == 0x20000000;

    switch (high & 7) {
    case 1: out->vehicle_respawn_time = 900; break;
    case 2: out->vehicle_respawn_time = halo::game::k_ticks_per_minute; break;
    case 3: out->vehicle_respawn_time = halo::game::seconds_to_ticks(90); break;
    case 4: out->vehicle_respawn_time = halo::game::seconds_to_ticks(120); break;
    case 5: out->vehicle_respawn_time = halo::game::seconds_to_ticks(180); break;
    case 6: out->vehicle_respawn_time = 9000; break;
    default: out->vehicle_respawn_time = 0; break;
    }

    {
        uint32_t nibble1 = (high >> 3) & 0xf;
        uint32_t nibble2 = (high >> 7) & 0xf;
        out->red_vehicle_set = (nibble1 < 9) ? nibble1 : 0;
        out->blue_vehicle_set = (nibble2 < 9) ? nibble2 : 0;
    }
}

void ServerBrowser::filter_headers_refresh(network_ui_widget *row)
{
    network_ui_widget *password_check;
    network_ui_widget *dedicated_check;
    network_ui_widget *classic_check;
    network_ui_widget *sort_col0;
    network_ui_widget *sort_col1;
    network_ui_widget *sort_col2;
    network_ui_widget *sort_col4;
    network_ui_widget *sort_col3;
    network_ui_widget *mark;
    int32_t direction;

    password_check = row->first_child;
    dedicated_check = password_check->next_sibling;
    sort_col0 = dedicated_check->next_sibling;
    sort_col1 = sort_col0->next_sibling;
    classic_check = sort_col1->next_sibling;
    sort_col2 = classic_check->next_sibling;
    sort_col4 = sort_col2->next_sibling;
    sort_col3 = sort_col4->next_sibling;

    direction = (server_browser_sort_column == 0) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    halo::networking::server_browser_column_header_update(sort_col0, direction);
    direction = (server_browser_sort_column == 1) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    halo::networking::server_browser_column_header_update(sort_col1, direction);
    direction = (server_browser_sort_column == 2) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    halo::networking::server_browser_column_header_update(sort_col2, direction);
    direction = (server_browser_sort_column == 4) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    halo::networking::server_browser_column_header_update(sort_col4, direction);
    direction = (server_browser_sort_column == 3) ? (server_browser_sort_ascending ? 1 : -1) : 0;
    halo::networking::server_browser_column_header_update(sort_col3, direction);

    mark = password_check->first_child;
    mark->highlight_flag = (server_browser_allow_password == 0) + 1;
    if (password_check->parent->selected_child == password_check) {
        password_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
    } else {
        password_check->highlight_flag = 0;
        *(uint32_t *)&mark->alpha = 0x3f400000;
    }

    mark = dedicated_check->first_child;
    mark->highlight_flag = (server_browser_filter_dedicated_only != 0) + 1;
    if (dedicated_check->parent->selected_child == dedicated_check) {
        dedicated_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
    } else {
        dedicated_check->highlight_flag = 0;
        *(uint32_t *)&mark->alpha = 0x3f400000;
    }

    mark = classic_check->first_child;
    mark->highlight_flag = (server_browser_filter_classic_only != 0) + 1;
    if (classic_check->parent->selected_child == classic_check) {
        classic_check->highlight_flag = 1;
        *(uint32_t *)&mark->alpha = 0x3f800000;
        return;
    }
    classic_check->highlight_flag = 0;
    *(uint32_t *)&mark->alpha = 0x3f400000;
}

void ServerBrowser::filter_panel_set_mode(network_ui_widget *panel, uint8_t internet_mode)
{
    network_ui_widget *w;
    network_ui_widget *control;
    uint16_t clamped;

    w = panel->first_child;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    if (internet_mode == 0) {
        w->visible = 1;
        w->hidden = 0;
    } else {
        w->visible = 0;
        w->hidden = 1;
    }
    w = w->next_sibling;
    w->visible = 0;
    w->hidden = 1;
    w = w->next_sibling;
    if (internet_mode != 0) {
        w->visible = 1;
        w->hidden = 0;
        w->parent->selected_child = w;

        w = w->first_child->next_sibling->first_child;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_allow_empty != 0);

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_allow_full != 0);

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_ping_limit_index <= clamped) {
            clamped = server_browser_filter_ping_limit_index;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_gametype <= clamped) {
            clamped = server_browser_filter_gametype;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        clamped = control->max_value - 1;
        if ((uint16_t)server_browser_filter_teamplay <= clamped) {
            clamped = server_browser_filter_teamplay;
        }
        control->value = (int16_t)clamped;

        w = w->next_sibling;
        control = ui_widget_find_control(w);
        control->value = (int16_t)(server_browser_filter_allow_unknown_map != 0);

        w->visible = 0;
        w->hidden = 1;
        server_browser_filter_panel_mode = internet_mode;
        return;
    }
    w->visible = 0;
    w->hidden = 1;
    w->parent->selected_child = w->parent->first_child->next_sibling;
    server_browser_filter_panel_mode = internet_mode;
}

int32_t ServerBrowser::filter_widget_clicked(network_ui_widget *clicked)
{
    network_ui_widget *w1;
    network_ui_widget *w2;
    network_ui_widget *w3;
    network_ui_widget *w4;
    network_ui_widget *w5;
    network_ui_widget *w6;
    network_ui_widget *w7;
    uint8_t new_sort_column;

    w1 = clicked->parent->first_child;
    w2 = w1->next_sibling;
    w3 = w2->next_sibling;
    w4 = w3->next_sibling;
    w5 = w4->next_sibling;
    w6 = w5->next_sibling;
    w7 = w6->next_sibling;

    if (clicked == w1) {
        server_browser_allow_password = (server_browser_allow_password == 0);
        goto play_and_set_query_mode;
    }
    if (clicked == w2) {
        server_browser_filter_dedicated_only = (server_browser_filter_dedicated_only == 0);
        goto play_and_set_query_mode;
    }
    if (clicked == w3) {
        if (server_browser_sort_column == 0) {
        toggle_direction:
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
        server_browser_sort_column = 0;
        new_sort_column = server_browser_sort_column;
    } else if (clicked == w4) {
        if (server_browser_sort_column == 1) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
        server_browser_sort_column = 1;
        new_sort_column = server_browser_sort_column;
    } else if (clicked == w5) {
        server_browser_filter_classic_only = (server_browser_filter_classic_only == 0);
        goto play_and_set_query_mode;
    } else if (clicked == w6) {
        if (server_browser_sort_column == 2) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
        } else {
            server_browser_sort_column = 2;
            server_browser_sort_ascending = 1;
        }
        goto play_and_reset_query_timer;
    } else if (clicked == w7) {
        new_sort_column = 4;
        if (server_browser_sort_column == 4) {
            server_browser_sort_ascending = (server_browser_sort_ascending == 0);
            goto play_and_reset_query_timer;
        }
    } else if (clicked == w7->next_sibling) {
        new_sort_column = 3;
        if (server_browser_sort_column == 3) {
            goto toggle_direction;
        }
    } else {
        return 1;
    }
    server_browser_sort_column = new_sort_column;
    server_browser_sort_ascending = 1;
play_and_reset_query_timer:
    halo::interface::widget_play_sound_effect(2);
    server_browser_query_elapsed_ms = 9999;
    return 1;
play_and_set_query_mode:
    halo::interface::widget_play_sound_effect(2);
    server_browser_query_pending = 1;
    return 1;
}

uint32_t ServerBrowser::gametype1_flags_pack(server_browser_gametype1_options *options)
{
    uint8_t *flags = options->flags;
    int32_t time_limit = options->time_limit;
    uint32_t bits = (((uint32_t)(flags[3] != 0) << 1 | (uint32_t)(flags[2] != 0)) << 1 |
                      (uint32_t)(flags[1] != 0)) << 1 | (uint32_t)(flags[0] != 0);
    if (time_limit == 0) {
        return bits << 3 | 1;
    }
    if (time_limit == halo::game::k_ticks_per_minute) {
        return bits << 3 | 0x81;
    }
    if (time_limit == halo::game::seconds_to_ticks(120)) {
        return bits << 3 | 0x101;
    }
    if (time_limit == halo::game::seconds_to_ticks(180)) {
        return bits << 3 | 0x181;
    }
    if (time_limit == 9000) {
        return bits << 3 | 0x201;
    }
    if (time_limit == 18000) {
        return bits << 3 | 0x281;
    }
    return bits << 3 | 1;
}

void ServerBrowser::gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out)
{
    out->flags[0] = (uint8_t)(code >> 3) & 1;
    out->flags[1] = (uint8_t)(code >> 4) & 1;
    out->flags[2] = (uint8_t)(code >> 5) & 1;
    out->flags[3] = (uint8_t)(code >> 6) & 1;
    switch ((code >> 7) & 7) {
    case 1: out->low = 8; out->high = 7; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 2: out->low = 0x10; out->high = 0xe; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 3: out->low = 0x18; out->high = 0x15; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 4: out->low = 0x28; out->high = 0x23; out->unknown_06 = 0; out->unknown_07 = 0; break;
    case 5: out->low = 0x50; out->high = 0x46; out->unknown_06 = 0; out->unknown_07 = 0; break;
    default: out->low = 0; out->high = 0; out->unknown_06 = 0; out->unknown_07 = 0; break;
    }
}

uint32_t ServerBrowser::gametype2_flags_pack(uint8_t *flags)
{
    return (((uint32_t)(flags[2] != 0) << 1 | (uint32_t)(flags[1] != 0)) << 1 |
            (uint32_t)(flags[0] != 0)) << 3 | 2;
}

uint32_t ServerBrowser::gametype3_flags_pack(server_browser_gametype3_options *options)
{
    return (((((((((uint32_t)options->value_14 & 0x1f) << 2 | ((uint32_t)options->value_10 & 3)) << 2 |
                 ((uint32_t)options->value_0c & 3)) << 2 | ((uint32_t)options->value_08 & 3)) << 2 |
               ((uint32_t)options->value_04 & 3)) << 1 | (uint32_t)(options->flag1 != 0)) << 1 |
             (uint32_t)(options->flag0 != 0)) << 3 | 3);
}

void ServerBrowser::gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out)
{
    out->flag0 = (uint8_t)(code >> 3) & 1;
    out->flag1 = (uint8_t)(code >> 4) & 1;
    out->value_04 = (int32_t)((code >> 5) & 3);
    out->value_08 = (int32_t)((code >> 7) & 3);
    out->value_0c = (int32_t)((code >> 9) & 3);
    out->value_10 = (int32_t)((code >> 0xb) & 3);
    out->value_14 = (int32_t)((code >> 0xd) & 0x1f);
}

uint32_t ServerBrowser::gametype5_flags_pack(int32_t *values)
{
    uint32_t code = 5;
    if (values[0] < 3) {
        code = ((uint32_t)values[0] & 3) << 3 | 5;
    }
    if (values[1] < 3) {
        code = code ^ (((uint32_t)values[1] & 3) << 5);
    }
    return code;
}

void ServerBrowser::gametype5_flags_unpack(uint32_t code, int32_t *out)
{
    uint32_t a = (code >> 3) & 3;
    uint32_t b = (code >> 5) & 3;
    out[0] = (a <= 2) ? (int32_t)a : 0;
    out[1] = (b <= 2) ? (int32_t)b : 0;
}

void ServerBrowser::latch_join_target(void)
{
    uint32_t wait_result;

    if (server_list_thread != 0) {
        wait_result = WaitForSingleObject(server_list_mutex->handle, 100);
        if (wait_result != 0 && wait_result != 0x80) {
            return;
        }
    }
    if (server_browser_join_requested != 0 && -1 < server_browser_selected_index &&
        server_browser_selected_index < server_list.result_count) {
        server_browser_join_target =
            *(void **)((uint8_t *)&server_list + server_browser_selected_index * 4);
        server_browser_join_target_has_password =
            SBServerGetBoolValue(server_browser_join_target, "password", 0);
        if (server_browser_join_target_has_password != 0) {
            halo::interface::virtual_keyboard_open(network_join_target_address, 0x12, 0xc);
            network_host_edit_field_00719410 = 0;
        }
        master_server_request_flags = master_server_request_flags | 4;
    }
    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

void ServerBrowser::list_row_gather(network_ui_widget *row, uint8_t flag, void *entry)
{
    char *hostname;
    char *mapname;
    char *gametype;
    int32_t is_classic;
    int32_t is_password;
    int32_t is_dedicated;
    int32_t probe;
    int32_t count_a;
    int32_t count_b;
    int32_t ping;
    wchar_t friendly_map[0x20];

    if (flag == 0) {
        row->highlight_flag = (row->parent->selected_child == row);
    } else {
        row->highlight_flag = 2;
    }
    if (entry != 0) {
        memset(friendly_map, 0, sizeof(friendly_map));
        is_password = SBServerGetBoolValue(entry, "password", 0);
        is_dedicated = SBServerGetBoolValue(entry, "dedicated", 0);
        hostname = SBServerGetStringValue(entry, "hostname", "");
        mapname = SBServerGetStringValue(entry, "mapname", "");
        gametype = SBServerGetStringValue(entry, "gametype", "");
        is_classic = SBServerGetBoolValue(entry, "game_classic", 0);

        probe = SBServerGetIntValue(entry, "numplayers", 0);
        count_b = 0x10;
        if (probe < -1) {
            count_a = -1;
        } else {
            probe = SBServerGetIntValue(entry, "numplayers", 0);
            if (probe < 0x11) {
                count_a = SBServerGetIntValue(entry, "numplayers", 0);
            } else {
                count_a = 0x10;
            }
        }
        probe = SBServerGetIntValue(entry, "maxplayers", 0);
        if (probe < -1) {
            count_b = -1;
        } else {
            probe = SBServerGetIntValue(entry, "maxplayers", 0);
            if (probe < 0x11) {
                count_b = SBServerGetIntValue(entry, "maxplayers", 0);
            }
        }
        probe = SBServerGetPing(entry);
        if (probe < 0) {
            ping = 0;
        } else {
            probe = SBServerGetPing(entry);
            if (probe < 10000) {
                ping = SBServerGetPing(entry);
            } else {
                ping = 9999;
            }
        }

        halo::interface::map_list_get_friendly_level_name(friendly_map, (char *)mapname, 0x20);
        halo::networking::server_browser_list_row_populate(row, (uint8_t)is_password, (uint8_t)is_dedicated,
                                          hostname, friendly_map, gametype,
                                          (uint8_t)(is_classic == 1), count_a, count_b, ping);
        row->hidden = 0;
        return;
    }
    halo::networking::server_browser_list_row_populate(row, 0, 0, 0, empty_string, 0, 0, halo::k_dword_none, halo::k_dword_none, halo::k_dword_none);
    row->hidden = 1;
}

void ServerBrowser::list_row_populate(network_ui_widget *row, uint8_t flag1, uint8_t flag2,
                                        const char *server_name, wchar_t *map_name,
                                        const char *gametype_name,
                                        uint8_t flag3, int32_t count_a, int32_t count_b, int32_t ping)
{
    network_ui_widget *w1;
    network_ui_widget *w2;
    wchar_t *text;

    w1 = row->first_child;
    w1->visible = flag1 != 0;
    w1->highlight_flag = 1;
    w1 = w1->next_sibling;
    w2 = w1->next_sibling;
    w1->highlight_flag = 1;
    w1->visible = flag2 != 0;
    text = (wchar_t *)halo::memory::heap_reallocate(w2->label_text, 0x80, halo::interface::globals().widget_memory_pool);
    w2->label_text = (uint16_t *)text;
    if (text != 0) {
        wchar_t *source = reinterpret_cast<wchar_t *>(halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(string_widen_scratch), 0x800, server_name));
        wcsncpy((wchar_t *)w2->label_text, source, 0x3f);
        *(uint16_t *)((uint8_t *)w2->label_text + 0x7e) = 0;
    }
    w1 = w2->next_sibling;
    text = (wchar_t *)halo::memory::heap_reallocate(w1->label_text, 0x40, halo::interface::globals().widget_memory_pool);
    w1->label_text = (uint16_t *)text;
    if (text != 0) {
        wcsncpy(text, map_name, 0x1f);
        *(uint16_t *)((uint8_t *)w1->label_text + 0x3e) = 0;
    }
    w1 = w1->next_sibling;
    w2 = w1->next_sibling;
    w1->highlight_flag = 1;
    w1->visible = flag3 != 0;
    text = (wchar_t *)halo::memory::heap_reallocate(w2->label_text, 0x40, halo::interface::globals().widget_memory_pool);
    w2->label_text = (uint16_t *)text;
    if (text != 0) {
        wchar_t *source = reinterpret_cast<wchar_t *>(halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(string_widen_scratch), 0x800, gametype_name));
        wcsncpy((wchar_t *)w2->label_text, source, 0x1f);
        *(uint16_t *)((uint8_t *)w2->label_text + 0x3e) = 0;
    }
    w1 = w2->next_sibling;
    text = (wchar_t *)halo::memory::heap_reallocate(w1->label_text, 0x40, halo::interface::globals().widget_memory_pool);
    w1->label_text = (uint16_t *)text;
    if (text != 0) {
        if (count_a == -1 || count_b == -1) {
            wcscpy(text, L"");
        } else {
            halo::text::string_format_wide_va_bounded(0x1f, reinterpret_cast<uint16_t *>(text), reinterpret_cast<const uint16_t *>(L"%d / %d"), count_a, count_b);
            *(uint16_t *)((uint8_t *)w1->label_text + 0x3e) = 0;
        }
    }
    w1 = w1->next_sibling;
    text = (wchar_t *)halo::memory::heap_reallocate(w1->label_text, 0x10, halo::interface::globals().widget_memory_pool);
    w1->label_text = (uint16_t *)text;
    if (text != 0) {
        if (0 < ping && ping < 9999) {
            halo::text::string_format_wide_va_bounded(7, reinterpret_cast<uint16_t *>(text), reinterpret_cast<const uint16_t *>(PTR_s_parameter_handles_0063fff0_0x35_006607a0), ping);
            *(uint16_t *)((uint8_t *)w1->label_text + 0xe) = 0;
            return;
        }
        wcscpy(text, L"");
    }
}

int32_t ServerBrowser::open(network_ui_widget *root)
{
    uint8_t &network_join_target_address = reinterpret_cast<uint8_t &>(::network_join_target_address);
    char motd_string[1020];
    int32_t motd_available;
    int32_t tag_index;
    uint16_t *source;
    uint8_t saved_config[3200];
    network_ui_widget *w0;
    network_ui_widget *w1;
    network_ui_widget *w2;
    network_ui_widget *w3;
    network_ui_widget *w4;
    network_ui_widget *list_container;
    network_ui_widget *col_headers[15];
    network_ui_widget *w;
    int32_t i;

    halo::networking::network_channels_open();
    if (server_browser_initialized == 0 && halo::networking::master_server_connection_start() == 0) {
        server_browser_join_requested = 1;
        master_server_query_engine = ServerBrowserNew(&network_session_start_host_name, &network_session_start_host_name, &network_session_start_map_name, 0,
                                                    10, 1, (void *)halo::networking::network_channel_gap_4ba660, 0);
    }
    server_browser_join_target = 0;
    server_browser_join_target_has_password = 0;
    network_join_target_address = 0;
    halo::networking::autopatch_download_pool_initialize();
    if (server_browser_require_valid_entry != 0 && browser_state::motd_download_state == 0) {
        motd_available = halo::shell::shell_load_localized_string(sizeof(motd_string), halo::shell::globals().module_handle, motd_string, 0x90);
        if (motd_available != 0) {
            browser_state::motd_download_slot = halo::networking::autopatch_download_start(motd_string, 0);
            browser_state::motd_download_state = 1;
            halo::networking::join_game_ticker_string_copy((uint16_t *)browser_state::ticker_message, 0x100, 4);
        }
    }
    if (browser_state::ticker_message[0] == 0) {
        browser_state::ticker_message[0] = 0;
        tag_index = halo::cache::tag_lookup(halo::groups::unicode_string_list,
            halo::tag_paths::join_game_ticker_labels);
        if (tag_index != -1) {
            source = halo::text::text_string_list_get_string(tag_index, 0);
            wcsncpy(browser_state::ticker_message, (const wchar_t *)source, 0xff);
            browser_state::ticker_message_terminator = 0;
        }
    }
    server_browser_player_ticker.scroll_delay_ms = 100;
    server_browser_variant_ticker.scroll_delay_ms = 100;
    server_browser_player_ticker.text = 0;
    server_browser_player_ticker.capacity = 0;
    server_browser_player_ticker.length = 0;
    server_browser_player_ticker.scroll_cursor = 0;
    server_browser_variant_ticker.text = 0;
    server_browser_variant_ticker.capacity = 0;
    server_browser_variant_ticker.length = 0;
    server_browser_variant_ticker.scroll_cursor = 0;

    if (halo::saved_games::globals().player_profile_slots_handle == -1) {
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
        memcpy(saved_config, profile_globals_block, 0x7ff * 4);
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
    halo::networking::server_list_reset((uint8_t *)0);

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
    halo::networking::join_game_server_browser_tick(root->selected_child);
    browser_state::next_auto_refresh_ms = 0;
    browser_state::refresh_in_flight = 0;
    return 1;
}

int32_t ServerBrowser::player_list_populate(void *entry)
{
    int32_t probe;
    int32_t player_count;
    int32_t i;
    char *name;
    char *score;
    wchar_t row[0x100];

    if (entry == 0) {
        return 1;
    }
    probe = SBServerGetIntValue(entry, "numplayers", 0);
    if (probe < 0) {
        player_count = 0;
    } else {
        probe = SBServerGetIntValue(entry, "numplayers", 0);
        if (probe < 0x11) {
            player_count = SBServerGetIntValue(entry, "numplayers", 0);
        } else {
            player_count = 0x10;
        }
    }
    halo::networking::ticker_text_buffer_append(0, 0, &server_browser_player_ticker);
    i = 0;
    if (0 < player_count) {
        do {
            name = SBServerGetPlayerStringValue(entry, i, "player", 0);
            if (name == 0) {
                swprintf(row, 0x100, L"  %s %d     ", hud_text_unbound, 0);
            } else {
                score = SBServerGetPlayerStringValue(entry, i, "score", "--");
                swprintf(row, 0x100, L"  %S %S     ", name, score);
            }
            halo::networking::ticker_text_buffer_append(row, 0, &server_browser_player_ticker);
            i = i + 1;
        } while (i < player_count);
    }
    return 1;
}

void ServerBrowser::query_results_ingest(server_list_globals *list)
{
    if (server_browser_initialized != 0 && master_server_query_engine != 0) {
        int32_t result_count = ServerBrowserCount(master_server_query_engine);
        int32_t i = 0;

        halo::networking::server_list_reset((uint8_t *)list);

        if (0 < result_count) {
            do {
                int32_t record = ServerBrowserGetServer(master_server_query_engine, i);

                if (SBServerHasBasicKeys(record) != 0) {
                    halo::networking::dynamic_pointer_array_add_unique((void *)(intptr_t)record, list);
                }
                i = i + 1;
            } while (i < result_count);
        }

        halo::networking::server_browser_result_array_sort(list);
        server_browser_query_pending = 0;
    }
}

void ServerBrowser::result_array_sort(server_list_globals *array)
{
    if (0 < array->result_count) {
        server_browser_sort_comparator comparator;
        void *previous_key;

        if (server_browser_selected_index == -1) {
            previous_key = 0;
        } else {
            previous_key = array->list[server_browser_selected_index];
        }

        comparator = halo::networking::server_browser_sort_comparator_select();
        qsort(array->list, (size_t)array->result_count, 4, comparator);

        if (server_browser_skip_reselect == 0) {
            server_browser_selected_index = -1;

            if (previous_key != 0 && 0 < array->result_count) {
                void **element = array->list;
                int32_t i = 0;

                do {
                    server_browser_selected_index = i;
                    if (*element == previous_key) {
                        break;
                    }
                    i = i + 1;
                    element = element + 1;
                    server_browser_selected_index = -1;
                } while (i < array->result_count);
            }

            if (server_browser_selected_index != -1 &&
                (server_browser_selected_index < server_list_scroll_offset ||
                 server_list_scroll_offset + 0xf <= server_browser_selected_index)) {
                server_list_scroll_offset = server_browser_selected_index;
                halo::networking::server_list_scroll_clamp(array);
            }
        }

        array->pending_count = 0;
        halo::networking::server_browser_total_players_compute(array);
    }
}

int32_t ServerBrowser::selected_variant_description_build(void *entry)
{
    char *player_flags;
    int32_t game_flags;
    char *gamevariant;
    char *fraglimit;
    wchar_t gamevariant_wide[0x400];
    wchar_t fraglimit_wide[0x400];

    halo::networking::ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    if (entry != 0) {
        player_flags = SBServerGetStringValue(entry, "player_flags", "");
        game_flags = SBServerGetIntValue(entry, "game_flags", 0);
        gamevariant = SBServerGetStringValue(entry, "gamevariant", 0);
        fraglimit = SBServerGetStringValue(entry, "fraglimit", "0");
        if (player_flags != 0 && game_flags != 0) {
            halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(gamevariant_wide), 0x800, gamevariant);
            halo::text::string_convert_ascii_to_unicode(reinterpret_cast<uint16_t *>(fraglimit_wide), 0x800, fraglimit);
            halo::game::multiplayer_game_variant_description_generate(player_flags, &server_browser_variant_ticker,
                game_flags, gamevariant_wide, fraglimit_wide);
        }
    }
    return 1;
}

uint8_t ServerBrowser::server_passes_filter(void *entry)
{
    int32_t probe;
    int32_t ping;
    int32_t numplayers;
    int32_t maxplayers;
    int32_t has_password;
    int32_t is_dedicated;
    int32_t is_classic;
    char *gametype_name;
    char *mapname;
    int32_t is_teamplay;
    char *gamever;
    uint8_t version_outdated;
    const char *gametype_filter_name;
    int32_t teamplay_mismatch;

    if (entry == 0 ||
        (server_browser_require_valid_entry != 0 &&
         (probe = (int32_t)halo::networking::gamespy_array_length((void *)((int32_t)(uintptr_t)entry)), probe == 0))) {
        return 0;
    }

    probe = SBServerGetPing(entry);
    maxplayers = -1;
    if (probe < -1) {
        ping = -1;
    } else {
        probe = SBServerGetPing(entry);
        if (probe < 10000) {
            ping = SBServerGetPing(entry);
        } else {
            ping = 9999;
        }
    }

    probe = SBServerGetIntValue(entry, "numplayers", 0);
    if (probe < -1) {
        numplayers = -1;
    } else {
        probe = SBServerGetIntValue(entry, "numplayers", 0);
        if (probe < 0x11) {
            numplayers = SBServerGetIntValue(entry, "numplayers", 0);
        } else {
            numplayers = 0x10;
        }
    }

    probe = SBServerGetIntValue(entry, "maxplayers", 0);
    if (-2 < probe) {
        probe = SBServerGetIntValue(entry, "maxplayers", 0);
        if (probe < 0x11) {
            maxplayers = SBServerGetIntValue(entry, "maxplayers", 0);
        } else {
            maxplayers = 0x10;
        }
    }

    has_password = SBServerGetBoolValue(entry, "password", 0);
    is_dedicated = SBServerGetBoolValue(entry, "dedicated", 0);
    is_classic = SBServerGetBoolValue(entry, "game_classic", 0);
    gametype_name = SBServerGetStringValue(entry, "gametype", "");
    mapname = SBServerGetStringValue(entry, "mapname", "");
    is_teamplay = SBServerGetBoolValue(entry, "teamplay", 0);
    gamever = SBServerGetStringValue(entry, "gamever", "");
    version_outdated = halo::networking::autopatch_version_string_is_outdated(gamever);

    if (0x270e < ping) {
        return 0;
    }
    if (numplayers == -1) {
        return 0;
    }
    if (maxplayers == -1) {
        return 0;
    }
    if (server_browser_filter_dedicated_only != 0 && is_dedicated != 1) {
        return 0;
    }
    if (server_browser_allow_password == 0 && has_password == 1) {
        return 0;
    }
    if (server_browser_filter_classic_only != 0 && is_classic != 1) {
        return 0;
    }
    if (server_browser_allow_empty == 0 && numplayers == 0) {
        return 0;
    }
    if (server_browser_allow_full == 0 && maxplayers <= numplayers) {
        return 0;
    }
    if (server_browser_filter_ping_limit_index != 0 &&
        server_browser_ping_limits[server_browser_filter_ping_limit_index] < ping) {
        return 0;
    }
    if (version_outdated == 0) {
        return 0;
    }
    if (server_browser_filter_gametype != 0) {
        switch (server_browser_filter_gametype) {
        case 1:
            gametype_filter_name = "CTF";
            break;
        case 2:
            gametype_filter_name = "Slayer";
            break;
        case 3:
            gametype_filter_name = "Oddball";
            break;
        case 4:
            gametype_filter_name = "King";
            break;
        case 5:
            gametype_filter_name = "Race";
            break;
        default:
            goto skip_gametype_check;
        }
        probe = _stricmp(gametype_name, gametype_filter_name);
        if (probe != 0) {
            return 0;
        }
    }
skip_gametype_check:
    if (is_teamplay == 1) {
        teamplay_mismatch = (server_browser_filter_teamplay == 1);
    } else {
        teamplay_mismatch = (server_browser_filter_teamplay == 2);
    }
    if (!teamplay_mismatch &&
        (server_browser_filter_allow_unknown_map != 0 ||
         (probe = halo::interface::map_list_find_known_map_index(mapname), probe != -1))) {
        return 1;
    }
    return 0;
}

server_browser_sort_comparator ServerBrowser::sort_comparator_select(void)
{
    return ServerSortRegistry::for_column(server_browser_sort_column).comparator();
}

void ServerBrowser::total_players_compute(server_list_globals *array)
{
    int32_t i = 0;

    server_browser_total_players = 0;
    if (0 < array->result_count) {
        do {
            int32_t num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);

            if (-2 < num_players) {
                num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);
                if (num_players < 0x11) {
                    num_players = SBServerGetIntValue(array->list[i], "numplayers", -1);
                    if (num_players == -1) {
                        goto next;
                    }
                } else {
                    num_players = 0x10;
                }
                server_browser_total_players = server_browser_total_players + num_players;
            }
next:
            i = i + 1;
        } while (i < array->result_count);
    }
}

void ServerBrowser::ui_refresh(void)
{
    halo::networking::ticker_text_buffer_append(0, 2, &server_browser_player_ticker);
    halo::networking::ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    halo::networking::ticker_text_buffer_append(browser_state::ticker_message, 0, &server_browser_player_ticker);
}

int32_t ServerBrowser::compare_by_gametype(const void *a, const void *b)
{
    const char * (*const SBServerGetStringValue)(void *server, const char *key, const char *default_value) = reinterpret_cast<const char * (*)(void *server, const char *key, const char *default_value)>(&::SBServerGetStringValue);
    int32_t (*const SBServerGetIntValue)(void *server, const char *key, int32_t default_value) = reinterpret_cast<int32_t (*)(void *server, const char *key, int32_t default_value)>(&::SBServerGetIntValue);
    int32_t result = key_order(a, b, "gametype");

    if (result != 0) {
        return result;
    }
    result = key_order(a, b, "hostname");
    if (result != 0) {
        return result;
    }
    return address_order(a, b);
}

int32_t ServerBrowser::compare_by_hostname(const void *a, const void *b)
{
    const char * (*const SBServerGetStringValue)(void *server, const char *key, const char *default_value) = reinterpret_cast<const char * (*)(void *server, const char *key, const char *default_value)>(&::SBServerGetStringValue);
    int32_t (*const SBServerGetIntValue)(void *server, const char *key, int32_t default_value) = reinterpret_cast<int32_t (*)(void *server, const char *key, int32_t default_value)>(&::SBServerGetIntValue);
    int32_t result = key_order(a, b, "hostname");

    if (result != 0) {
        return result;
    }
    result = key_order(a, b, "gametype");
    if (result != 0) {
        return result;
    }
    return address_order(a, b);
}

int32_t ServerBrowser::compare_by_mapname(void **a, void **b)
{
    void *entry_a;
    void *entry_b;
    char *map_name_a;
    char *map_name_b;
    wchar_t friendly_a[0x20];
    wchar_t friendly_b[0x20];
    int32_t result;

    entry_a = *a;
    entry_b = *b;
    memset(friendly_b, 0, sizeof(friendly_b));
    memset(friendly_a, 0, sizeof(friendly_a));
    map_name_a = SBServerGetStringValue(entry_a, "mapname", "");
    map_name_b = SBServerGetStringValue(entry_b, "mapname", "");
    halo::interface::map_list_get_friendly_level_name(friendly_a, (char *)map_name_a, 0x20);
    halo::interface::map_list_get_friendly_level_name(friendly_b, (char *)map_name_b, 0x20);
    result = wcscmp(friendly_a, friendly_b);
    if (server_browser_sort_ascending == 0) {
        result = -result;
    }
    return result;
}

int32_t ServerBrowser::compare_by_mapname_then_hostname(void **a, void **b)
{
    int32_t mapname_diff;
    char *hostname_a;
    char *hostname_b;
    int32_t hostname_diff;

    mapname_diff = halo::networking::server_list_compare_by_mapname(a, b);
    if (mapname_diff == 0) {
        hostname_b = SBServerGetStringValue(*b, "hostname", "");
        hostname_a = SBServerGetStringValue(*a, "hostname", "");
        hostname_diff = _stricmp(hostname_a, hostname_b);
        if (server_browser_sort_ascending == 0) {
            hostname_diff = -hostname_diff;
        }
        if (hostname_diff == 0) {
            if (server_browser_sort_ascending != 0) {
                if ((void **)a < (void **)b) {
                    return -1;
                }
                return (int32_t)((void **)b < (void **)a);
            }
            if ((void **)a < (void **)b) {
                return 1;
            }
            return -(int32_t)((void **)b < (void **)a);
        }
        return hostname_diff;
    }
    return mapname_diff;
}

int32_t ServerBrowser::compare_by_ping_then_hostname(void **a, void **b)
{
    int32_t ping_a;
    int32_t ping_b;
    int32_t ping_diff;
    char *hostname_a;
    char *hostname_b;
    int32_t hostname_diff;

    ping_a = SBServerGetIntValue(*a, "ping", 0);
    ping_b = SBServerGetIntValue(*b, "ping", 0);
    if (ping_a == 0) {
        ping_a = 9999;
    }
    if (ping_b == 0) {
        ping_b = 9999;
    }
    ping_diff = ping_a - ping_b;
    if (ping_diff == 0) {
        hostname_b = SBServerGetStringValue(*b, "hostname", "");
        hostname_a = SBServerGetStringValue(*a, "hostname", "");
        hostname_diff = _stricmp(hostname_a, hostname_b);
        if (server_browser_sort_ascending == 0) {
            hostname_diff = -hostname_diff;
        }
        if (hostname_diff == 0) {
            if (server_browser_sort_ascending == 0) {
                if ((void **)b <= (void **)a) {
                    return -(int32_t)((void **)b < (void **)a);
                }
                return 1;
            }
            if ((void **)b <= (void **)a) {
                return (int32_t)((void **)b < (void **)a);
            }
            return -1;
        }
        return hostname_diff;
    }
    if (server_browser_sort_ascending == 0) {
        ping_diff = -ping_diff;
    }
    return ping_diff;
}

int32_t ServerBrowser::compare_by_players(const void *a, const void *b)
{
    const char * (*const SBServerGetStringValue)(void *server, const char *key, const char *default_value) = reinterpret_cast<const char * (*)(void *server, const char *key, const char *default_value)>(&::SBServerGetStringValue);
    int32_t (*const SBServerGetIntValue)(void *server, const char *key, int32_t default_value) = reinterpret_cast<int32_t (*)(void *server, const char *key, int32_t default_value)>(&::SBServerGetIntValue);
    void *server_a = *(void **)a;
    void *server_b = *(void **)b;
    int32_t players_a = SBServerGetIntValue(server_a, "numplayers", 0);
    int32_t players_b = SBServerGetIntValue(server_b, "numplayers", 0);
    int32_t maximum_a = SBServerGetIntValue(server_a, "maxplayers", 0);
    int32_t maximum_b = SBServerGetIntValue(server_b, "maxplayers", 0);
    int32_t result;

    if (players_a != players_b) {
        result = players_a < players_b ? 1 : -1;
    } else if (maximum_a != maximum_b) {
        result = maximum_a < maximum_b ? 1 : -1;
    } else {
        result = halo::networking::server_list_compare_by_string_key((void **)a, (void **)b, "hostname");
        if (result == 0) {
            result = (uint32_t)b > (uint32_t)a ? -1 : ((uint32_t)b < (uint32_t)a ? 1 : 0);
        }
    }
    return server_browser_sort_ascending != 0 ? result : -result;
}

int32_t ServerBrowser::compare_by_string_key(void **a, void **b, const char *key)
{
    char *string_b;
    char *string_a;
    int32_t result;

    string_b = SBServerGetStringValue(*b, key, "");
    string_a = SBServerGetStringValue(*a, key, "");
    result = _stricmp(string_a, string_b);
    if (server_browser_sort_ascending == 0) {
        result = -result;
    }
    return result;
}

server_list_globals * ServerBrowser::mutex_try_lock(uint32_t timeout_ms)
{
    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, timeout_ms);

        if (wait_result != 0 && wait_result != 0x80) {
            return 0;
        }
    }
    return &server_list;
}

void ServerBrowser::mutex_unlock(server_list_globals **list_slot)
{
    *list_slot = 0;
    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

void ServerBrowser::reset(uint8_t *entry)
{
    server_list_scroll_offset = 0;
    server_browser_selected_index = -1;
    server_browser_last_click_ms = 0;
    server_browser_player_list_ready = 0;
    server_browser_skip_reselect = 0;
    browser_state::scroll_arrow_flash = 0;
    server_browser_total_players = 0;
    halo::networking::server_list_result_reset(entry);
    halo::networking::ticker_text_buffer_append(0, 2, &server_browser_player_ticker);
    halo::networking::ticker_text_buffer_append(0, 1, &server_browser_variant_ticker);
    halo::networking::ticker_text_buffer_append(browser_state::ticker_message, 0, &server_browser_player_ticker);
}

uint32_t ServerBrowser::result_count_get(void)
{
    uint32_t result;

    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, 100);

        if (wait_result != 0 && wait_result != 0x80) {
            return 0;
        }
    }

    result = (uint32_t)server_list.result_count;

    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
    return result;
}

void ServerBrowser::result_reset(uint8_t *entry)
{
    if (entry != 0) {
        *(uint32_t *)(entry + 4) = 0;
        *(uint32_t *)(entry + 0xc) = 0;
        return;
    }

    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, 100);

        if (wait_result != 0 && wait_result != 0x80) {
            return;
        }
    }

    server_list.result_count = 0;
    server_browser_query_elapsed_ms = 0;

    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

void ServerBrowser::scroll_clamp(server_list_globals *results)
{
    int32_t count;
    int32_t max_scroll;

    if (results == 0) {
        count = halo::networking::server_list_result_count_get();
    } else {
        count = results->result_count;
    }
    max_scroll = (count - 0xf < 0) ? 0 : (count - 0xf);
    if (server_list_scroll_offset < 0) {
        server_list_scroll_offset = 0;
        return;
    }
    if (max_scroll < server_list_scroll_offset) {
        server_list_scroll_offset = max_scroll;
    }
}

void ServerBrowser::scroll_page_down(uint8_t jump_to_bottom)
{
    int32_t old_offset;
    int32_t count;
    int32_t max_scroll;

    old_offset = server_list_scroll_offset;
    if (jump_to_bottom == 0) {
        server_list_scroll_offset = server_list_scroll_offset + 0xe;
    } else {
        count = halo::networking::server_list_result_count_get();
        server_list_scroll_offset = count - 0xf;
    }
    count = halo::networking::server_list_result_count_get();
    max_scroll = (count - 0xf < 0) ? 0 : (count - 0xf);
    if (server_list_scroll_offset < 0) {
        max_scroll = 0;
    } else if (server_list_scroll_offset <= max_scroll) {
        goto after_clamp;
    }
    server_list_scroll_offset = max_scroll;
after_clamp:
    if (old_offset != server_list_scroll_offset) {
        browser_state::scroll_arrow_flash = 0x10;
        if (server_list_scroll_offset <= server_browser_selected_index &&
            server_browser_selected_index < server_list_scroll_offset + 0xf) {
            server_browser_skip_reselect = 0;
            return;
        }
        server_browser_skip_reselect = 1;
    }
}

void ServerBrowser::scroll_page_up(uint8_t jump_to_top)
{
    int32_t old_offset;
    int32_t count;
    int32_t max_scroll;

    old_offset = server_list_scroll_offset;
    if (jump_to_top == 0) {
        server_list_scroll_offset = server_list_scroll_offset - 0xe;
    } else {
        server_list_scroll_offset = 0;
    }
    count = halo::networking::server_list_result_count_get();
    max_scroll = (count - 0xf < 0) ? 0 : (count - 0xf);
    if (server_list_scroll_offset < 0) {
        max_scroll = 0;
    } else if (server_list_scroll_offset <= max_scroll) {
        goto after_clamp;
    }
    server_list_scroll_offset = max_scroll;
after_clamp:
    if (old_offset != server_list_scroll_offset) {
        browser_state::scroll_arrow_flash = 0xfffffff0;
        if (server_list_scroll_offset <= server_browser_selected_index &&
            server_browser_selected_index < server_list_scroll_offset + 0xf) {
            server_browser_skip_reselect = 0;
            return;
        }
        server_browser_skip_reselect = 1;
    }
}

}  // namespace halo::networking

namespace halo::networking {
void game_variant_list_matching_substring(uint32_t argument_count, char **arguments)
{
    halo::networking::ServerBrowser::matching_substring(argument_count, arguments);
}

int32_t join_game_server_browser_tick(network_ui_widget *browser_widget)
{
    return halo::networking::ServerBrowser::server_browser_tick(browser_widget);
}

void join_game_ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index)
{
    halo::networking::ServerBrowser::ticker_string_copy(buffer, capacity, string_index);
}

void map_list_matching_substring(uint32_t argument_count, char **arguments)
{
    halo::networking::ServerBrowser::map_list_matching_substring(argument_count, arguments);
}

void server_browser_column_header_update(network_ui_widget *header, int32_t sort_direction)
{
    halo::networking::ServerBrowser::column_header_update(header, sort_direction);
}

char * server_browser_custom_options_pack(server_browser_custom_options *options)
{
    return halo::networking::ServerBrowser::custom_options_pack(options);
}

void server_browser_custom_options_unpack(char *text, server_browser_custom_options *out)
{
    halo::networking::ServerBrowser::custom_options_unpack(text, out);
}

void server_browser_filter_headers_refresh(network_ui_widget *row)
{
    halo::networking::ServerBrowser::filter_headers_refresh(row);
}

void server_browser_filter_panel_set_mode(network_ui_widget *panel, uint8_t internet_mode)
{
    halo::networking::ServerBrowser::filter_panel_set_mode(panel, internet_mode);
}

int32_t server_browser_filter_widget_clicked(network_ui_widget *clicked)
{
    return halo::networking::ServerBrowser::filter_widget_clicked(clicked);
}

uint32_t server_browser_gametype1_flags_pack(server_browser_gametype1_options *options)
{
    return halo::networking::ServerBrowser::gametype1_flags_pack(options);
}

void server_browser_gametype1_flags_unpack(uint32_t code, server_browser_gametype1_decoded *out)
{
    halo::networking::ServerBrowser::gametype1_flags_unpack(code, out);
}

uint32_t server_browser_gametype2_flags_pack(uint8_t *flags)
{
    return halo::networking::ServerBrowser::gametype2_flags_pack(flags);
}

uint32_t server_browser_gametype3_flags_pack(server_browser_gametype3_options *options)
{
    return halo::networking::ServerBrowser::gametype3_flags_pack(options);
}

void server_browser_gametype3_flags_unpack(uint32_t code, server_browser_gametype3_options *out)
{
    halo::networking::ServerBrowser::gametype3_flags_unpack(code, out);
}

uint32_t server_browser_gametype5_flags_pack(int32_t *values)
{
    return halo::networking::ServerBrowser::gametype5_flags_pack(values);
}

void server_browser_gametype5_flags_unpack(uint32_t code, int32_t *out)
{
    halo::networking::ServerBrowser::gametype5_flags_unpack(code, out);
}

void server_browser_latch_join_target(void)
{
    halo::networking::ServerBrowser::latch_join_target();
}

void server_browser_list_row_gather(network_ui_widget *row, uint8_t flag, void *entry)
{
    halo::networking::ServerBrowser::list_row_gather(row, flag, entry);
}

void server_browser_list_row_populate(network_ui_widget *row, uint8_t flag1, uint8_t flag2,
                                        const char *server_name, wchar_t *map_name,
                                        const char *gametype_name,
                                        uint8_t flag3, int32_t count_a, int32_t count_b, int32_t ping)
{
    halo::networking::ServerBrowser::list_row_populate(row, flag1, flag2, server_name, map_name, gametype_name, flag3, count_a, count_b, ping);
}

int32_t server_browser_open(network_ui_widget *root)
{
    return halo::networking::ServerBrowser::open(root);
}

int32_t server_browser_player_list_populate(void *entry)
{
    return halo::networking::ServerBrowser::player_list_populate(entry);
}

void server_browser_query_results_ingest(server_list_globals *list)
{
    halo::networking::ServerBrowser::query_results_ingest(list);
}

void server_browser_result_array_sort(server_list_globals *array)
{
    halo::networking::ServerBrowser::result_array_sort(array);
}

int32_t server_browser_selected_variant_description_build(void *entry)
{
    return halo::networking::ServerBrowser::selected_variant_description_build(entry);
}

uint8_t server_browser_server_passes_filter(void *entry)
{
    return halo::networking::ServerBrowser::server_passes_filter(entry);
}

server_browser_sort_comparator server_browser_sort_comparator_select(void)
{
    return halo::networking::ServerBrowser::sort_comparator_select();
}

void server_browser_total_players_compute(server_list_globals *array)
{
    halo::networking::ServerBrowser::total_players_compute(array);
}

void server_browser_ui_refresh(void)
{
    halo::networking::ServerBrowser::ui_refresh();
}

int32_t server_list_compare_by_gametype(const void *a, const void *b)
{
    return halo::networking::ServerBrowser::compare_by_gametype(a, b);
}

int32_t server_list_compare_by_hostname(const void *a, const void *b)
{
    return halo::networking::ServerBrowser::compare_by_hostname(a, b);
}

int32_t server_list_compare_by_mapname(void **a, void **b)
{
    return halo::networking::ServerBrowser::compare_by_mapname(a, b);
}

int32_t server_list_compare_by_mapname_then_hostname(void **a, void **b)
{
    return halo::networking::ServerBrowser::compare_by_mapname_then_hostname(a, b);
}

int32_t server_list_compare_by_ping_then_hostname(void **a, void **b)
{
    return halo::networking::ServerBrowser::compare_by_ping_then_hostname(a, b);
}

int32_t server_list_compare_by_players(const void *a, const void *b)
{
    return halo::networking::ServerBrowser::compare_by_players(a, b);
}

int32_t server_list_compare_by_string_key(void **a, void **b, const char *key)
{
    return halo::networking::ServerBrowser::compare_by_string_key(a, b, key);
}

server_list_globals * server_list_mutex_try_lock(uint32_t timeout_ms)
{
    return halo::networking::ServerBrowser::mutex_try_lock(timeout_ms);
}

void server_list_mutex_unlock(server_list_globals **list_slot)
{
    halo::networking::ServerBrowser::mutex_unlock(list_slot);
}

void server_list_reset(uint8_t *entry)
{
    halo::networking::ServerBrowser::reset(entry);
}

uint32_t server_list_result_count_get(void)
{
    return halo::networking::ServerBrowser::result_count_get();
}

void server_list_result_reset(uint8_t *entry)
{
    halo::networking::ServerBrowser::result_reset(entry);
}

void server_list_scroll_clamp(server_list_globals *results)
{
    halo::networking::ServerBrowser::scroll_clamp(results);
}

void server_list_scroll_page_down(uint8_t jump_to_bottom)
{
    halo::networking::ServerBrowser::scroll_page_down(jump_to_bottom);
}

void server_list_scroll_page_up(uint8_t jump_to_top)
{
    halo::networking::ServerBrowser::scroll_page_up(jump_to_top);
}

}
