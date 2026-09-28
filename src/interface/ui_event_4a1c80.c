// ui_event_4a1c80  (not a Ghidra function; a ui_event_function_table entry; no C existed, so it trapped as
//   unlisted_4a1c80)
// address 0x4a1c80, size 23 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x4a1c80..0x4a1c96: clears the byte at 0x719757, sets the byte at 0x71975b and
//   the dword at 0x7196d4 to 1, and returns 1 (EAX = 1). The arguments are not read.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

extern uint8_t split_screen_quit_prompt_armed;  // 0x00719757
extern uint8_t ui_event_byte_0071975b;  // 0x0071975b
extern int32_t movie_playback_abort; // 0x007196d4

uint8_t ui_event_4a1c80(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    split_screen_quit_prompt_armed = 0;
    ui_event_byte_0071975b = 1;
    movie_playback_abort = 1;
    return 1;
}
