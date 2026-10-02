// server_browser_list_row_event  (reached only through a .data code pointer; no C existed)
// address 0x4b7c40, size 319 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b7c40..0x4b7d7e: ui_event_function_table slot 0x6929ac, a server-browser
//   widget event. the server list (the parent  children: two headers, page up, 15 rows, page down): a key-up of kind
//   4 / 3 is ignored; page up / down scroll; a row within the results selects scroll offset + row -- a second click
//   on the same row within 250 ms latches the join target, otherwise the player ticker restarts with ticker string 3
//   and a player list request (flag 0x20) goes out; 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t server_browser_selected_index;             // 0x006953f4
extern int32_t server_list_scroll_offset;                 // 0x00719478
extern int32_t server_browser_last_click_ms;              // 0x0071947c
extern uint8_t server_browser_skip_reselect;              // 0x00719480
extern uint8_t server_browser_player_list_ready;          // 0x00719481
extern uint32_t master_server_request_flags;              // 0x0071969c
extern uint8_t server_browser_player_ticker[0x1c];        // 0x006b5e58
extern void server_list_scroll_page_up(uint8_t jump_to_top);   // 0x4b7b20, AL
extern void server_list_scroll_page_down(uint8_t jump_to_bottom); // 0x4b7bb0, AL
extern uint32_t server_list_result_count_get(void);       // 0x4ba820
extern uint32_t time_query_performance_counter_ms(void);  // 0x449210
extern void server_browser_latch_join_target(void);       // 0x4b6730
extern void join_game_ticker_string_copy(uint16_t *buffer, int32_t capacity, int32_t string_index); // 0x4b6160, ESI, EBX
extern void ticker_text_buffer_append(uint16_t *text, int32_t reset_column, void *self); // 0x4b8a60, EDI self

uint8_t server_browser_list_row_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *header = widget->parent->first_child->next_sibling;
    widget_instance *page_up = header->next_sibling;
    widget_instance *page_down;
    widget_instance *rows[15];
    uint32_t count;
    int32_t old_selection;
    int32_t now;
    int32_t i;

    (void)out_handled;
    page_down = page_up->next_sibling;
    for (i = 0; i < 15; i++) {
        rows[i] = page_down;
        page_down = page_down->next_sibling;
    }
    if (event[0] == 4 && ((uint8_t *)event)[4] == 3) {
        return 1;
    }
    if (widget == page_up) {
        server_list_scroll_page_up(0);
        return 1;
    }
    if (widget == page_down) {
        server_list_scroll_page_down(0);
        return 1;
    }
    count = server_list_result_count_get();
    if (count == 0) {
        return 1;
    }
    for (i = 0; i < 15; i++) {
        if (rows[i] == widget && i < (int32_t)count) {
            break;
        }
    }
    if (i == 15) {
        return 1;
    }
    old_selection = server_browser_selected_index;
    now = (int32_t)time_query_performance_counter_ms();
    server_browser_selected_index = server_list_scroll_offset + i;
    if (old_selection == server_browser_selected_index && server_browser_last_click_ms != 0 &&
        now - server_browser_last_click_ms < 250) {
        server_browser_latch_join_target();
        server_browser_last_click_ms = now;
        return 1;
    }
    {
        uint16_t text[0x40];

        join_game_ticker_string_copy(text, 0x40, 3);
        ticker_text_buffer_append(0, 0, server_browser_player_ticker);
        ticker_text_buffer_append(text, 0, server_browser_player_ticker);
    }
    server_browser_last_click_ms = now;
    server_browser_player_list_ready = 0;
    server_browser_skip_reselect = 0;
    master_server_request_flags |= 0x20;
    return 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
