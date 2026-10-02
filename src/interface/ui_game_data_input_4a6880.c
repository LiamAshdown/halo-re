// ui_game_data_input_4a6880  (not a Ghidra function; game_data_input_function_table[20])
// address 0x4a6880, size 103 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: game_data_input_function_table 0x00692b18 slot 0x00692b68 (index 20); widget_instance_render
//   0x49a8c0 runs it once per frame for each game_data_inputs entry. Only reachable through that table; no C
//   existed, so it trapped as unlisted_4a6880.
// WRITTEN 2026-09-28 from objdump 0x4a6880..0x4a68e6: with a focused child: sums, over the children before the
//   focused one, the item count (+0x48) of each child's first spinner list (type 2), plus the focused child's list
//   selection (+0x40), into the extended description's background frame (+0x58). Without a focused child the binary
//   stores the low word of its own stack argument (the widget pointer); kept.
// blam-cc: stack -> widget (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

void ui_game_data_input_4a6880(widget_instance *widget)
{
    widget_instance *focused = widget->focused_child;
    widget_instance *label = widget->extended_description;
    widget_instance *child;
    int16_t sum = 0;

    if (focused == 0) {
        label->background_bitmap_frame = (int16_t)(uint32_t)widget;
        return;
    }
    for (child = widget->first_child; child != 0; child = child->next_sibling) {
        widget_instance *list = first_list_child(child);

        if (child == focused) {
            sum = (int16_t)(sum + list->selection_index);
            break;
        }
        sum = (int16_t)(sum + list->item_count);
    }
    label->background_bitmap_frame = sum;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
