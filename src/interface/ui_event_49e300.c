// ui_event_49e300  (not a Ghidra function; ui_event_function_table[42])
// address 0x49e300, size 626 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692878 (index 42); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_49e300.
// WRITTEN 2026-09-28 from objdump 0x49e300..0x49e571: for a selected variant, from the grandparent's first six
//   spinner lists: 0 / 1 set byte +0x7c to 1 / 0; 0..5 set dword +0x80 to 0, 0x708, 0xe10, 0x1518, 0x2328, 0x4650; 0
//   / 1 set byte +0x7e to 1 / 0; the same for +0x7f; 0..4 set dword +0x58 to 1, 3, 5, 10, 15; 0..6 set dword +0x78 to
//   0, 0x4650 .. 0x13c68 (other selections leave them); then pops the grandparent controller's widget history node
//   (the block free inlined, as 0x4a1b00). Returns 1, also without a variant (then nothing is popped).
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern int32_t selected_saved_item; // 0x00714e7c, low nibble: 0 profile, 1 variant
extern uint8_t saved_item_working_copy[0x1ffc]; // 0x00714e80, the record itself
extern widget_history_node *ui_widget_history[3]; // 0x00718f98
extern heap *widget_memory_pool; // 0x006926c4
extern void heap_unlink_block(heap_block *block, heap *self); // 0x4d20a0

static widget_instance *first_list_child(widget_instance *widget)
{
    widget_instance *child = widget->first_child;

    while (child != 0 && child->widget_type != 2) {
        child = child->next_sibling;
    }
    return child;
}

static void widget_history_pop(int16_t controller)
{
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
}

uint8_t ui_event_49e300(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *variant = (selected_saved_item & 0xf) == 1 ? saved_item_working_copy : 0;
    static const int32_t delays[] = {0, 0x708, 0xe10, 0x1518, 0x2328, 0x4650};
    static const int32_t lives[] = {1, 3, 5, 10, 15};
    static const int32_t times[] = {0, 0x4650, 0x6978, 0x8ca0, 0xafc8, 0xd2f0, 0x13c68};
    widget_instance *parent = widget->parent->parent;
    widget_instance *group;
    int16_t selection;

    if (variant == 0) {
        return 1;
    }
    group = parent->first_child;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        variant[0x7c] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 5) {
        ((struct game_variant *)variant)->ctf_value_80 = delays[selection];
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        variant[0x7e] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection == 0 || selection == 1) {
        variant[0x7f] = (uint8_t)(selection == 0);
    }
    group = group->next_sibling;
    selection = first_list_child(group)->selection_index;
    if (selection >= 0 && selection <= 4) {
        ((struct game_variant *)variant)->score_limit = lives[selection];
    }
    selection = first_list_child(group->next_sibling)->selection_index;
    if (selection >= 0 && selection <= 6) {
        ((struct game_variant *)variant)->unknown_78 = times[selection];
    }
    widget_history_pop(parent->controller_index);
    return 1;
}
