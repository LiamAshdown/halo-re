#include "halo/memory/memory.hpp"

#include "tags.h"
#include "crt.h"

/**
 * Reads a single bit from a bounds-checked bit stream into *out_bit and advances the stream's one-bit
 * cursor. Returns 1 on success, 0 if the stream has no room left.
 *
 * @address 0x4cfb80
 */
uint32_t bit_stream::read_bit(uint8_t *out_bit)
{
    uint32_t pos;
    uint32_t result;
    uint8_t bit_index;
    uint32_t new_pos;

    pos = (uint32_t)this->bit_cursor + (uint32_t)this->byte_cursor * 8;
    result = 0;
    if (this->first_bit <= pos && pos <= this->last_bit) {
        bit_index = (uint8_t)this->bit_cursor;
        *out_bit = (uint8_t)((this->data[this->byte_cursor] & (1 << (bit_index & 0x1f))) >>
                              (bit_index & 0x1f));
        new_pos = (uint32_t)this->bit_cursor + 1 + (uint32_t)this->byte_cursor * 8;
        if ((this->first_bit <= new_pos && new_pos <= this->last_bit) ||
            new_pos == this->last_bit + 1) {
            this->bit_cursor = new_pos & 7;
            this->byte_cursor = new_pos >> 3;
        }
        result = 1;
    }
    return result;
}

/**
 * Reads the low bit_count bits (LSB first) out of the stream into *out_value, merging with whatever
 * was already in *out_value above bit position bit_count. Always returns bit_count; the stream-bounds
 * failure path leaves *out_value untouched and returns 0.
 *
 * @address 0x4cfbf0
 */
uint32_t bit_stream::read_bits(uint32_t bit_count, uint32_t *out_value)
{
    int32_t initial_bit_cursor;
    int32_t initial_byte_cursor;
    uint32_t initial_pos;
    uint32_t last_pos;
    uint32_t first_run;
    uint32_t accumulator;
    uint32_t remaining;
    uint32_t pos;
    uint32_t chunk;
    uint8_t *byte_cursor_ptr;
    uint32_t low_mask;

    initial_bit_cursor = this->bit_cursor;
    initial_byte_cursor = this->byte_cursor;
    initial_pos = (uint32_t)initial_bit_cursor + (uint32_t)initial_byte_cursor * 8;
    last_pos = (bit_count - 1) + initial_pos;
    if (last_pos < this->first_bit || this->last_bit < last_pos) {
        return 0;
    }

    accumulator = 0;
    first_run = bit_count;
    if (8U - (uint32_t)initial_bit_cursor <= bit_count) {
        first_run = 8U - (uint32_t)initial_bit_cursor;
    }
    if (first_run == 8) {
        first_run = 0;
        remaining = bit_count;
    } else {
        pos = first_run + initial_pos;
        if ((this->first_bit <= pos && pos <= this->last_bit) || pos == this->last_bit + 1) {
            this->bit_cursor = pos & 7;
            this->byte_cursor = pos >> 3;
        }
        remaining = bit_count - first_run;
    }

    accumulator = 0;
    if (remaining != 0) {
        uint8_t *data = this->data;
        uint8_t *dst = (uint8_t *)&accumulator;
        chunk = remaining;
        do {
            uint32_t take = chunk;
            if (7 < chunk) {
                take = 8;
            }
            *dst = data[this->byte_cursor] & bit_mask_keep[take];
            pos = take + (uint32_t)this->byte_cursor * 8 + (uint32_t)this->bit_cursor;
            dst = dst + 1;
            if ((this->first_bit <= pos && pos <= this->last_bit) || pos == this->last_bit + 1) {
                this->bit_cursor = pos & 7;
                this->byte_cursor = pos >> 3;
            }
            chunk = chunk - take;
        } while (chunk != 0);
    }

    low_mask = (bit_count < 0x20) ? (uint32_t)(-1 << (bit_count & 0x1f)) : 0;
    if (first_run == 0) {
        *out_value = (*out_value & low_mask) | accumulator;
        return bit_count;
    }
    byte_cursor_ptr = this->data + initial_byte_cursor;
    *out_value = ((uint32_t)(*byte_cursor_ptr >> (initial_bit_cursor & 0x1f)) &
                  bit_mask_keep[first_run]) |
                 (*out_value & low_mask) | (accumulator << (first_run & 0x1f));
    return bit_count;
}

