exec(open(r'C:\Users\Liam-\halo-re\scratchpad\md_lib.py').read())
HDR = '#include "tags.h"\n#include "memory.h"\n#include "math.h"\n#include "interface.h"\n'
SIG = '(widget_instance *widget, int16_t *event, uint8_t *out_handled)'


def ui(addr, size, name, table_slot, note, code, cc='stack -> widget, event, out_handled (cdecl); returns AL'):
    emit(addr, size, name, 'ui_event_function_table slot 0x%x, a server-browser widget event. %s' % (table_slot, note),
         code, cc=cc, module='interface', header=HDR)


SHOW = '''static void widget_show(widget_instance *widget, uint8_t shown)
{
    widget->state = shown;
    widget->hidden = !shown;
}
'''
ui(0x4b61a0, 27, 'server_browser_hide_widget_event', 0x692a00, 'hides the widget (state 0, hidden) and moves its parent  focus to the second child; 1.', '''
uint8_t server_browser_hide_widget_event''' + SIG + '''
{
    widget_instance *parent;

    (void)event;
    (void)out_handled;
    widget->state = 0;
    widget->hidden = 1;
    parent = widget->parent;
    parent->focused_child = parent->first_child->next_sibling;
    return 1;
}
''')
ui(0x4b6370, 77, 'server_browser_filter_panel_cancel_event', 0x692a20, 'shows the widget  first three children and hides the next two, focuses the third child of the fifth one  parent, and leaves the filter panel (mode 0); 1.', '''
extern uint8_t server_browser_filter_panel_mode; // 0x007196b4

''' + SHOW + '''
uint8_t server_browser_filter_panel_cancel_event''' + SIG + '''
{
    widget_instance *child = widget->first_child;
    widget_instance *parent;

    (void)event;
    (void)out_handled;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 0);
    child = child->next_sibling;
    widget_show(child, 0);
    parent = child->parent;
    parent->focused_child = parent->first_child->next_sibling->next_sibling;
    server_browser_filter_panel_mode = 0;
    return 1;
}
''')
FIND = '''static widget_instance *find_control(widget_instance *row)
{
    widget_instance *child;

    for (child = row->first_child; child != 0 && child->widget_type != 2; child = child->next_sibling) {
    }
    return child;
}
'''
ui(0x4b63c0, 423, 'server_browser_filter_panel_apply_event', 0x692a24, 'reads the filter rows of the panel (the widget  grandparent): each row  first control (type 2) selection -- allow empty (== 1), allow full (== 1), ping limit (0..7), game type (0..5), team play (0..2), allow unknown map (== 1); plays sound 2; on the browser (four parents up) shows its first three children and hides the next two, focuses the third; leaves the filter panel and marks a query pending; 1.', '''
extern uint8_t server_browser_allow_empty;               // 0x006953fa
extern uint8_t server_browser_allow_full;                // 0x006953fb
extern uint8_t server_browser_filter_ping_limit_index;   // 0x00719490
extern uint8_t server_browser_filter_gametype;           // 0x0071948e
extern uint8_t server_browser_filter_teamplay;           // 0x0071948f
extern uint8_t server_browser_filter_allow_unknown_map;  // 0x0071948d
extern uint8_t server_browser_filter_panel_mode;         // 0x007196b4
extern uint8_t server_browser_query_pending;             // 0x0071948a
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, AX

''' + SHOW + FIND + '''
static uint8_t clamp_selection(int16_t selection, int16_t maximum)
{
    if (selection < 0) {
        return 0;
    }
    return selection > maximum ? (uint8_t)maximum : (uint8_t)selection;
}

uint8_t server_browser_filter_panel_apply_event''' + SIG + '''
{
    widget_instance *row = widget->parent->parent->first_child;
    widget_instance *child;
    widget_instance *parent;

    (void)event;
    (void)out_handled;
    server_browser_allow_empty = find_control(row)->selection_index == 1;
    row = row->next_sibling;
    server_browser_allow_full = find_control(row)->selection_index == 1;
    row = row->next_sibling;
    server_browser_filter_ping_limit_index = clamp_selection(find_control(row)->selection_index, 7);
    row = row->next_sibling;
    server_browser_filter_gametype = clamp_selection(find_control(row)->selection_index, 5);
    row = row->next_sibling;
    server_browser_filter_teamplay = clamp_selection(find_control(row)->selection_index, 2);
    row = row->next_sibling;
    server_browser_filter_allow_unknown_map = find_control(row)->selection_index == 1;
    widget_play_sound_effect(2);
    child = widget->parent->parent->parent->parent->first_child;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 1);
    child = child->next_sibling;
    widget_show(child, 0);
    child = child->next_sibling;
    widget_show(child, 0);
    parent = child->parent;
    parent->focused_child = parent->first_child->next_sibling->next_sibling;
    server_browser_filter_panel_mode = 0;
    server_browser_query_pending = 1;
    return 1;
}
''')
ui(0x4b6570, 125, 'server_browser_back_event', 0x692a28, 'in the filter panel: back to the list (first three children shown, next two hidden, third focused), sound 3, 1. Otherwise the browser closes (widget_instance_close_and_restore_previous), the event counts as handled, sound 3, 1.', '''
extern uint8_t server_browser_filter_panel_mode; // 0x007196b4
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, AX
extern void widget_instance_close_and_restore_previous(widget_instance *widget); // 0x49c3e0, EAX

''' + SHOW + '''
uint8_t server_browser_back_event''' + SIG + '''
{
    (void)event;
    if (server_browser_filter_panel_mode != 0) {
        widget_instance *child = widget->first_child;
        widget_instance *parent;

        widget_show(child, 1);
        child = child->next_sibling;
        widget_show(child, 1);
        child = child->next_sibling;
        widget_show(child, 1);
        child = child->next_sibling;
        widget_show(child, 0);
        child = child->next_sibling;
        widget_show(child, 0);
        parent = child->parent;
        parent->focused_child = parent->first_child->next_sibling->next_sibling;
        server_browser_filter_panel_mode = 0;
        widget_play_sound_effect(3);
        return 1;
    }
    widget_instance_close_and_restore_previous(widget);
    *out_handled = 1;
    widget_play_sound_effect(3);
    return 1;
}
''')
ui(0x4b7920, 341, 'server_browser_closed_event', 0x6929a4, 'the browser closing: a running browser stops (waiting for the master-server thread, or freeing the ServerBrowser); the server list and its block are reset, both tickers emptied, autopatch downloads shut down, the join target cleared (-1). With a profile selected, the browser settings (sort column / order, password, dedicated, classic, unknown map, empty, full, game type, team play, ping) go into the working copy (+0xc80..+0xc8a; only when the selected item is a profile) and it is saved when changed, else the selection is dropped; 1.', '''
extern uint8_t server_browser_initialized;                // 0x00719470
extern void *server_list_thread;                          // 0x007196ac
extern void *master_server_query_engine;                  // 0x0071946c
extern void *server_list_block;                           // 0x007196bc
extern int32_t server_list_block_used;                    // 0x007196c0
extern int32_t server_list_block_capacity;                // 0x007196c4
extern int32_t server_browser_query_elapsed_ms;           // 0x007196c8
extern uint8_t server_browser_player_ticker[0x1c];        // 0x006b5e58
extern uint8_t server_browser_variant_ticker[0x1c];       // 0x006b5e74
extern int32_t server_browser_join_target;                // 0x00695420
extern int32_t saved_game_profile_index;                  // 0x00714dd4
extern int32_t selected_saved_item;                       // 0x00714e7c
extern uint8_t saved_item_working_copy[0x1ffc];           // 0x00714e80
extern uint8_t server_browser_sort_column;                // 0x00719489
extern uint8_t server_browser_sort_ascending;             // 0x006953f8
extern uint8_t server_browser_allow_password;             // 0x006953f9
extern uint8_t server_browser_filter_dedicated_only;      // 0x0071948b
extern uint8_t server_browser_filter_classic_only;        // 0x0071948c
extern uint8_t server_browser_filter_allow_unknown_map;   // 0x0071948d
extern uint8_t server_browser_allow_empty;                // 0x006953fa
extern uint8_t server_browser_allow_full;                 // 0x006953fb
extern uint8_t server_browser_filter_gametype;            // 0x0071948e
extern uint8_t server_browser_filter_teamplay;            // 0x0071948f
extern uint8_t server_browser_filter_ping_limit_index;    // 0x00719490
extern void master_server_connection_wait_thread(void);  // 0x4b6070
extern void ServerBrowserFree(void *sb);                  // 0x616f30
extern void server_list_reset(uint8_t *entry);            // 0x4b65f0, EAX
extern void *__stdcall GlobalFree(void *memory);
extern void ticker_text_buffer_reset(void *self);         // 0x4b8a00, EDI
extern uint32_t autopatch_download_pool_shutdown(void);   // 0x576db0
extern void saved_item_select(int32_t item);              // 0x495be0, EBX
extern uint8_t saved_item_has_unsaved_changes(void);      // 0x495ea0
extern uint8_t player_profile_save(void);                 // 0x495d40

uint8_t server_browser_closed_event''' + SIG + '''
{
    int32_t profile;
    uint8_t *working;

    (void)widget;
    (void)event;
    (void)out_handled;
    if (server_browser_initialized) {
        if (server_list_thread != 0) {
            master_server_connection_wait_thread();
        } else {
            ServerBrowserFree(master_server_query_engine);
            master_server_query_engine = 0;
        }
    }
    server_browser_initialized = 0;
    server_list_reset(0);
    if (server_list_block != 0) {
        GlobalFree(server_list_block);
    }
    server_list_block = 0;
    server_list_block_used = 0;
    server_list_block_capacity = 0;
    server_browser_query_elapsed_ms = 0;
    ticker_text_buffer_reset(server_browser_player_ticker);
    ticker_text_buffer_reset(server_browser_variant_ticker);
    autopatch_download_pool_shutdown();
    profile = saved_game_profile_index;
    server_browser_join_target = -1;
    if (profile == -1) {
        return 1;
    }
    saved_item_select(profile);
    working = (selected_saved_item & 0xf) == 0 ? saved_item_working_copy : 0;
    working[0xc81] = server_browser_sort_ascending;
    working[0xc80] = server_browser_sort_column;
    working[0xc82] = server_browser_allow_password;
    working[0xc83] = server_browser_filter_dedicated_only;
    working[0xc84] = server_browser_filter_classic_only;
    working[0xc85] = server_browser_filter_allow_unknown_map;
    working[0xc86] = server_browser_allow_empty;
    working[0xc87] = server_browser_allow_full;
    working[0xc88] = server_browser_filter_gametype;
    working[0xc89] = server_browser_filter_teamplay;
    working[0xc8a] = server_browser_filter_ping_limit_index;
    if (saved_item_has_unsaved_changes()) {
        player_profile_save();
        return 1;
    }
    selected_saved_item = -1;
    return 1;
}
''')
ui(0x4b7a80, 152, 'server_browser_button_event', 0x6929b0, 'the button row (the parent  first four children): refresh (master_server_list_refresh_request), reconnect (master_server_ensure_list_connection), filters (the filter panel on the browser three parents up, internet mode), join (server_browser_latch_join_target) -- each with sound 2 and 1; another widget: 0.', '''
extern void master_server_list_refresh_request(void);      // 0x4b6660
extern void master_server_ensure_list_connection(void);    // 0x4b66c0
extern void server_browser_filter_panel_set_mode(void *panel, uint8_t internet_mode); // 0x4b61c0, EAX panel
extern void server_browser_latch_join_target(void);        // 0x4b6730
extern void widget_play_sound_effect(int16_t effect_id);  // 0x498e90, AX

uint8_t server_browser_button_event''' + SIG + '''
{
    widget_instance *parent = widget->parent;
    widget_instance *refresh = parent->first_child;
    widget_instance *reconnect = refresh->next_sibling;
    widget_instance *filters = reconnect->next_sibling;
    widget_instance *join = filters->next_sibling;

    (void)event;
    (void)out_handled;
    if (widget == refresh) {
        master_server_list_refresh_request();
    } else if (widget == reconnect) {
        master_server_ensure_list_connection();
    } else if (widget == filters) {
        server_browser_filter_panel_set_mode(parent->parent->parent, 1);
    } else if (widget == join) {
        server_browser_latch_join_target();
    } else {
        return 0;
    }
    widget_play_sound_effect(2);
    return 1;
}
''')
ui(0x4b7c40, 319, 'server_browser_list_row_event', 0x6929ac, 'the server list (the parent  children: two headers, page up, 15 rows, page down): a key-up of kind 4 / 3 is ignored; page up / down scroll; a row within the results selects scroll offset + row -- a second click on the same row within 250 ms latches the join target, otherwise the player ticker restarts with ticker string 3 and a player list request (flag 0x20) goes out; 1.', '''
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

uint8_t server_browser_list_row_event''' + SIG + '''
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
''')
print('ok')
