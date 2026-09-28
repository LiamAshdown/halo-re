// ui_event_4a1b00  (not a Ghidra function; ui_event_function_table[98])
// address 0x4a1b00, size 82 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692958 (index 98); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1b00.
// WRITTEN 2026-09-28 from objdump 0x4a1b00..0x4a1b51: pops the head of the widget history for the widget's
//   controller (-1 means 0) and frees that node's widget pool block, updating the pool byte and allocation counts;
//   returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"

extern widget_history_node *ui_widget_history[3]; // 0x00718f98
extern heap *widget_memory_pool; // 0x006926c4
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0

uint8_t ui_event_4a1b00(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    int16_t controller = widget->controller_index;
    widget_history_node *node;

    if (controller == -1) {
        controller = 0;
    }
    node = ui_widget_history[controller];
    if (node != 0) {
        heap_block *block = (heap_block *)((uint8_t *)node - 0x10);
        uint32_t size = block->size & 0x7fffffff;

        ui_widget_history[controller] = node->next;
        heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
    }
    return 1;
}
