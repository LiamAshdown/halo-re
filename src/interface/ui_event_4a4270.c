// ui_event_4a4270  (not a Ghidra function; ui_event_function_table[175])
// address 0x4a4270, size 14 bytes
// name confidence: 0.4 (named by address; it is the campaign menu's CONTINUE handler: it loads "savegame")
// rewrite confidence: 0.9
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a8c (index 175); widget_instance_handle_input_event
//   runs it for a widget event. Only reachable through that table. First-boot track: CAMPAIGN -> CONTINUE.
// objdump 0x4a4270..0x4a427d: saved_game_load_checkpoint_by_name("savegame" 0x0066a56c) and returns its AL.
// blam-cc: stack -> widget, event, out_handled (cdecl, none read); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

extern uint8_t saved_game_load_checkpoint_by_name(char *name); // 0x5391a0

uint8_t ui_event_4a4270(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    return saved_game_load_checkpoint_by_name("savegame");
}
