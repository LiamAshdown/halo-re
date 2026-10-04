/**
 * @file src/networking/net2_ticker.cpp
 * Join-game ticker text buffer.
 */
#include "tags.h"
#include "halo/core/datum.hpp"
#include "halo/text/api.hpp"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#include <stdint.h>
#include "halo/networking/net2_ticker.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"
#include "halo/text/text.hpp"
#include "halo/networking/api.hpp"
#include "halo/interface/api.hpp"
#include "halo/core/link.hpp"
#include "halo/interface/vars.hpp"
#include "halo/networking/vars.hpp"

static auto &hud_text_draw_font_tag_id = halo::link::ref<void *>(halo::ui::vars().hud_text_draw_font_tag_id);
static auto &hud_text_draw_color_or_flags = halo::link::ref<uint16_t>(halo::ui::vars().hud_text_draw_color_or_flags);
static auto &hud_text_draw_column = halo::link::ref<uint16_t>(halo::networking::vars().hud_text_draw_column);
static auto &hud_text_draw_color_a = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_a);
static auto &hud_text_draw_color_r = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_r);
static auto &hud_text_draw_color_g = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_g);
static auto &hud_text_draw_color_b = halo::link::ref<float>(halo::ui::vars().hud_text_draw_color_b);
static auto &hud_text_draw_unknown_4730 = halo::link::ref<int32_t>(halo::ui::vars().hud_text_draw_unknown_4730);


namespace halo::networking {

void TickerTextBuffer::advance(network_ui_widget *widget, ticker_text_buffer *self)
{
    network_ui_widget *row_object = widget->first_child;
    network_ui_widget *text_row = row_object->next_sibling;
    uint8_t *font_record;
    int32_t max_width[5];
    int32_t fit_count;
    wchar_t *display_text;

    row_object->value = (int16_t)self->start_column;

    font_record = (uint8_t *)halo::cache::globals().tag_instances[*reinterpret_cast<uint32_t *>(text_row) & halo::k_datum_slot_mask].data;
    hud_text_draw_font_tag_id = *(void **)(font_record + 0x108);
    max_width[0] = (int32_t)*(int16_t *)(font_record + 0x2a) - (int32_t)*(int16_t *)(font_record + 0x26);
    max_width[1] = 0;
    hud_text_draw_color_a = 0.0f;
    max_width[2] = 0;
    max_width[3] = 0;
    max_width[4] = 0;
    hud_text_draw_color_r = 0.0f;
    hud_text_draw_color_g = 0.0f;
    hud_text_draw_color_b = 0.0f;
    hud_text_draw_color_or_flags = 0xffff;
    hud_text_draw_column = 0;
    hud_text_draw_unknown_4730 = 0;

    bool advance = true;

    fit_count = halo::text::text_context::measure_string_fit_width(self->text + self->scroll_cursor, max_width);
    if (fit_count == 0) {
        if (self->scroll_cursor == 0) {

            display_text = (wchar_t *)halo::memory::heap_reallocate(text_row->label_text,
                (uint32_t)((uint16_t)((int16_t)(self->length + 1)) & 0x7fff) << 1,
                halo::interface::globals().widget_memory_pool);
            text_row->label_text = (uint16_t *)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text, self->length);
                display_text[self->length] = 0;
            }
            advance = false;
        } else {
            int32_t tail_length = self->length - self->scroll_cursor;
            int32_t wrap_length;

            max_width[0] = halo::text::text_context::measure_string_fit_width(self->text, max_width);
            display_text = (wchar_t *)halo::memory::heap_reallocate(text_row->label_text,
                (uint32_t)(((tail_length + max_width[0]) * 2 + 2) & halo::k_datum_slot_mask), halo::interface::globals().widget_memory_pool);
            text_row->label_text = (uint16_t *)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, tail_length);
                wrap_length = max_width[0];
                wcsncpy(display_text + tail_length, (const wchar_t *)self->text, wrap_length);
                fit_count = tail_length + wrap_length;
                display_text[fit_count] = 0;
                self->scroll_delay_ms = 100 +
                    (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
                self->scroll_cursor = self->scroll_cursor + 1;
                advance = false;
            }
        }
    } else {
        display_text = (wchar_t *)halo::memory::heap_reallocate(text_row->label_text,
            (uint32_t)((fit_count * 2 + 2) & 0xffff), halo::interface::globals().widget_memory_pool);
        text_row->label_text = (uint16_t *)display_text;
        if (display_text != 0) {
            wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, fit_count);
            display_text[fit_count] = 0;
        }
    }

    if (advance) {
        self->scroll_delay_ms = 100 +
            (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
        self->scroll_cursor = self->scroll_cursor + 1;
    }

    if (self->length <= self->scroll_cursor) {
        self->scroll_cursor = 0;
    }
}

void TickerTextBuffer::append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self)
{
    if (text == 0) {
        if (self->capacity == 0) {
            self->capacity = 0x20;
        }
        self->text = (uint16_t *)halo::memory::heap_reallocate(0, (uint16_t)((int16_t)self->capacity) << 1,
            halo::interface::globals().widget_memory_pool);
        self->length = 0;
        self->start_column = reset_column;
    } else {
        int32_t text_length = (int32_t)wcslen(text);

        if (self->capacity <= self->length + 1 + text_length) {
            do {
                if (self->capacity == 0) {
                    self->capacity = 0x20;
                } else {
                    self->capacity = self->capacity * 2;
                }
                self->text = (uint16_t *)halo::memory::heap_reallocate(self->text,
                    (uint16_t)((int16_t)self->capacity) << 1, halo::interface::globals().widget_memory_pool);
            } while (self->capacity <= self->length + 1 + text_length);
        }
        if (self->text != 0) {
            wcscpy((wchar_t *)self->text + self->length, (const wchar_t *)text);
            self->length = self->length + text_length;
        }
    }
    self->text[self->length] = 0;
    self->scroll_cursor = 0;
}

void TickerTextBuffer::reset(ticker_text_buffer *self)
{
    if (self->text != 0) {
        heap_block *block = (heap_block *)((uint8_t *)self->text - 0x10);
        uint32_t size = block->size & k_heap_block_size_mask;

        halo::memory::heap_unlink_block(block, halo::interface::globals().widget_memory_pool);
        halo::interface::globals().widget_memory_pool->bytes_allocated -= (int32_t)size;
        halo::interface::globals().widget_memory_pool->allocation_count -= 1;
    }
    self->text = 0;
    self->capacity = 0;
    self->length = 0;
    self->scroll_cursor = 0;
    self->scroll_delay_ms = 100;
}

}  // namespace halo::networking

namespace halo::networking {
void ticker_text_buffer_advance(network_ui_widget *widget, ticker_text_buffer *self)
{
    halo::networking::TickerTextBuffer::advance(widget, self);
}

void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self)
{
    halo::networking::TickerTextBuffer::append(text, reset_column, self);
}

void ticker_text_buffer_reset(ticker_text_buffer *self)
{
    halo::networking::TickerTextBuffer::reset(self);
}

}
