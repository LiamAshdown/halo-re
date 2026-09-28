// ui_game_data_input_4b5ce0  (not a Ghidra function; game_data_input_function_table[47])
// address 0x4b5ce0, size 134 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692bd4 (index 47); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4b5ce0.
// WRITTEN 2026-09-28 from objdump 0x4b5ce0..0x4b5d65: refreshes the gamepad lists and collects the 17 screen nodes;
//   nodes 15 and 14 show frame 1 when focused; node 16 gets selection 0 and state 1 when node 0 is focused, selection
//   1 and state 1 when node 5 is, else state 0.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern void controls_gamepad_lists_refresh(widget_instance *screen); // 0x4b55d0, blam-cc: ECX
extern void controls_gamepad_widget_nodes_collect(widget_instance **out, widget_instance *screen); // 0x4b5560, blam-cc: EAX out, ECX screen

void ui_game_data_input_4b5ce0(widget_instance *widget)
{
    widget_instance *nodes[17];
    widget_instance *focused;

    controls_gamepad_lists_refresh(widget);
    controls_gamepad_widget_nodes_collect(nodes, widget);
    nodes[15]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[15]);
    nodes[14]->background_bitmap_frame = (int16_t)(widget->focused_child == nodes[14]);
    focused = widget->focused_child;
    if (focused == nodes[0]) {
        nodes[16]->selection_index = 0;
        nodes[16]->state = 1;
    } else if (focused == nodes[5]) {
        nodes[16]->selection_index = 1;
        nodes[16]->state = 1;
    } else {
        nodes[16]->state = 0;
    }
}
