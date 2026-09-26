// ui_game_data_input_4a7660  (not a Ghidra function; game_data_input_function_table[0x2e])
// address 0x4a7660, size 107 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.9
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692bd0 (index 0x2e); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table.
//   First-boot track: runs when CAMPAIGN -> NEW GAME builds the next screen.
// objdump 0x4a7660..0x4a76ca: when the widget is not its parent's focused child, its own focused child is cleared
//   and every child's background bitmap frame (+0x58) set to 0. When it is the parent's focused child and has no
//   focused child yet, the first child becomes the focused child and every child with a two-frame background
//   bitmap (+0x5e == 2) shows frame 1 if it is the focused child, frame 0 otherwise.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"

void ui_game_data_input_4a7660(widget_instance *widget)
{
    widget_instance *child;

    if (widget->parent->focused_child != widget) {
        child = widget->first_child;
        widget->focused_child = 0;
        for (; child != 0; child = child->next_sibling) {
            child->background_bitmap_frame = 0;
        }
        return;
    }
    if (widget->focused_child != 0) {
        return;
    }
    child = widget->first_child;
    widget->focused_child = child;
    for (; child != 0; child = child->next_sibling) {
        if (child == widget->focused_child) {
            if (child->background_bitmap_frames == 2) {
                child->background_bitmap_frame = 1;
            }
        } else if (child->background_bitmap_frames == 2) {
            child->background_bitmap_frame = 0;
        }
    }
}
