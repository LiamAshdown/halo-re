// server_browser_filter_panel_cancel_event  (reached only through a .data code pointer; no C existed)
// address 0x4b6370, size 77 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b6370..0x4b63bc: ui_event_function_table slot 0x692a20, a server-browser
//   widget event. shows the widget  first three children and hides the next two, focuses the third child of the fifth
//   one  parent, and leaves the filter panel (mode 0); 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t server_browser_filter_panel_mode; // 0x007196b4

static void widget_show(widget_instance *widget, uint8_t shown)
{
    widget->state = shown;
    widget->hidden = !shown;
}

uint8_t server_browser_filter_panel_cancel_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
