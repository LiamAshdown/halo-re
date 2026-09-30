// server_browser_back_event  (reached only through a .data code pointer; no C existed)
// address 0x4b6570, size 125 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b6570..0x4b65ec: ui_event_function_table slot 0x692a28, a server-browser
//   widget event. in the filter panel: back to the list (first three children shown, next two hidden, third focused),
//   sound 3, 1. Otherwise the browser closes (widget_instance_close_and_restore_previous), the event counts as
//   handled, sound 3, 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "fn_interface.h"

extern uint8_t server_browser_filter_panel_mode; // 0x007196b4
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, AX


static void widget_show(widget_instance *widget, uint8_t shown)
{
    widget->state = shown;
    widget->hidden = !shown;
}

uint8_t server_browser_back_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
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
