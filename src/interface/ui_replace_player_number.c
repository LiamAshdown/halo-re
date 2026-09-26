// ui_replace_player_number  (not a Ghidra function; ui_replace_function_table[1])
// address 0x4a8840, size 141 bytes
// name confidence: 0.6  rewrite confidence: 1.0
// evidence: ui_replace_function_table 0x00692c08 (the widget text search/replace callbacks 0x4a8750, 0x4a8840,
//   0x4a8760, 0x4a8810); only reachable through that table.
// objdump 0x4a8840..0x4a88cc (table 0x4a88d0): the widget's controller index (+0x08) + 1 selects the text: -1 and 0
//   give "1", 1..3 give "2".."4", anything else "?"; written with its terminator into the wide buffer 0x006b3854.
// blam-cc: stack -> widget (cdecl); returns EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

extern uint16_t ui_player_number_text[2]; // 0x006b3854

void *ui_replace_player_number(widget_instance *widget)
{
    uint16_t digit;

    switch (widget->controller_index) {
    case -1: case 0: digit = '1'; break;
    case 1: digit = '2'; break;
    case 2: digit = '3'; break;
    case 3: digit = '4'; break;
    default: digit = '?'; break;
    }
    ui_player_number_text[0] = digit;
    ui_player_number_text[1] = 0;
    return ui_player_number_text;
}
