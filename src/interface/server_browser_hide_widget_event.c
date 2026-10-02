// server_browser_hide_widget_event  (reached only through a .data code pointer; no C existed)
// address 0x4b61a0, size 27 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4b61a0..0x4b61ba: ui_event_function_table slot 0x692a00, a server-browser
//   widget event. hides the widget (state 0, hidden) and moves its parent  focus to the second child; 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint8_t server_browser_hide_widget_event(widget_instance *widget, int16_t *event, uint8_t *out_handled)
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
