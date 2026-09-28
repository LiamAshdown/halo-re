// ui_event_4a1dc0  (not a Ghidra function; ui_event_function_table[112])
// address 0x4a1dc0, size 545 bytes
// name confidence: 0.3 (named by address: the function names live only in the widget tag definitions)
// rewrite confidence: 0.85
// evidence: ui_event_function_table 0x006927d0 slot 0x00692990 (index 112); widget_instance_handle_input_event /
//   widget_close run it for a widget event. Only reachable through that table; no C existed, so it trapped as
//   unlisted_4a1dc0.
// WRITTEN 2026-09-28 from objdump 0x4a1dc0..0x4a1fe0: mouse click on a spinner: with the widget's absolute origin,
//   a cursor (0x00718f84 / 0x00718f88) inside the definition's left arrow box (+0x174 top, +0x178 bottom / right; the
//   left edge is 0) steps the selection back (wrapping to item_count - 1), sets scroll +0x42 = -4 and +0x54 = -1 and
//   plays sound 1; inside the right arrow box (+0x17c top / left, +0x180 bottom; right edge 640) steps it forward
//   (wrapping to 0) with +0x42 = 4, +0x54 = 1, sound 1. Otherwise, over a child's bounds (definition +0x24 top /
//   left, +0x28 bottom / right at its absolute origin) pushes an input event (kind 3, code 0, pressed 1) into queue 0
//   when the queue is up. Returns 1.
// blam-cc: stack -> widget, event, out_handled (cdecl); returns AL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "interface.h"
#include <string.h>

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t ui_cursor_x; // 0x00718f84
extern int32_t ui_cursor_y; // 0x00718f88
extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90, blam-cc: AX effect_id
extern uint8_t input_event_queue_active; // 0x00712cc0
extern void input_queue_push_event(int16_t queue_index, ui_input_event *record); // 0x492340, blam-cc: EAX, EDI

static void widget_absolute_origin(widget_instance *widget, int16_t *x, int16_t *y)
{
    *x = 0;
    *y = 0;
    for (; widget != 0; widget = widget->parent) {
        *x = (int16_t)(*x + widget->local_x);
        *y = (int16_t)(*y + widget->local_y);
    }
}

uint8_t ui_event_4a1dc0(widget_instance *widget, int16_t *event, uint8_t *out_handled)
{
    uint8_t *definition = (uint8_t *)tag_instances[widget->definition & 0xffff].data;
    int32_t x = ui_cursor_x;
    int32_t y = ui_cursor_y;
    int16_t origin_x;
    int16_t origin_y;
    widget_instance *child;

    widget_absolute_origin(widget, &origin_x, &origin_y);
    if (x >= 0 && x <= (int16_t)(*(int16_t *)(definition + 0x17a) + origin_x) &&
        y >= (int16_t)(*(int16_t *)(definition + 0x174) + origin_y) && y <= (int16_t)(*(int16_t *)(definition + 0x178) + origin_y)) {
        int32_t selection = widget->selection_index - 1;

        if (selection < 0) {
            selection = widget->item_count - 1;
        }
        if (selection != widget->selection_index) {
            widget->selection_index = (int16_t)selection;
            widget->scroll_blink = -4;
            widget->unknown_54 = -1;
            widget_play_sound_effect(1);
        }
        return 1;
    }
    if (x >= (int16_t)(*(int16_t *)(definition + 0x17e) + origin_x) && x <= 0x280 &&
        y >= (int16_t)(*(int16_t *)(definition + 0x17c) + origin_y) && y <= (int16_t)(*(int16_t *)(definition + 0x180) + origin_y)) {
        int32_t selection = widget->selection_index + 1;

        if (selection >= widget->item_count) {
            selection = 0;
        }
        if (selection != widget->selection_index) {
            widget->selection_index = (int16_t)selection;
            widget->scroll_blink = 4;
            widget->unknown_54 = 1;
            widget_play_sound_effect(1);
        }
        return 1;
    }
    for (child = widget->first_child; child != 0; child = child->next_sibling) {
        uint8_t *bounds = (uint8_t *)tag_instances[child->definition & 0xffff].data;
        int16_t cx;
        int16_t cy;

        widget_absolute_origin(child, &cx, &cy);
        if (x >= (int16_t)(*(int16_t *)(bounds + 0x26) + cx) && x <= (int16_t)(*(int16_t *)(bounds + 0x2a) + cx) &&
            y >= (int16_t)(*(int16_t *)(bounds + 0x24) + cy) && y <= (int16_t)(*(int16_t *)(bounds + 0x28) + cy)) {
            if (input_event_queue_active != 0) {
                ui_input_event queued;

                memset(&queued, 0, sizeof(queued));
                queued.kind = 3;
                queued.code = 0;
                queued.pressed = 1;
                input_queue_push_event(0, &queued);
            }
            return 1;
        }
    }
    return 1;
}
