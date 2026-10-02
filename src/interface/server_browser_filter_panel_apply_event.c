// server_browser_filter_panel_apply_event  (reached only through a .data code pointer; no C existed)
// address 0x4b63c0, size 423 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b63c0..0x4b6566: ui_event_function_table slot 0x692a24, a server-browser
//   widget event. reads the filter rows of the panel (the widget  grandparent): each row  first control (type 2)
//   selection -- allow empty (== 1), allow full (== 1), ping limit (0..7), game type (0..5), team play (0..2), allow
//   unknown map (== 1); plays sound 2; on the browser (four parents up) shows its first three children and hides the
//   next two, focuses the third; leaves the filter panel and marks a query pending; 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t server_browser_allow_empty;               // 0x006953fa
extern uint8_t server_browser_allow_full;                // 0x006953fb
extern uint8_t server_browser_filter_ping_limit_index;   // 0x00719490
extern uint8_t server_browser_filter_gametype;           // 0x0071948e
extern uint8_t server_browser_filter_teamplay;           // 0x0071948f
extern uint8_t server_browser_filter_allow_unknown_map;  // 0x0071948d
extern uint8_t server_browser_filter_panel_mode;         // 0x007196b4
extern uint8_t server_browser_query_pending;             // 0x0071948a
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, AX

static void widget_show(widget_instance *widget, uint8_t shown)
{
    widget->state = shown;
    widget->hidden = !shown;
}
static widget_instance *find_control(widget_instance *row)
{
    widget_instance *child;

    for (child = row->first_child; child != 0 && child->widget_type != 2; child = child->next_sibling) {
    }
    return child;
}

static uint8_t clamp_selection(int16_t selection, int16_t maximum)
{
    if (selection < 0) {
        return 0;
    }
    return selection > maximum ? (uint8_t)maximum : (uint8_t)selection;
}

uint8_t server_browser_filter_panel_apply_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
