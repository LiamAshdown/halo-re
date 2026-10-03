#include "halo/interface/ifr2_browser.hpp"
#include "halo/cseries/api.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"

#ifdef interface
#undef interface
#endif

extern "C" {
extern uint8_t server_browser_filter_panel_mode;
extern void widget_play_sound_effect(int16_t effect_id);
extern void widget_instance_close_and_restore_previous(widget_instance *widget);
extern uint8_t server_browser_allow_empty;
extern uint8_t server_browser_allow_full;
extern uint8_t server_browser_filter_ping_limit_index;
extern uint8_t server_browser_filter_gametype;
extern uint8_t server_browser_filter_teamplay;
extern uint8_t server_browser_filter_allow_unknown_map;
extern uint8_t server_browser_query_pending;
extern int32_t server_browser_selected_index;
extern int32_t server_list_scroll_offset;
extern int32_t server_browser_last_click_ms;
extern uint8_t server_browser_skip_reselect;
extern uint8_t server_browser_player_list_ready;
extern uint32_t master_server_request_flags;
extern uint8_t server_browser_player_ticker[0x1c];
}

namespace halo::interface {

namespace {
const BackHandler k_back_event;
const ButtonHandler k_button_event;
const FilterPanelApplyHandler k_filter_panel_apply_event;
const FilterPanelCancelHandler k_filter_panel_cancel_event;
const HideWidgetHandler k_hide_widget_event;
const ListRowHandler k_list_row_event;
} // namespace

void ServerBrowserHandler::widget_show(widget_instance *widget, uint8_t shown)
{
    widget->state = shown;
    widget->hidden = !shown;
}

widget_instance * ServerBrowserHandler::find_control(widget_instance *row)
{
    widget_instance *child;

    for (child = row->first_child; child != 0 && child->widget_type != 2; child = child->next_sibling) {
    }
    return child;
}

uint8_t ServerBrowserHandler::clamp_selection(int16_t selection, int16_t maximum)
{
    if (selection < 0) {
        return 0;
    }
    return selection > maximum ? (uint8_t)maximum : (uint8_t)selection;
}

/**
 * @address 0x4b6570
 */
uint8_t BackHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
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
        halo::interface::widget_play_sound_effect(3);
        return 1;
    }
    halo::interface::widget_instance_close_and_restore_previous(widget);
    *out_handled = 1;
    halo::interface::widget_play_sound_effect(3);
    return 1;
}

/**
 * @address 0x4b7a80
 */
uint8_t ButtonHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
{
    widget_instance *parent = widget->parent;
    widget_instance *refresh = parent->first_child;
    widget_instance *reconnect = refresh->next_sibling;
    widget_instance *filters = reconnect->next_sibling;
    widget_instance *join = filters->next_sibling;

    (void)event;
    (void)out_handled;
    if (widget == refresh) {
        halo::networking::master_server_list_refresh_request();
    } else if (widget == reconnect) {
        halo::networking::master_server_ensure_list_connection();
    } else if (widget == filters) {
        halo::networking::server_browser_filter_panel_set_mode((network_ui_widget *)parent->parent->parent, 1);
    } else if (widget == join) {
        halo::networking::server_browser_latch_join_target();
    } else {
        return 0;
    }
    halo::interface::widget_play_sound_effect(2);
    return 1;
}

/**
 * @address 0x4b63c0
 */
uint8_t FilterPanelApplyHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
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
    halo::interface::widget_play_sound_effect(2);
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

/**
 * @address 0x4b6370
 */
uint8_t FilterPanelCancelHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
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

/**
 * @address 0x4b61a0
 */
uint8_t HideWidgetHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
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

/**
 * @address 0x4b7c40
 */
uint8_t ListRowHandler::handle(widget_instance *widget, int16_t *event, uint8_t *out_handled) const
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
        halo::networking::server_list_scroll_page_up(0);
        return 1;
    }
    if (widget == page_down) {
        halo::networking::server_list_scroll_page_down(0);
        return 1;
    }
    count = halo::networking::server_list_result_count_get();
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
    now = (int32_t)halo::cseries::time_query_performance_counter_ms();
    server_browser_selected_index = server_list_scroll_offset + i;
    if (old_selection == server_browser_selected_index && server_browser_last_click_ms != 0 &&
        now - server_browser_last_click_ms < 250) {
        halo::networking::server_browser_latch_join_target();
        server_browser_last_click_ms = now;
        return 1;
    }
    {
        uint16_t text[0x40];

        halo::networking::join_game_ticker_string_copy(text, 0x40, 3);
        halo::networking::ticker_text_buffer_append(0, 0, (ticker_text_buffer *)server_browser_player_ticker);
        halo::networking::ticker_text_buffer_append((wchar_t *)text, 0, (ticker_text_buffer *)server_browser_player_ticker);
    }
    server_browser_last_click_ms = now;
    server_browser_player_list_ready = 0;
    server_browser_skip_reselect = 0;
    master_server_request_flags |= 0x20;
    return 1;
}

} // namespace halo::interface

namespace halo::interface {

uint8_t server_browser_back_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_back_event.handle(widget, event, out_handled);
}

uint8_t server_browser_button_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_button_event.handle(widget, event, out_handled);
}

uint8_t server_browser_filter_panel_apply_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_filter_panel_apply_event.handle(widget, event, out_handled);
}

uint8_t server_browser_filter_panel_cancel_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_filter_panel_cancel_event.handle(widget, event, out_handled);
}

uint8_t server_browser_hide_widget_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_hide_widget_event.handle(widget, event, out_handled);
}

uint8_t server_browser_list_row_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return halo::interface::k_list_row_event.handle(widget, event, out_handled);
}

}