/**
 * Reads total_bit_count bits into the uint32_t array at buffer, 32 bits per element, through
 * read_bits. Returns the number of bits read, which is less than total_bit_count when a chunk came up
 * short.
 *
 * @address 0x4cf950
 */
int32_t bit_stream::read_bits_chunked(int32_t total_bit_count, uint32_t *buffer)
{
    int32_t remaining;
    int32_t total_read;
    uint32_t chunk_read;

    remaining = total_bit_count;
    total_read = 0;
    for (;;) {
        if (remaining < 1) {
            return total_read;
        }
        if (remaining < 0x20) {
            chunk_read = this->read_bits((uint32_t)remaining, buffer);
            if (chunk_read != (uint32_t)remaining) {
                return total_read;
            }
        } else {
            chunk_read = this->read_bits(0x20, buffer);
            if (chunk_read != 0x20) {
                return total_read;
            }
            buffer = buffer + 1;
            chunk_read = 0x20;
        }
        remaining = remaining - (int32_t)chunk_read;
        total_read = total_read + (int32_t)chunk_read;
    }
}

/**
 * Writes a single bit into the bounds-checked bit stream and advances the one-bit cursor. Only the low
 * byte of the result is meaningful: nonzero on success.
 *
 * @address 0x4cf9a0
 */
uint8_t bit_stream::write_bit(int32_t bit_value)
{
    int32_t byte_cursor;
    uint32_t pos;
    uint32_t result;
    uint8_t bit_index;
    uint8_t *byte_ptr;
    uint32_t new_pos;

    byte_cursor = this->byte_cursor;
    pos = (uint32_t)this->bit_cursor + (uint32_t)byte_cursor * 8;
    result = 0;

    if (this->first_bit <= pos && pos <= this->last_bit) {
        bit_index = (uint8_t)this->bit_cursor;
        byte_ptr = this->data + byte_cursor;
        if (bit_value == 1) {
            *byte_ptr = *byte_ptr | (uint8_t)(1 << (bit_index & 0x1f));
        } else {
            *byte_ptr = *byte_ptr & (uint8_t)~(1 << (bit_index & 0x1f));
        }
        new_pos = (uint32_t)this->bit_cursor + 1 + (uint32_t)this->byte_cursor * 8;
        if ((this->first_bit <= new_pos && new_pos <= this->last_bit) ||
            new_pos == this->last_bit + 1) {
            this->bit_cursor = new_pos & 7;
            this->byte_cursor = new_pos >> 3;
        }
        result = 1;
    }
    return result;
}

/**
 * Writes the low bit_count bits of value, least significant bit first, at the cursor, merging into
 * partially filled bytes with the bit mask tables. Returns 1 and advances the cursor when the run fits
 * the bounds, otherwise leaves the stream untouched and returns 0.
 *
 * @address 0x4cfa20
 */
