// ui_event_4a3d40  (not a Ghidra function; ui_event_function_table[171])
// address 0x4a3d40, size 74 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a7c (index 171); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3d40.
// WRITTEN 2026-09-28 from objdump 0x4a3d40..0x4a3d89: resets the hud text message queue (first two dwords -1,
//   GlobalFree the data and clear it); a positive message cycle state (0x00719230) restarts the main menu music with
//   (state == 2); the state becomes 0; returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern growable_array hud_text_message_queue; // 0x006b37e8
extern void *__stdcall GlobalFree(void *memory); // 0x0063a0bc IAT
extern int32_t hud_text_message_cycle_state_00719230; // 0x00719230, compared as a dword
extern void chimera__main_menu_music(uint8_t finalize_render_frame); // 0x4921a0, blam-cc: BL

uint8_t ui_event_4a3d40(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    hud_text_message_queue.element_size = -1;
    hud_text_message_queue.count = -1;
    if (hud_text_message_queue.data != 0) {
        GlobalFree(hud_text_message_queue.data);
        hud_text_message_queue.data = 0;
    }
    if (hud_text_message_cycle_state_00719230 > 0) {
        chimera__main_menu_music((uint8_t)(hud_text_message_cycle_state_00719230 == 2));
    }
    hud_text_message_cycle_state_00719230 = 0;
    return 1;
}
