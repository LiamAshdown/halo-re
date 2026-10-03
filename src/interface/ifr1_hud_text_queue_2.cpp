#include "halo/interface/ifr1_hud_text_queue.hpp"
#include <wchar.h>
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/text/api.hpp"

extern "C" {
extern int64_t performance_frequency;
extern int32_t hud_text_message_time_base;
extern growable_array hud_text_message_queue;
extern int32_t hud_text_message_cycle_state_00719230;
extern ColorARGB *hud_text_message_hold_color;
extern ColorARGB *hud_text_message_normal_color;
extern int32_t hud_text_draw_font_tag_id;
extern uint32_t hud_text_draw_color_or_flags;
extern int32_t hud_text_draw_unknown_4730;
extern uint16_t missing_string_text[];
extern void widget_instance_close_and_restore_previous(widget_instance *widget);
extern int32_t hud_text_message_queue_add(uint16_t *text, int32_t start_time, int32_t tag);
extern void chimera__draw_16_bit_text(Rectangle2D *clip_rect_override, int32_t *dest_rect_override,
    uint32_t position_or_color1, uint32_t position_or_color2, const int16_t *text);
}

namespace halo::interface {

/**
 * Original engine function hud_text_message_queue_update_and_draw; the author notes are in
 * docs/original/interface/hud_text_message_queue_update_and_draw.txt.
 *
 * @address 0x4a3e30
 */
uint32_t HudTextQueue::message_queue_update_and_draw(widget_instance *widget)
{
    UIWidgetDefinition *tag = (UIWidgetDefinition *)halo::cache::globals().tag_instances[widget->definition & 0xffff].data;
    UnicodeStringList *strings =
        (UnicodeStringList *)halo::cache::globals().tag_instances[tag->text_label_unicode_strings_list.tag_id.index].data;
    int32_t string_count = strings->strings.count;
    int32_t bottom = 0x1ae;
    int32_t message_index = -1;
    large_integer counter;
    int32_t now_ms;
    int32_t elapsed;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    elapsed = (int32_t)(long long)((double)(uint32_t)(now_ms - hud_text_message_time_base) * (double)0.08f);

    if (elapsed != 0) {
        hud_text_message_time_base = now_ms;

        if (hud_text_message_queue.count > 0) {
            int32_t i;

            for (i = 0; i < hud_text_message_queue.count; i++) {
                hud_text_message *entry = &((hud_text_message *)hud_text_message_queue.data)[i];

                message_index = entry->unknown_04;
                entry->start_time = entry->start_time - elapsed;
                entry->end_time = entry->end_time - elapsed;
                bottom = entry->end_time;
                if (bottom < 0x32) {
                    halo::memory::growable_array_remove_element(&hud_text_message_queue, (uint32_t)i);
                    i--;
                }
            }
            if (bottom > 0x1ae) {
                goto draw;
            }
        }

        do {
            uint16_t *text = missing_string_text;

            message_index++;
            if (message_index >= string_count) {
                if (hud_text_message_cycle_state_00719230 != 0) {
                    if (hud_text_message_queue.count > 0) {
                        goto draw;
                    }
                    hud_text_message_cycle_state_00719230 = 2;
                    widget_instance_close_and_restore_previous(widget);
                    return 1;
                }
                message_index = 0;
            }

            if (*(uint32_t *)&tag->text_label_unicode_strings_list.tag_id != 0xffffffff) {
                UnicodeStringList *list =
                    (UnicodeStringList *)halo::cache::globals().tag_instances[tag->text_label_unicode_strings_list.tag_id.index].data;

                if ((int16_t)message_index >= 0 && (int16_t)message_index < (int32_t)list->strings.count) {
                    UnicodeStringListString *string =
                        &((UnicodeStringListString *)list->strings.pointer)[(int16_t)message_index];
                    int32_t size = (int32_t)string->string.size;

                    if (size > 0) {
                        text = (uint16_t *)string->string.pointer;
                        text[((uint32_t)size >> 1) - 1] = 0;
                    }
                }
            }
            bottom += hud_text_message_queue_add(text, bottom, message_index);
        } while (bottom <= 0x1ae);
    }

draw:
    if (hud_text_message_queue.count > 0) {
        Rectangle2D clip;
        Rectangle2D dest;
        int32_t i;

        clip.top = 0x32;
        clip.left = 0;
        clip.bottom = 0x1ae;
        clip.right = 0x280;
        dest.left = 0;
        dest.right = 0x280;
        for (i = 0; i < hud_text_message_queue.count; i++) {
            hud_text_message *entry = &((hud_text_message *)hud_text_message_queue.data)[i];
            ColorARGB *color = (entry->hold == 1) ? hud_text_message_normal_color : hud_text_message_hold_color;

            dest.bottom = (int16_t)entry->end_time;
            dest.top = (int16_t)entry->start_time;
            hud_text_draw_font_tag_id = *(int32_t *)&((struct UIWidgetDefinition *)tag)->text_font.tag_id;
            halo::text::globals().hud_text_draw_color_a = *color;
            hud_text_draw_color_or_flags = 0x0002ffff;
            hud_text_draw_unknown_4730 = 0;
            chimera__draw_16_bit_text(&clip, (int32_t *)&dest, 0, 0, (const int16_t *)entry->text);
        }
    }
    return 1;
}

}
