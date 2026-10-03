#include "halo/interface/ifr1_hud_text_queue.hpp"

/**
 * C ABI entry point; forwards to halo::interface::HudTextQueue::draw_configure.
 * blam-cc: stack -> (font_table_index, color_or_flags, column, unknown_4730, color_table_index,
 *
 * @address 0x4944c0
 */
extern "C" void hud_text_draw_configure(int16_t font_table_index, uint16_t color_or_flags, int16_t column, uint32_t unknown_4730, int16_t color_table_index, int16_t color_index)
{
    halo::interface::HudTextQueue::draw_configure(font_table_index, color_or_flags, column, unknown_4730, color_table_index, color_index);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTextQueue::message_queue_add.
 * blam-cc: EAX -> text, EBX -> start_time, stack -> tag
 *
 * @address 0x4a3d90
 */
extern "C" int32_t hud_text_message_queue_add(uint16_t *text, int32_t start_time, int32_t tag)
{
    return halo::interface::HudTextQueue::message_queue_add(text, start_time, tag);
}

/**
 * C ABI entry point; forwards to halo::interface::HudTextQueue::message_queue_init.
 *
 * @address 0x4a3ce0
 */
extern "C" uint32_t hud_text_message_queue_init(void)
{
    return halo::interface::HudTextQueue::message_queue_init();
}

/**
 * C ABI entry point; forwards to halo::interface::HudTextQueue::message_queue_update_and_draw.
 *
 * @address 0x4a3e30
 */
extern "C" uint32_t hud_text_message_queue_update_and_draw(widget_instance *widget)
{
    return halo::interface::HudTextQueue::message_queue_update_and_draw(widget);
}
