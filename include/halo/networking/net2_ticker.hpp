/**
 * @file include/halo/networking/net2_ticker.hpp
 * Join-game ticker text buffer.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Join-game ticker text buffer.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class TickerTextBuffer {
public:
    /**
     * 0x0087bc14
     *
     * @address 0x4b8b40
     */
    static void advance(uint8_t *widget, ticker_text_buffer *self);

    /**
     * 0x4d1f80 (only used on the NULL path), EDI -> self With text == NULL, (re)allocates the buffer to hold at least 0x20 wide characters, empties it and stores reset_column into start_column. Otherwise grows the buffer (doubling capacity, or seeding it at 0x20 characters) until it can hold the existing text plus the new text plus a NUL, then appends the new text. Either way the result is always left NUL-terminated and scroll_cursor is reset to the beginning.
     *
     * @address 0x4b8a60
     */
    static void append(wchar_t *text, int32_t reset_column, ticker_text_buffer *self);

    /**
     * 0x4d20a0 Frees the buffer's current heap allocation (if any), updating widget_memory_pool's usage totals, then resets every field to an empty buffer with the default 100 ms scroll delay.
     *
     * @address 0x4b8a00
     */
    static void reset(ticker_text_buffer *self);

};

}  // namespace halo::networking