uint8_t bit_stream::write_bits(uint32_t bit_count, uint32_t value)
{
    int32_t initial_bit_cursor;
    uint32_t last_pos;
    uint32_t result;
    uint32_t written;
    uint8_t merge_mask;
    uint8_t *byte_ptr;
    uint8_t low_byte;
    uint32_t chunk;
    uint32_t pos;

    initial_bit_cursor = this->bit_cursor;
    last_pos = (uint32_t)initial_bit_cursor + (uint32_t)this->byte_cursor * 8 - 1 + bit_count;
    result = 0;
    if (this->first_bit <= last_pos && last_pos <= this->last_bit) {
        written = 0;
        if (initial_bit_cursor != 0) {

            uint32_t room_in_byte = 8 - (uint32_t)initial_bit_cursor;
            if (bit_count < room_in_byte) {
                merge_mask = bit_mask_keep[bit_count];
                written = bit_count;
            } else {
                merge_mask = bit_mask_keep[8 - initial_bit_cursor];
                written = room_in_byte;
            }
            byte_ptr = this->data + this->byte_cursor;
            low_byte = (uint8_t)value;
            value = value >> (written & 0x1f);
            *byte_ptr = (uint8_t)(~(merge_mask << (initial_bit_cursor & 0x1f)) & *byte_ptr) |
                        (uint8_t)((low_byte & merge_mask) << (initial_bit_cursor & 0x1f));
            pos = written + (uint32_t)this->byte_cursor * 8 + (uint32_t)this->bit_cursor;
            if ((this->first_bit <= pos && pos <= this->last_bit) ||
                pos == this->last_bit + 1) {
                this->bit_cursor = pos & 7;
                this->byte_cursor = pos >> 3;
            }
        }
        while (written < bit_count) {
            chunk = bit_count - written;
            if (chunk < 8) {
                byte_ptr = this->data + this->byte_cursor;
                *byte_ptr = (bit_mask_clear[chunk] & *byte_ptr) |
                            (bit_mask_keep[chunk] & (uint8_t)value);
                written = written + chunk;
                value = value >> (chunk & 0x1f);
                pos = chunk + (uint32_t)this->byte_cursor * 8 + (uint32_t)this->bit_cursor;
                if ((this->first_bit <= pos && pos <= this->last_bit) ||
                    pos == this->last_bit + 1) {
                    this->bit_cursor = pos & 7;
                    this->byte_cursor = pos >> 3;
                }
            } else {
                this->data[this->byte_cursor] = (uint8_t)value;
                pos = (uint32_t)this->bit_cursor + 8 + (uint32_t)this->byte_cursor * 8;
                value = value >> 8;
                written = written + 8;
                if ((this->first_bit <= pos && pos <= this->last_bit) ||
                    pos == this->last_bit + 1) {
                    this->bit_cursor = pos & 7;
                    this->byte_cursor = pos >> 3;
                }
            }
        }
        result = 1;
    }
    return result;
}

/**
 * Writes total_bit_count bits taken from the array values, 32 bits per element, through write_bits.
 * Returns the number of bits written, which is less than total_bit_count when the stream ran out of
 * room.
 *
 * @address 0x4cf8f0
 */
int32_t bit_stream::write_bits_chunked(const uint32_t *values, int32_t total_bit_count)
{
    int32_t remaining = total_bit_count;

    if (0 < total_bit_count) {
        while (0x1f < remaining) {
            if (this->write_bits(0x20, *values) == 0) {
                return total_bit_count - remaining;
            }
            remaining = remaining - 0x20;
            values = values + 1;
            if (remaining < 1) {
                return total_bit_count - remaining;
            }
        }
        if (this->write_bits((uint32_t)remaining, *values) != 0) {
            remaining = 0;
        }
    }
    return total_bit_count - remaining;
}

/**
 * Reads a big-endian 32 bit value and advances the cursor. Sets the overflow flag and returns 0 when
 * the stream is exhausted or already overflowed.
 *
 * @address 0x4d0850
 */
uint32_t byte_stream::read_long()
{
    uint32_t *value_ptr;

    if (this->size < this->cursor + 4 || this->overflow != 0) {
        this->overflow = 1;
    } else {
        value_ptr = (uint32_t *)(this->data + this->cursor);
        halo::memory::byte_swap_array(-4, value_ptr, 1);
        this->cursor = this->cursor + 4;
        if (value_ptr != 0) {
            return *value_ptr;
        }
    }
    return 0;
}

/**
 * Reads a big-endian value stored in the narrowest of 1, 2 or 4 bytes that can hold maximum, the
 * inverse of write_ranged_integer. Sets the overflow flag and returns 0 when there is no room.
 *
 * @address 0x4d08a0
 */
