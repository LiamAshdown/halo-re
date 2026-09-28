// ui_event_4a3540  (not a Ghidra function; ui_event_function_table[160])
// address 0x4a3540, size 577 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692a50 (index 160); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a3540.
// WRITTEN 2026-09-28 from objdump 0x4a3540..0x4a3780: a click on a row of a scrolling list (the parent): the
//   definition's row count (+0x3e0, less one with a spinner-list header row) decides whether the list overflows
//   (definition flag bit 3 clear and item_count above rows - 1), leaving rows - 3 (arrows shown) or rows - 1 visible
//   data rows (at most item_count). The widget's position among the parent's children (the last child is never
//   matched) maps to a data row starting at the first visible item (+0x3e). The up arrow slot (first child, or second
//   with a header) scrolls up by the visible rows (floor 0); the down arrow slot (second to last child) scrolls down
//   (capped at item_count - rows); both with sound 2. Other rows (or any row when not overflowing) play sound 2 and
//   commit the row (+0x3c); a double click (event kind 3 with code 0 or 0xc, more than 250 ms after the widget's
//   creation time, on the already committed row) pushes an event of kind 5 into queue 0. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>
#include "objects.h"
#include "units.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t time_query_performance_counter_ms(void); // 0x449210
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern uint8_t input_event_queue_active; // 0x00712cc0
extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, blam-cc: EAX, EDI

static void row_clicked(widget_instance *list, int32_t row, int32_t old_committed, uint8_t double_click)
{
    widget_play_sound_effect(2);
    *(int16_t *)&((struct widget_instance *)list)->text = (int16_t)row;
    if (double_click && old_committed == row) {
        ui_input_event queued;

        memset(&queued, 0, sizeof(queued));
        queued.kind = 5;
        input_queue_push_event(0, &queued);
    }
}

uint8_t ui_event_4a3540(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    widget_instance *list = widget->parent;
    uint8_t *definition = (uint8_t *)tag_instances[list->definition & 0xffff].data;
    int32_t rows = *(int32_t *)(definition + 0x3e0);
    int32_t first_visible = *(int16_t *)((uint8_t *)list + 0x3e);
    int32_t committed = *(int16_t *)&((struct widget_instance *)list)->text;
    widget_instance *child = list->first_child;
    uint8_t header = (uint8_t)(child != 0 && child->first_child != 0 && child->first_child->widget_type == 2);
    uint8_t double_click = 0;
    uint8_t fits;
    int32_t shown;
    int32_t row;
    int32_t position;

    if (event[0] == 3 && (((uint8_t *)event)[4] == 0 || ((uint8_t *)event)[4] == 0xc) &&
        time_query_performance_counter_ms() - (uint32_t)widget->creation_time > 0xfa) {
        double_click = 1;
    }
    if (header) {
        rows--;
    }
    fits = (uint8_t)((*(uint8_t *)(definition + 0x150) & 8) != 0 || (int32_t)list->item_count <= rows - 1);
    shown = rows - (fits ? 1 : 3);
    if (shown > (int32_t)list->item_count) {
        shown = list->item_count;
    }
    row = first_visible;
    for (position = 0; child != 0 && child->next_sibling != 0; position++, row++, child = child->next_sibling) {
        if (child == widget) {
            break;
        }
        if ((position == 0 && (header || !fits)) || (position == 1 && !fits && header)) {
            row--;
        }
    }
    if (child == 0 || child->next_sibling == 0) {
        return 1;
    }
    if ((position == 0 && !header) || (position == 1 && header)) {
        if (fits) {
            row_clicked(list, row, committed, double_click);
            return 1;
        }
        first_visible = first_visible + 1 - shown;
        if (first_visible < 0) {
            first_visible = 0;
        }
        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)first_visible;
        widget_play_sound_effect(2);
        return 1;
    }
    if (child->next_sibling->next_sibling == 0 && !fits) {
        int32_t last = first_visible + shown - 1;

        if (last >= (int32_t)list->item_count - shown) {
            last = (int32_t)list->item_count - shown;
        }
        *(int16_t *)((uint8_t *)list + 0x3e) = (int16_t)last;
        widget_play_sound_effect(2);
        return 1;
    }
    row_clicked(list, row, committed, double_click);
    return 1;
}
