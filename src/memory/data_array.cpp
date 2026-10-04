#include "halo/memory/memory.hpp"

#include "crt.h"
#include "tags.h"
#include "win32.h"
#include "halo/core/datum.hpp"
#include "halo/platform/memory.hpp"

namespace halo::memory {

/**
 * Empties the whole array: resets the counters, reseeds next_identifier from the array name with the
 * top bit forced so it is never zero, and clears every element's identifier.
 *
 * @address 0x4d0580
 */
void data_array_view::delete_all()
{
    int16_t index;

    this->last_index = 0;
    this->actual_count = 0;
    this->next_index = 0;
    strncpy((char *)&this->next_identifier, this->name, 2);
    this->next_identifier = this->next_identifier | (int16_t)k_datum_identifier_wrap;
    index = 0;
    if (0 < this->maximum_count) {
        do {
            *(int16_t *)((uint8_t *)this->data + (int32_t)this->size * (int32_t)index) = 0;
            index = index + 1;
        } while (index < this->maximum_count);
    }
}

/**
 * Allocates a data_array for maximum_count elements of element_size bytes and initialises its header;
 * the element storage directly follows the header. The name is truncated to 31 characters. Returns
 * null when the allocation fails.
 *
 * @address 0x4d0370
 */
data_array *data_array_view::create(int16_t element_size, const char *name, int16_t maximum_count)
{
    data_array *array;
    uint8_t *zero;
    int32_t i;

    array = (data_array *)halo::platform::heap_allocate(0, (int32_t)maximum_count * (int32_t)element_size + 0x38);
    if (array != 0) {
        zero = (uint8_t *)array;
        for (i = 0xe; i != 0; i = i - 1) {
            zero[0] = 0;
            zero[1] = 0;
            zero[2] = 0;
            zero[3] = 0;
            zero = zero + 4;
        }
        strncpy(array->name, name, 0x1f);
        array->maximum_count = maximum_count;
        array->size = element_size;
        array->signature = k_data_array_signature;
        array->data = (uint8_t *)array + 0x38;
        array->valid = 0;
    }
    return array;
}

/**
 * Frees the datum named by handle. A zero salt in the handle matches any live slot. The slot is
 * cleared, next_index is rewound when needed, and last_index walks back over trailing free slots.
 *
 * A handle that fails validation is not supported: the original code falls through to a null element
 * write, which is kept.
 *
 * @address 0x4d0510
 */
void data_array_view::delete_datum(datum_index handle)
{
    int16_t index;
    int16_t salt;
    int16_t *element;

    index = (int16_t)handle;
    element = 0;
    if (-1 < index && index < this->last_index) {
        element = (int16_t *)((int32_t)this->size * (int32_t)index + (int32_t)this->data);
        if (*element != 0) {
            salt = (int16_t)(handle >> 0x10);
            if (salt == 0 || salt == *element) {
                goto do_delete;
            }
        }
        element = 0;
    }

do_delete:
    *element = 0;
    if (index < this->next_index) {
        this->next_index = index;
    }
    if (index + 1 == (int32_t)this->last_index) {
        do {
            element = (int16_t *)((uint8_t *)element - this->size);
            this->last_index = this->last_index - 1;
            if (this->last_index < 1) {
                break;
            }
        } while (*element == 0);
    }
    this->actual_count = this->actual_count - 1;
}

/**
 * Zero-fills one element's worth of bytes at `element`, then stamps its datum_header::identifier with
 * array->next_identifier and advances/reseeds that salt counter (wrapping to k_datum_identifier_wrap
 * when it would otherwise become k_datum_identifier_none).
 *
 * @address 0x4d06c0
 */
void data_array_view::initialize_element(void *element)
{
    int16_t element_size;
    uint32_t words;
    uint32_t bytes;
    uint8_t *dst;

    element_size = this->size;
    dst = (uint8_t *)element;
    for (words = (uint32_t)(int32_t)element_size >> 2; words != 0; words = words - 1) {
        *(uint32_t *)dst = 0;
        dst = dst + 4;
    }
    for (bytes = (uint32_t)(int32_t)element_size & 3; bytes != 0; bytes = bytes - 1) {
        *dst = 0;
        dst = dst + 1;
    }
    *(int16_t *)element = this->next_identifier;
    this->next_identifier = this->next_identifier + 1;
    if (this->next_identifier == 0) {
        this->next_identifier = (int16_t)k_datum_identifier_wrap;
    }
}

/**
 * Resolves `handle` (index in the low 16 bits, salt in the high 16, zero salt acting as a wildcard) to
 * its element pointer if the index and salt are valid and the slot is in use, otherwise returns NULL.
 *
 * @address 0x4d0680
 */
void *data_array_view::get(datum_index handle)
{
    void *result;
    int16_t index;
    int16_t *element;
    int16_t identifier;
    int16_t salt;

    result = 0;
    if (handle != halo::k_dword_none) {
        index = (int16_t)handle;
        if (-1 < index && index < this->maximum_count) {
            element = (int16_t *)((int32_t)this->size * (int32_t)index + (int32_t)this->data);
            identifier = *element;
            if (identifier != 0) {
                salt = (int16_t)(handle >> 0x10);
                if (salt == 0 || identifier == salt) {
                    result = element;
                }
            }
        }
    }
    return result;
}

/**
 * Allocates the first free slot at or after next_index, zero fills it and stamps a fresh salt. Returns
 * the new handle (salt in the high half, index in the low half), or k_datum_index_none when the array
 * is full.
 *
 * @address 0x4d0480
 */
datum_index data_array_view::new_datum()
{
    int16_t element_size;
    int16_t index;
    uint32_t result;
    int16_t *element;
    int16_t *scan;
    uint32_t words;
    uint32_t bytes;
    uint8_t *dst;

    element_size = this->size;
    index = this->next_index;
    result = halo::k_dword_none;
    element = (int16_t *)((int32_t)index * (int32_t)element_size + (int32_t)this->data);
    if (index < this->maximum_count) {
        while (*element != 0) {
            index = index + 1;
            element = (int16_t *)((uint8_t *)element + element_size);
            if (this->maximum_count <= index) {
                return result;
            }
        }
        dst = (uint8_t *)element;
        for (words = (uint32_t)(int32_t)element_size >> 2; words != 0; words = words - 1) {
            *(uint32_t *)dst = 0;
            dst = dst + 4;
        }
        for (bytes = (uint32_t)(int32_t)element_size & 3; bytes != 0; bytes = bytes - 1) {
            *dst = 0;
            dst = dst + 1;
        }
        *element = this->next_identifier;
        this->next_identifier = this->next_identifier + 1;
        if (this->next_identifier == 0) {
            this->next_identifier = (int16_t)k_datum_identifier_wrap;
        }
        this->actual_count = this->actual_count + 1;
        this->next_index = index + 1;
        if (this->last_index <= index) {
            this->last_index = index + 1;
        }
        result = (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
    }
    return result;
}

/**
 * Allocates the data_array slot at `index`, auto-assigning it a fresh generation salt via
 * datum_element_initialize. Fails (returns k_datum_index_none) if the index is out of range or the
 * slot is already in use.
 *
 * @address 0x4d0430
 */
datum_index data_array_view::new_at_index(int16_t index)
{
    int16_t *element;

    if (-1 < index && index < this->maximum_count) {
        element = (int16_t *)((int32_t)this->size * (int32_t)index + (int32_t)this->data);
        if (*element == 0) {
            this->actual_count = this->actual_count + 1;
            if (this->last_index <= index) {
                this->last_index = index + 1;
            }
            this->initialize_element(element);
            return (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
        }
        return halo::k_dword_none;
    }
    return halo::k_dword_none;
}

/**
 * Allocates the data_array slot at requested_handle's index, using requested_handle's own salt
 * (identifier) rather than an auto-assigned one. Fails (returns k_datum_index_none) if the index is
 * out of range, the caller-supplied salt is k_datum_identifier_none, or the slot is already in use. On
 * success the slot is zero-initialized (which also assigns it a fresh auto salt) and then its
 * identifier is overwritten with the caller-supplied salt; the returned handle carries that same
 * caller-supplied salt.
 *
 * @address 0x4d03d0
 */
datum_index data_array_view::new_at_index_with_salt(datum_index requested_handle)
{
    int16_t index;
    int16_t salt;
    int16_t *element;

    index = (int16_t)requested_handle;
    if (-1 < index && index < this->maximum_count) {
        salt = (int16_t)(requested_handle >> 0x10);
        if (salt != 0) {
            element = (int16_t *)((int32_t)this->size * (int32_t)index + (int32_t)this->data);
            if (*element == 0) {
                this->actual_count = this->actual_count + 1;
                if (this->last_index <= index) {
                    this->last_index = index + 1;
                }
                this->initialize_element(element);
                *element = salt;
                return (uint32_t)((int32_t)salt << 0x10) | (uint16_t)index;
            }
            return halo::k_dword_none;
        }
        return halo::k_dword_none;
    }
    return halo::k_dword_none;
}

/**
 * Finds the handle of the next in-use slot strictly after `after_index`, or k_datum_index_none if none
 * remain.
 *
 * @address 0x4d0630
 */
datum_index data_array_view::next_datum(int16_t after_index)
{
    uint32_t result;
    int16_t index;
    int16_t *element;

    result = halo::k_dword_none;
    index = after_index + 1;
    if (-1 < index && index < this->last_index) {
        element = (int16_t *)((int32_t)index * (int32_t)this->size + (int32_t)this->data);
        while (*element == 0) {
            index = index + 1;
            element = (int16_t *)((uint8_t *)element + this->size);
            if (this->last_index <= index) {
                return result;
            }
        }
        result = (uint32_t)((int32_t)*element << 0x10) | (uint16_t)index;
    }
    return result;
}

/**
 * Advances `iterator` to the next in-use element at or after its resume index, returning that
 * element's address (or NULL once the array is exhausted). On a real element, iterator->index is
 * updated to that element's handle; either way iterator->next_index is updated so a later call resumes
 * from where this one left off.
 *
 * @address 0x4d05d0
 */
void *data_iterator_view::next()
{
    int16_t resume;
    int16_t element_size;
    int16_t *element;
    int16_t *found;
    uint32_t handle_index;

    resume = this->next_index;
    found = 0;
    element_size = this->data->size;
    element = (int16_t *)((int32_t)resume * (int32_t)element_size +
                          (int32_t)this->data->data);
    if (resume < this->data->last_index) {
        for (;;) {
            found = element;
            handle_index = (uint32_t)(uint16_t)resume;
            resume = resume + 1;
            if (*found != 0) {
                break;
            }
            element = (int16_t *)((uint8_t *)found + element_size);
            if (this->data->last_index <= resume) {
                this->next_index = resume;
                return 0;
            }
        }
        this->index = (uint32_t)((int32_t)*found << 0x10) | handle_index;
    }
    this->next_index = resume;
    return found;
}

/**
 * Sets *out_index to the invalid datum_index sentinel (k_datum_index_none).
 *
 * @address 0x4d02c0
 */
void datum_index_invalidate(datum_index *out_index)
{
    *out_index = k_datum_index_none;
}

} // namespace halo::memory
