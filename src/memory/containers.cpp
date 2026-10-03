#include "halo/memory/memory.hpp"

#include "win32.h"
#include "tags.h"
#include "crt.h"

#define GMEM_MOVEABLE 0x0002

/**
 * Appends one zero-initialized element, growing the GlobalAlloc-backed storage as needed. Returns the
 * new element's index, or -1 (k_datum_index_none) on allocation failure.
 *
 * @address 0x4cf810
 */
uint32_t growable_array::add_element()
{
    uint32_t new_count;
    uint32_t bytes;
    void *new_data;
    uint32_t old_count;
    uint32_t element_size;
    uint8_t *dst;
    uint32_t i;

    if (this->count < 0x7fffffff) {
        new_count = this->count + 1;
        bytes = (uint32_t)this->element_size * new_count;
        new_data = this->data;
        if (new_data == 0) {
            new_data = GlobalAlloc(0, bytes);
        } else {
            if (bytes == 0) {
                GlobalFree(new_data);
                return 0xffffffff;
            }
            new_data = GlobalReAlloc(new_data, bytes, GMEM_MOVEABLE);
        }
        if (new_data != 0) {
            element_size = (uint32_t)this->element_size;
            old_count = (uint32_t)this->count;
            dst = (uint8_t *)new_data + element_size * old_count;

            for (i = 0; i < element_size; i = i + 1) {
                dst[i] = 0;
            }
            this->count = (int32_t)new_count;
            this->data = new_data;
            return old_count;
        }
    }
    return 0xffffffff;
}

/**
 * Removes the element at `index`, compacting the elements above it down by one slot and shrinking (or
 * freeing) the GlobalAlloc-backed storage to match the new count. No return value.
 *
 * @address 0x4cf890
 */
void growable_array::remove_element(uint32_t index)
{
    uint32_t new_count;
    uint32_t element_size;
    uint8_t *dst;
    uint32_t bytes;
    void *data;

    new_count = (uint32_t)this->count - 1;
    this->count = (int32_t)new_count;
    if (index < new_count) {
        element_size = (uint32_t)this->element_size;
        dst = (uint8_t *)this->data + element_size * index;
        memmove(dst, dst + element_size, (new_count - index) * element_size);
    }
    data = this->data;
    bytes = (uint32_t)this->element_size * (uint32_t)this->count;
    if (data != 0) {
        if (bytes != 0) {
            this->data = GlobalReAlloc(data, bytes, GMEM_MOVEABLE);
            return;
        }
        GlobalFree(data);
        this->data = 0;
        return;
    }
    this->data = GlobalAlloc(0, bytes);
}

/**
 * Allocates a circular_buffer with room for requested_size bytes (one slot is always kept empty) and
 * initialises it. name is stored as given, not copied. The allocation result is not returned to the
 * caller.
 *
 * @address 0x4d0170
 */
void circular_buffer::create(char *name, int32_t requested_size)
{
    circular_buffer *buf;

    buf = (circular_buffer *)GlobalAlloc(0, requested_size + 0x19);
    if (buf != 0) {
        buf->name = 0;
        buf->signature = 0;
        buf->read_cursor = 0;
        buf->write_cursor = 0;
        buf->capacity = 0;
        buf->data = 0;
        buf->name = name;
        buf->signature = k_circular_buffer_signature;
        buf->capacity = requested_size + 1;
        buf->data = (uint8_t *)buf + 0x18;
    }
}

/**
 * Reads byte_count bytes into destination with wraparound and advances the read cursor only when
 * consume is nonzero, so a zero consume is a peek. Returns 1, or 0 without reading when fewer than
 * byte_count bytes are buffered.
 *
 * @address 0x4d0240
 */
uint32_t circular_buffer::read(uint8_t *destination, uint32_t byte_count, char consume)
{
    int32_t read_cursor;
    uint32_t result;
    int32_t available;
    uint32_t tail_room;
    uint8_t *src;
    uint8_t *dst;
    uint32_t words;
    uint32_t bytes;

    read_cursor = this->read_cursor;
    result = 0;
    available = this->write_cursor - read_cursor;
    if (available < 0) {
        available = available + this->capacity;
    }
    if ((int32_t)byte_count <= available) {
        tail_room = (uint32_t)this->capacity - (uint32_t)read_cursor;
        if (tail_room <= byte_count) {
            src = this->data + read_cursor;
            dst = destination;
            for (words = tail_room >> 2; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (bytes = tail_room & 3; bytes != 0; bytes = bytes - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
            destination = destination + tail_room;
            read_cursor = 0;
            byte_count = byte_count - tail_room;
        }
        if (0 < (int32_t)byte_count) {
            src = this->data + read_cursor;
            for (words = byte_count >> 2; words != 0; words = words - 1) {
                *(uint32_t *)destination = *(uint32_t *)src;
                src = src + 4;
                destination = destination + 4;
            }
            for (bytes = byte_count & 3; bytes != 0; bytes = bytes - 1) {
                *destination = *src;
                src = src + 1;
                destination = destination + 1;
            }
            read_cursor = read_cursor + (int32_t)byte_count;
        }
        if (consume != 0) {
            this->read_cursor = read_cursor;
        }
        result = 1;
    }
    return result;
}

/**
 * Writes byte_count bytes from source with wraparound and advances the write cursor. Returns 1, or 0
 * without writing when the free space is too small.
 *
 * @address 0x4d01c0
 */
uint32_t circular_buffer::write(uint32_t byte_count, uint8_t *source)
{
    int32_t write_cursor;
    uint32_t result;
    int32_t used;
    uint32_t tail_room;
    uint8_t *src;
    uint8_t *dst;
    uint32_t words;
    uint32_t bytes;

    write_cursor = this->write_cursor;
    result = 0;
    used = write_cursor - this->read_cursor;
    if (used < 0) {
        used = used + this->capacity;
    }
    if ((int32_t)((uint32_t)used + byte_count) < this->capacity) {
        tail_room = (uint32_t)this->capacity - (uint32_t)write_cursor;
        if (tail_room <= byte_count) {
            src = source;
            dst = this->data + write_cursor;
            for (words = tail_room >> 2; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (bytes = tail_room & 3; bytes != 0; bytes = bytes - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
            source = source + tail_room;
            this->write_cursor = 0;
            byte_count = byte_count - tail_room;
        }
        if (0 < (int32_t)byte_count) {
            dst = this->data + this->write_cursor;
            src = source;
            for (words = byte_count >> 2; words != 0; words = words - 1) {
                *(uint32_t *)dst = *(uint32_t *)src;
                src = src + 4;
                dst = dst + 4;
            }
            for (bytes = byte_count & 3; bytes != 0; bytes = bytes - 1) {
                *dst = *src;
                src = src + 1;
                dst = dst + 1;
            }
            this->write_cursor = this->write_cursor + (int32_t)byte_count;
        }
        result = 1;
    }
    return result;
}
