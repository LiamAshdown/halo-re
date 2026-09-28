// server_browser_button_event  (reached only through a .data code pointer; no C existed)
// address 0x4b7a80, size 152 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b7a80..0x4b7b17: ui_event_function_table slot 0x6929b0, a server-browser
//   widget event. the button row (the parent  first four children): refresh (master_server_list_refresh_request),
//   reconnect (master_server_ensure_list_connection), filters (the filter panel on the browser three parents up,
//   internet mode), join (server_browser_latch_join_target) -- each with sound 2 and 1; another widget: 0.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

extern void master_server_list_refresh_request(void);      // 0x4b6660
extern void master_server_ensure_list_connection(void);    // 0x4b66c0
extern void server_browser_filter_panel_set_mode(void *panel, uint8_t internet_mode); // 0x4b61c0, EAX panel
extern void server_browser_latch_join_target(void);        // 0x4b6730
extern void widget_play_sound_effect(int16_t effect_id);  // 0x498e90, AX

uint8_t server_browser_button_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
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
