// ui_game_data_input_4a4c70  (not a Ghidra function; game_data_input_function_table[1])
// address 0x4a4c70, size 58 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b1c (index 1); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a4c70.
// WRITTEN 2026-09-28 from objdump 0x4a4c70..0x4a4ca9: counts the widget's children up to its focused child (all of
//   them when none is focused; 0 with no children) and stores the count as the background frame of the extended
//   description's first child and the selection of that child's next sibling (skipped when the count is 0xffff).
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void ui_game_data_input_4a4c70(widget_instance *widget)
{
    widget_instance *label = widget->extended_description->first_child;
    widget_instance *list = label->next_sibling;
    widget_instance *child = widget->first_child;
    int16_t index = 0;

    if (child != 0) {
        for (; child != 0 && child != widget->focused_child; child = child->next_sibling) {
            index++;
        }
        if ((uint16_t)index == 0xffff) {
            return;
        }
    }
    label->background_bitmap_frame = index;
    list->selection_index = index;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
