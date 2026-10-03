/**
 * @file src/networking/net2_ticker.cpp
 * Join-game ticker text buffer.
 */
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>
#include <stdint.h>
#include "halo/networking/net2_ticker.hpp"
#include "halo/memory/api.hpp"
#include "halo/cache/api.hpp"

extern "C" {
extern heap * widget_memory_pool;
extern void * hud_text_draw_font_tag_id;
extern uint16_t hud_text_draw_color_or_flags;
extern uint16_t hud_text_draw_column;
extern float hud_text_draw_color_a;
extern float hud_text_draw_color_r;
extern float hud_text_draw_color_g;
extern float hud_text_draw_color_b;
extern int32_t hud_text_draw_unknown_4730;
extern int32_t text_measure_string_fit_width(int32_t *max_width_inout);
void ticker_text_buffer_advance(uint8_t *widget, ticker_text_buffer *self);
void ticker_text_buffer_append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self);
void ticker_text_buffer_reset(ticker_text_buffer *self);
}


namespace halo::networking {

void TickerTextBuffer::advance(uint8_t *widget, ticker_text_buffer *self)
{
    uint8_t *row_object = *(uint8_t **)(widget + 0x34);
    uint32_t *text_row = *(uint32_t **)(row_object + 0x2c);
    uint8_t *font_record;
    int32_t max_width[5];
    int32_t fit_count;
    wchar_t *display_text;

    *(int16_t *)(row_object + 0x40) = (int16_t)self->start_column;

    font_record = *(uint8_t **)(halo::cache::globals().tag_instances + (*text_row & 0xffff) * 0x20 + 0x14);
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

    fit_count = text_measure_string_fit_width(max_width);
    if (fit_count == 0) {
        if (self->scroll_cursor == 0) {

            display_text = (wchar_t *)halo::memory::heap_reallocate((void *)(uintptr_t)text_row[0xf],
                (uint32_t)((uint16_t)((int16_t)(self->length + 1)) & 0x7fff) << 1,
                widget_memory_pool);
            text_row[0xf] = (uint32_t)(uintptr_t)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text, self->length);
                display_text[self->length] = 0;
            }
            goto wrap_cursor;
        } else {
            int32_t tail_length = self->length - self->scroll_cursor;
            int32_t wrap_length;

            max_width[0] = text_measure_string_fit_width(max_width);
            display_text = (wchar_t *)halo::memory::heap_reallocate((void *)(uintptr_t)text_row[0xf],
                (uint32_t)(((tail_length + max_width[0]) * 2 + 2) & 0xffff), widget_memory_pool);
            text_row[0xf] = (uint32_t)(uintptr_t)display_text;
            if (display_text != 0) {
                wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, tail_length);
                wrap_length = max_width[0];
                wcsncpy(display_text + tail_length, (const wchar_t *)self->text, wrap_length);
                fit_count = tail_length + wrap_length;
                display_text[fit_count] = 0;
                self->scroll_delay_ms = 100 +
                    (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
                self->scroll_cursor = self->scroll_cursor + 1;
                goto wrap_cursor;
            }
        }
    } else {
        display_text = (wchar_t *)halo::memory::heap_reallocate((void *)(uintptr_t)text_row[0xf],
            (uint32_t)((fit_count * 2 + 2) & 0xffff), widget_memory_pool);
        text_row[0xf] = (uint32_t)(uintptr_t)display_text;
        if (display_text != 0) {
            wcsncpy(display_text, (const wchar_t *)self->text + self->scroll_cursor, fit_count);
            display_text[fit_count] = 0;
        }
    }

    self->scroll_delay_ms = 100 +
        (((*(uint16_t *)(self->text + self->scroll_cursor) & 0xff00) != 0) ? 0x52 : 0);
    self->scroll_cursor = self->scroll_cursor + 1;

wrap_cursor:
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
            widget_memory_pool);
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
                    (uint16_t)((int16_t)self->capacity) << 1, widget_memory_pool);
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

        halo::memory::heap_unlink_block(block, widget_memory_pool);
        widget_memory_pool->bytes_allocated -= (int32_t)size;
        widget_memory_pool->allocation_count -= 1;
    }
    self->text = 0;
    self->capacity = 0;
    self->length = 0;
    self->scroll_cursor = 0;
    self->scroll_delay_ms = 100;
}

}  // namespace halo::networking

extern "C" {
void ticker_text_buffer_advance(uint8_t *widget, ticker_text_buffer *self)
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
