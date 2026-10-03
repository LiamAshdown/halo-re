#include "halo/interface/ifr1_hud_text_queue.hpp"

extern "C" {
extern Globals *global_globals;
extern uint16_t hud_text_draw_color_or_flags;
extern int16_t hud_text_draw_column;
extern uint32_t hud_text_draw_unknown_4730;
extern int32_t hud_text_draw_font_tag_id;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern void globals_color_table_get_cyclic_color(int16_t table_index, int16_t color_index,
                                                   ColorARGB *out);
extern growable_array hud_text_message_queue;
extern uint16_t empty_string[];
extern int32_t growable_array_add_element(growable_array *array);
extern int64_t performance_frequency;
extern int32_t hud_text_message_time_base;
}

namespace halo::interface {

/**
 * color_index) Configures the shared HUD text-draw state for a subsequent draw call: resolves
 * font_table_index's globals TagDependency tag id as the font, resolves color_table_index's cyclic color entry
 * at color_index as the color, and copies the three remaining flag/column arguments straight through.
 * blam-cc: stack -> (font_table_index, color_or_flags, column, unknown_4730, color_table_index,
 *
 * @address 0x4944c0
 */
void HudTextQueue::draw_configure(int16_t font_table_index, uint16_t color_or_flags, int16_t column, uint32_t unknown_4730, int16_t color_table_index, int16_t color_index)
{
    GlobalsInterfaceBitmaps *interface_bitmaps;
    TagDependency *dependency;
    ColorARGB color;

    globals_color_table_get_cyclic_color(color_table_index, color_index, &color);

    interface_bitmaps = (global_globals->interface_bitmaps.count == 0)
                             ? (GlobalsInterfaceBitmaps *)0
                             : (GlobalsInterfaceBitmaps *)global_globals->interface_bitmaps.pointer;
    dependency = (TagDependency *)((char *)interface_bitmaps + font_table_index * 0x10);
    hud_text_draw_font_tag_id = *(int32_t *)&dependency->tag_id;

    hud_text_draw_color_a = color.alpha;
    hud_text_draw_color_r = color.red;
    hud_text_draw_color_g = color.green;
    hud_text_draw_color_b = color.blue;
    hud_text_draw_color_or_flags = color_or_flags;
    hud_text_draw_column = column;
    hud_text_draw_unknown_4730 = unknown_4730;
}

/**
 * Appends `text` to the HUD text-message queue with `start_time` as its arrival time, recognizing a leading
 * "\sNNNN" (a sound-delay count in units of 16ms, replacing the text with a blank string) or "\h" (marks the
 * message as "hold", skipping past the marker) escape. Returns the message's duration in milliseconds (or 0 if
 * `text` is empty).
 * blam-cc: EAX -> text, EBX -> start_time, stack -> tag
 *
 * @address 0x4a3d90
 */
int32_t HudTextQueue::message_queue_add(uint16_t *text, int32_t start_time, int32_t tag)
{
    hud_text_message *message;
    uint16_t *body;
    int32_t index;

    if (wcslen((const wchar_t *)text) == 0) {
        return 0;
    }

    index = growable_array_add_element(&hud_text_message_queue);
    message = (hud_text_message *)hud_text_message_queue.data + index;
    message->start_time = start_time;
    message->unknown_04 = tag;
    message->hold = 0;

    body = text;
    if (text[0] == '\\') {
        body = text + 1;
        if (text[1] == 's') {
            int32_t delay = _wtol((const wchar_t *)(text + 2));

            message->text = empty_string;
            message->end_time = delay * 0x10 + start_time;
            return delay * 0x10;
        }
        if (text[1] == 'h') {
            body = text + 2;
            message->hold = 1;
        }
    }
    message->text = body;
    message->end_time = start_time + 0x10;
    return 0x10;
}

/**
 * Initializes the empty HUD text-message queue and stamps the current time (in milliseconds) as its base for
 * later message expiry calculations.
 *
 * @address 0x4a3ce0
 */
uint32_t HudTextQueue::message_queue_init(void)
{
    large_integer counter;

    hud_text_message_queue.element_size = 0x14;
    hud_text_message_queue.count = 0;
    hud_text_message_queue.data = (void *)0;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    hud_text_message_time_base = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    return 1;
}

}
