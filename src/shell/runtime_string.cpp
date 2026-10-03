#include "halo/shell/runtime.hpp"
#include "halo/shell/layout.hpp"


namespace halo::shell {

namespace {

uint32_t pair_count(uint32_t from, uint32_t to)
{
    return (uint32_t)(((int32_t)to - (int32_t)from) / (int32_t)sizeof(hwreq_string_pair));
}

}

/**
 * std::string::assign(right, pos, count): copies a substring of right into this string, growing it
 * as needed. Self assignment erases the rest instead.
 *
 * @address 0x57b830
 */
msvc_std_string *StdString::assign_substr(const msvc_std_string *right, uint32_t pos, uint32_t count)
{
    uint32_t available;
    const char *source;
    char *data;
    uint32_t i;

    if (right->size < pos) {
        StdThrow::string_out_of_range();
    }
    available = right->size - pos;
    if (count < available) {
        available = count;
    }
    if (self == right) {
        erase(pos + available, k_string_npos);
        erase(0, pos);
        return self;
    }
    if (available > k_string_npos - 1) {
        StdThrow::string_too_long();
    }
    if (self->capacity < available) {
        grow_reserve(available, self->size);
        if (available == 0) {
            return self;
        }
    } else if (available == 0) {
        self->size = 0;
        StdString(self).data()[0] = 0;
        return self;
    }
    source = StdString(right).data() + pos;
    data = StdString(self).data();
    for (i = 0; i < available; i++) {
        data[i] = source[i];
    }
    self->size = available;
    StdString(self).data()[available] = 0;
    return self;
}

/**
 * std::string::assign(source, count): replaces the contents with count characters, handling the
 * case where source points into the string itself.
 *
 * @address 0x57bc90
 */
msvc_std_string *StdString::assign_n(const char *source, uint32_t count)
{
    char *data = StdString(self).data();
    uint32_t i;

    if (source >= data && data + self->size > source) {
        return assign_substr(self, (uint32_t)(source - data), count);
    }
    if (count > k_string_npos - 1) {
        StdThrow::string_too_long();
    }
    if (self->capacity < count) {
        grow_reserve(count, self->size);
        if (count == 0) {
            return self;
        }
    } else if (count == 0) {
        self->size = 0;
        StdString(self).data()[0] = 0;
        return self;
    }
    data = StdString(self).data();
    for (i = 0; i < count; i++) {
        data[i] = source[i];
    }
    self->size = count;
    StdString(self).data()[count] = 0;
    return self;
}

/**
 * std::string::compare(pos, n1, s, n2): three way comparison of a substring against a character
 * range. Raises the invalid string position error when pos is past the end.
 *
 * @address 0x57ce10
 */
int32_t StdString::compare(uint32_t n1, uint32_t pos, const char *s, uint32_t n2)
{
    uint32_t remaining;
    uint32_t compare_count;
    int32_t result = 0;

    if (self->size < pos) {
        StdThrow::string_out_of_range();
    }

    remaining = self->size - pos;
    if (remaining < n1) {
        n1 = remaining;
    }

    if (n1 != 0) {
        const char *lhs;
        compare_count = (n2 <= n1) ? n2 : n1;

        lhs = StdString(self).data();
        lhs += pos;

        {
            uint32_t remaining_cmp = compare_count;
            uint8_t less = 0;
            uint8_t equal = 1;
            const uint8_t *l = (const uint8_t *)lhs;
            const uint8_t *r = (const uint8_t *)s;
            while (remaining_cmp != 0 && equal) {
                remaining_cmp--;
                less = *l < *r;
                equal = (*l == *r);
                l++;
                r++;
            }
            if (!equal) {
                result = less ? (uint32_t)-1 : 1;
            }
        }
        if (result != 0) {
            return result;
        }
    }

    if (n1 < n2) {
        return -1;
    }
    return (n1 != n2);
}

/**
 * std::string::erase(pos, count): removes up to count characters starting at pos.
 *
 * @address 0x57bd80
 */
msvc_std_string *StdString::erase(uint32_t pos, uint32_t count)
{
    uint32_t remaining;
    char *buffer;
    uint32_t new_size;

    if (self->size < pos) {
        StdThrow::string_out_of_range();
    }

    remaining = self->size - pos;
    if (remaining < count) {
        count = remaining;
    }

    if (count == 0) {
        return self;
    }

    buffer = StdString(self).data();
    memmove(buffer + pos, buffer + pos + count, remaining - count);

    new_size = self->size - count;
    self->size = new_size;

    buffer = StdString(self).data();
    buffer[new_size] = 0;
    return self;
}

/**
 * Reallocates the buffer to hold at least new_capacity characters, keeping the first preserve_count
 * characters and applying the 1.5x growth rule.
 *
 * @address 0x57c6d0
 */
void StdString::grow_reserve(uint32_t new_capacity, uint32_t preserve_count)
{
    uint32_t capacity = new_capacity | k_string_inline_capacity;
    char *new_buffer;
    char *terminator;

    if (capacity != k_string_npos) {
        uint32_t current_capacity = self->capacity;
        uint32_t half = current_capacity >> 1;
        if (capacity / 3 < half && current_capacity <= (k_string_npos - 1 - half)) {
            capacity = half + current_capacity;
        }
    } else {
        capacity = new_capacity;
    }

    new_buffer = (char *)malloc(capacity + 1);

    if (preserve_count != 0) {
        const char *old_buffer = StdString(self).data();
        uint32_t i;
        for (i = 0; i < preserve_count; i++) {
            new_buffer[i] = old_buffer[i];
        }
    }

    if (self->capacity > k_string_inline_capacity) {
        free(StdString::heap_pointer(*self));
    }

    self->buffer.inline_buffer[0] = 0;
    StdString::set_heap_pointer(*self, new_buffer);
    self->capacity = capacity;
    self->size = preserve_count;

    terminator = (capacity >= k_string_inline_capacity + 1) ? new_buffer : self->buffer.inline_buffer;
    terminator[preserve_count] = 0;
}

/**
 * Constructs the string from a NUL terminated C string.
 *
 * @address 0x57b520
 */
msvc_std_string *StdString::construct_cstr(const char *source)
{
    const char *end = source;

    self->capacity = k_string_inline_capacity;
    self->size = 0;
    self->buffer.inline_buffer[0] = 0;
    while (*end) {
        end++;
    }
    assign_n(source, (uint32_t)(end - source));
    return self;
}

/**
 * Frees the heap buffer if one is in use and resets the string to empty.
 *
 * @address 0x57b560
 */
void StdString::destroy()
{
    if (self->capacity >= k_string_inline_capacity + 1) {
        free(StdString::heap_pointer(*self));
    }
    self->capacity = k_string_inline_capacity;
    self->size = 0;
    self->buffer.inline_buffer[0] = 0;
}

/**
 * Assigns a NUL terminated C string to the string.
 *
 * @address 0x57b590
 */
msvc_std_string *StdString::assign_cstr(const char *s)
{
    const char *cursor = s;
    do {
        cursor++;
    } while (*(cursor - 1) != '\0');
    return assign_n(s, (uint32_t)(cursor - (s + 1)));
}

/**
 * operator< on the two strings, using the same comparison as the map.
 *
 * @address 0x57bbd0
 */
uint8_t StdString::less_than(const msvc_std_string *other)
{
    const char *other_data = StdString(other).data();
    return compare(self->size, 0, other_data, other->size) < 0;
}

/**
 * Constructs the pair from copies of the two strings.
 *
 * @address 0x57b670
 */
hwreq_string_pair *StringPair::construct(const msvc_std_string *first_source, const msvc_std_string *second_source)
{
    self->first.capacity = k_string_inline_capacity;
    self->first.size = 0;
    self->first.buffer.inline_buffer[0] = 0;
    StdString(&self->first).assign_substr(first_source, 0, k_string_npos);

    self->second.capacity = k_string_inline_capacity;
    self->second.size = 0;
    self->second.buffer.inline_buffer[0] = 0;
    StdString(&self->second).assign_substr(second_source, 0, k_string_npos);

    return self;
}

/**
 * Copy constructs the pair from source.
 *
 * @address 0x57c640
 */
hwreq_string_pair *StringPair::copy_construct(const hwreq_string_pair *source)
{
    self->first.capacity = k_string_inline_capacity;
    self->first.size = 0;
    self->first.buffer.inline_buffer[0] = 0;
    StdString(&self->first).assign_substr(&source->first, 0, k_string_npos);

    self->second.capacity = k_string_inline_capacity;
    self->second.size = 0;
    self->second.buffer.inline_buffer[0] = 0;
    StdString(&self->second).assign_substr(&source->second, 0, k_string_npos);

    return self;
}

/**
 * Destructs a heap-embedded pair of strings, releasing any out-of-line buffers each string owns.
 *
 * @address 0x5785b0
 */
void StringPair::destroy()
{
    if (self->second.capacity > k_msvc_string_inline_capacity) {
        free(StdString::heap_pointer(self->second));
    }
    self->second.capacity = k_msvc_string_inline_capacity;
    self->second.size = 0;
    self->second.buffer.inline_buffer[0] = 0;

    if (self->first.capacity > k_msvc_string_inline_capacity) {
        free(StdString::heap_pointer(self->first));
    }
    self->first.size = 0;
    self->first.capacity = k_msvc_string_inline_capacity;
    self->first.buffer.inline_buffer[0] = 0;
}

/**
 * std::copy_backward over a range of pairs, assigning both strings. Returns the new destination
 * start.
 *
 * @address 0x57cf10
 */
hwreq_string_pair *StringPair::copy_backward(hwreq_string_pair *first, hwreq_string_pair *last, hwreq_string_pair *dest_end)
{
    while (last != first) {
        last--;
        dest_end--;
        StdString(&dest_end->first).assign_substr(&last->first, 0, k_string_npos);
        StdString(&dest_end->second).assign_substr(&last->second, 0, k_string_npos);
    }
    return dest_end;
}

/**
 * Destroys every pair in [first, last).
 *
 * @address 0x57be00
 */
void StringPair::destroy_range(hwreq_string_pair *first, hwreq_string_pair *last)
{
    while (first != last) {
        StringPair(first).destroy();
        first = (hwreq_string_pair *)((uint8_t *)first + 0x38);
    }
}

/**
 * Assigns value to every pair in [first, last).
 *
 * @address 0x57cda0
 */
void StringPair::fill_range(hwreq_string_pair *first, hwreq_string_pair *last, const hwreq_string_pair *value)
{
    while (first != last) {
        StdString(&first->first).assign_substr(&value->first, 0, k_string_npos);
        StdString(&first->second).assign_substr(&value->second, 0, k_string_npos);
        first = (hwreq_string_pair *)((uint8_t *)first + 0x38);
    }
}

/**
 * Copy constructs [source_begin, source_end) into raw storage at dest and returns the end of the
 * copy.
 *
 * @address 0x57cf50
 */
hwreq_string_pair *StringPair::uninit_copy(hwreq_string_pair *source_begin, hwreq_string_pair *source_end, hwreq_string_pair *dest)
{
    while (source_begin != source_end) {
        if (dest != 0) {
            StringPair(dest).copy_construct(source_begin);
        }
        dest = (hwreq_string_pair *)((uint8_t *)dest + 0x38);
        source_begin = (hwreq_string_pair *)((uint8_t *)source_begin + 0x38);
    }
    return dest;
}

/**
 * Copy constructs count copies of value into raw storage at dest.
 *
 * @address 0x57ce80
 */
void StringPair::uninit_fill_n(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value)
{
    while (count != 0) {
        if (dest != 0) {
            StringPair(dest).copy_construct(value);
        }
        dest = (hwreq_string_pair *)((uint8_t *)dest + 0x38);
        count--;
    }
}

/**
 * Number of elements in the vector.
 *
 * @address 0x57b5b0
 */
int32_t PairVector::element_count()
{
    if (self->first == 0) {
        return 0;
    }
    return ((int32_t)self->last - (int32_t)self->first) / 0x38;
}

/**
 * vector::push_back: constructs the element in place when capacity remains, otherwise inserts at
 * the end with reallocation.
 *
 * @address 0x57b5e0
 */
void PairVector::push_back(const hwreq_string_pair *value)
{
    if (self->first != 0 &&
        (uint32_t)(((int32_t)self->last - (int32_t)self->first) / 0x38) <
        (uint32_t)(((int32_t)self->end - (int32_t)self->first) / 0x38)) {
        void *dest = (void *)self->last;
        StringPair::uninit_fill_n((hwreq_string_pair *)dest, 1, value);
        self->last = (uint32_t)((uint8_t *)dest + 0x38);
        return;
    }
    {
        hwreq_string_pair *result;
        insert(&result, (hwreq_string_pair *)self->last, value);
    }
}

/**
 * vector::insert(where, value): inserts one element and reports the resulting position through
 * result.
 *
 * @address 0x57b920
 */
hwreq_string_pair **PairVector::insert(hwreq_string_pair **result, hwreq_string_pair *where, const hwreq_string_pair *value)
{
    int32_t offset = 0;

    if (self->first != 0 && (int32_t)(self->last - self->first) / (int32_t)sizeof(hwreq_string_pair) != 0) {
        offset = ((int32_t)where - (int32_t)self->first) / (int32_t)sizeof(hwreq_string_pair);
    }
    insert_n(where, 1, value);
    *result = (hwreq_string_pair *)self->first + offset;
    return result;
}

/**
 * vector::insert(where, count, value): inserts count copies of value, reallocating with the 1.5x
 * growth rule when capacity runs out. Raises the vector too long error past the maximum size.
 *
 * @address 0x57be20
 */
void PairVector::insert_n(hwreq_string_pair *where, uint32_t count, const hwreq_string_pair *value)
{
    hwreq_string_pair temporary;
    uint32_t capacity = 0;
    uint32_t size;

    StringPair(&temporary).copy_construct(value);
    if (self->first != 0) {
        capacity = pair_count(self->first, self->end);
    }
    if (count != 0) {
        size = self->first != 0 ? pair_count(self->first, self->last) : 0;
        if (0x4924924 - size < count) {
            StdThrow::vector_too_long();
        }
        size = self->first != 0 ? pair_count(self->first, self->last) : 0;
        if (capacity < size + count) {
            hwreq_string_pair *new_first;
            hwreq_string_pair *cursor;
            uint32_t half = capacity >> 1;

            capacity = 0x4924924 - half < capacity ? 0 : capacity + half;
            size = self->first != 0 ? pair_count(self->first, self->last) : 0;
            if (capacity < size + count) {
                capacity = (uint32_t)element_count() + count;
            }
            new_first = (hwreq_string_pair *)malloc(capacity * sizeof(hwreq_string_pair));
            cursor = StringPair::uninit_copy((hwreq_string_pair *)self->first, where, new_first);
            StringPair::uninit_fill_n(cursor, count, &temporary);
            StringPair::uninit_copy(where, (hwreq_string_pair *)self->last, cursor + count);
            if (self->first != 0) {
                count += pair_count(self->first, self->last);
                StringPair::destroy_range((hwreq_string_pair *)self->first, (hwreq_string_pair *)self->last);
                free((void *)self->first);
            }
            self->end = (uint32_t)(new_first + capacity);
            self->last = (uint32_t)(new_first + count);
            self->first = (uint32_t)new_first;
        } else if (pair_count((uint32_t)where, self->last) < count) {
            StringPair::uninit_copy(where, (hwreq_string_pair *)self->last, where + count);
            StringPair::uninit_fill_n((hwreq_string_pair *)self->last, count - pair_count((uint32_t)where, self->last), &temporary);
            self->last = (uint32_t)((hwreq_string_pair *)self->last + count);
            StringPair::fill_range(where, (hwreq_string_pair *)self->last - count, &temporary);
        } else {
            hwreq_string_pair *old_last = (hwreq_string_pair *)self->last;

            self->last = (uint32_t)StringPair::uninit_copy(old_last - count, old_last, old_last);
            StringPair::copy_backward(where, old_last - count, old_last);
            StringPair::fill_range(where, where + count, &temporary);
        }
    }
    StringPair(&temporary).destroy();
}

/**
 * Sets key to value (case-insensitive match on the key): rewrites the value of an existing entry,
 * or appends a new name/value pair.
 *
 * @address 0x578410
 */
void PropertySet::upsert(char *key, char *value)
{
    hwreq_string_pair *cursor;
    hwreq_string_pair *end;
    const char *first_text;

    cursor = (hwreq_string_pair *)self->flags.first;
    end = (hwreq_string_pair *)self->flags.last;

    while (cursor != end) {
        first_text = (cursor->first.capacity < k_msvc_string_heap_capacity) ? cursor->first.buffer.inline_buffer
                                                       : StdString::heap_pointer(cursor->first);
        if (_stricmp(first_text, key) == 0) {
            StdString(&cursor->second).assign_n(value, cstr_length(value));
            return;
        }
        cursor++;
    }

    {
        msvc_std_string key_string;
        msvc_std_string value_string;
        hwreq_string_pair new_pair;

        value_string.capacity = k_msvc_string_inline_capacity;
        value_string.size = 0;
        value_string.buffer.inline_buffer[0] = 0;
        StdString(&value_string).assign_n(value, cstr_length(value));
        key_string.capacity = k_msvc_string_inline_capacity;
        key_string.size = 0;
        key_string.buffer.inline_buffer[0] = 0;
        StdString(&key_string).assign_n(key, cstr_length(key));

        StringPair(&new_pair).construct(&key_string, &value_string);
        PairVector(&self->flags).push_back(&new_pair);
        StringPair(&new_pair).destroy();

        if (value_string.capacity > k_msvc_string_inline_capacity) {
            free(StdString::heap_pointer(value_string));
        }
        if (key_string.capacity > k_msvc_string_inline_capacity) {
            free(StdString::heap_pointer(key_string));
        }
    }
}

/**
 * Merges every name/value pair of this set into target.
 *
 * @address 0x57b470
 */
void PropertySet::apply_to(hwreq_property_set *target)
{
    hwreq_string_pair *pair = (hwreq_string_pair *)self->flags.first;
    hwreq_string_pair *end = (hwreq_string_pair *)self->flags.last;

    for (; pair != end; pair++) {
        PropertySet(target).upsert(StdString(&pair->first).data(), StdString(&pair->second).data());
    }
}

/**
 * Looks up a key in the device-override list and, if found, copies its associated value string into
 * the caller-supplied buffer.
 *
 * @address 0x578630
 */
uint32_t PropertySet::find_value(const char *key, char *out_value, uint32_t capacity)
{
    hwreq_string_pair *cursor;
    hwreq_string_pair *end;
    const char *first_text;
    const char *second_text;

    cursor = (hwreq_string_pair *)self->flags.first;
    end = (hwreq_string_pair *)self->flags.last;

    while (cursor != end) {
        first_text = (cursor->first.capacity < k_msvc_string_heap_capacity) ? cursor->first.buffer.inline_buffer
                                                       : StdString::heap_pointer(cursor->first);
        if (_stricmp(first_text, key) == 0) {
            break;
        }
        cursor++;
    }
    if (cursor == end) {
        return 0;
    }

    second_text = (cursor->second.capacity < k_msvc_string_heap_capacity) ? cursor->second.buffer.inline_buffer
                                                     : StdString::heap_pointer(cursor->second);
    strncpy(out_value, second_text, capacity);
    return 1;
}

/**
 * Destroys every pair, frees the vector storage and empties the set.
 *
 * @address 0x57b990
 */
void PropertySet::destroy_flags()
{
    hwreq_string_pair *pair = (hwreq_string_pair *)self->flags.first;

    if (pair != 0) {
        hwreq_string_pair *last = (hwreq_string_pair *)self->flags.last;

        for (; pair != last; pair++) {
            StringPair(pair).destroy();
        }
        free((void *)self->flags.first);
    }
    self->flags.first = 0;
    self->flags.last = 0;
    self->flags.end = 0;
}

/**
 * Allocates an empty property set owned by parser; returns null if the allocation fails.
 */
hwreq_property_set *PropertySet::create(hwreq_parser *parser)
{
    hwreq_property_set *set = (hwreq_property_set *)malloc(sizeof(hwreq_property_set));

    if (set == 0) {
        return 0;
    }
    set->flags.first = 0;
    set->flags.last = 0;
    set->flags.end = 0;
    set->owner = (uint32_t)parser;
    return set;
}

}