uint32_t byte_stream::read_ranged_integer(int32_t maximum)
{
    int32_t next_cursor;
    uint8_t *byte_ptr;
    uint16_t *word_ptr;

    if (maximum < 0x100) {
        next_cursor = this->cursor + 1;
        if (this->size < next_cursor || this->overflow != 0) {
            this->overflow = 1;
        } else {
            byte_ptr = this->data + this->cursor;
            this->cursor = next_cursor;
            if (byte_ptr != 0) {
                return (uint32_t)*byte_ptr;
            }
        }
        return 0;
    }
    if (0xffff < maximum) {
        return this->read_long();
    }
    if (this->cursor + 2 <= this->size && this->overflow == 0) {
        word_ptr = (uint16_t *)(this->data + this->cursor);
        *word_ptr = (uint16_t)(((*word_ptr & 0xff) << 8) | ((*word_ptr >> 8) & 0xff));
        this->cursor = this->cursor + 2;
        return (uint32_t)(int16_t)*word_ptr;
    }
    this->overflow = 1;
    return 0;
}

/**
 * Reads a NUL-terminated string and advances the cursor past the terminator. Returns the address of
 * the string inside the stream buffer, or null with the overflow flag set when no terminator is found.
 *
 * @address 0x4d0930
 */
char *byte_stream::read_string()
{
    int32_t start;
    int32_t offset;
    int16_t length;

    start = this->cursor;
    length = 0;
    if (start < this->size) {
        offset = 0;
        do {
            if (this->data[offset + start] == 0) {
                this->cursor = length + 1 + start;
                return (char *)(this->data + start);
            }
            length = length + 1;
            offset = (int32_t)length;
        } while (start + offset < this->size);
    }
    this->overflow = 1;
    return 0;
}

/**
 * Writes value big-endian in the narrowest of 1, 2 or 4 bytes that can hold maximum, matching the
 * encoding struct_definition::encode uses for counts. Sets the overflow flag and returns false when
 * there is no room.
 *
 * @address 0x4d0700
 */
uint32_t byte_stream::write_ranged_integer(int32_t maximum, uint32_t value)
{
    uint8_t *dst;
    uint16_t native16;

    if (maximum < 0x100) {
        if (this->cursor + 1 <= this->size && this->overflow == 0) {
            *(this->data + this->cursor) = (uint8_t)value;
            this->cursor = this->cursor + 1;
            return this->overflow == 0;
        }
    } else if (maximum < 0x10000) {
        if (this->cursor + 2 <= this->size && this->overflow == 0) {
            dst = this->data + this->cursor;
            native16 = (uint16_t)value;
            *(uint16_t *)dst = native16;
            *(uint16_t *)dst = (uint16_t)(((value & 0xff) << 8) | ((value >> 8) & 0xff));
            this->cursor = this->cursor + 2;
            return this->overflow == 0;
        }
    } else if (this->cursor + 4 <= this->size && this->overflow == 0) {
        *(uint32_t *)(this->data + this->cursor) = value;
        halo::memory::byte_swap_array(-4, (uint32_t *)(this->data + this->cursor), 1);
        this->cursor = this->cursor + 4;
        return this->overflow == 0;
    }
    this->overflow = 1;
    return this->overflow == 0;
}

/**
 * Writes a NUL-terminated copy of string, capped to max_length characters. Sets the overflow flag and
 * returns false when the string and its terminator do not fit.
 *
 * @address 0x4d07e0
 */
uint32_t byte_stream::write_string(char *string, int16_t max_length)
{
    int32_t length;
    char *scan;
    char ch;
    int32_t count;
    char *dst;

    length = 0;
    scan = string;
    if (0 < max_length) {
        do {
            ch = *scan;
            scan = scan + 1;
            if (ch == 0) {
                break;
            }
            length = length + 1;
        } while (length < max_length);
    }
    count = (int16_t)length;
    dst = (char *)(this->data + this->cursor);
    if (count + 1 + this->cursor <= this->size && this->overflow == 0) {
        strncpy(dst, string, (uint32_t)count);
        dst[count] = 0;
        this->cursor = this->cursor + count + 1;
        return this->overflow == 0;
    }
    this->overflow = 1;
    return this->overflow == 0;
}
