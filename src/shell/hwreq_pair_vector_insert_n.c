// hwreq_pair_vector_insert_n  (Ghidra: FUN_0057be20; MSVC 7.1 vector<hwreq_string_pair>::_Insert_n)
// address 0x57be20, size 713 bytes (with its two catch funclets 0x57bfec and 0x57c0a8)
// name confidence: 0.8  rewrite confidence: 0.8
// evidence: hwreq_pair_vector_insert 0x57b920 calls it with count 1. objdump 0x57be20..0x57c128 is Dinkumware's
//   _Insert_n statement for statement:
//   - _Tmp = value (the pair copy constructor 0x57c640, ECX value), capacity = (end - first) / 0x38 (0 if never
//     allocated); nothing happens for count 0
//   - max_size 0x4924924: max_size - size < count -> _Xlen (0x57c130)
//   - capacity < size + count: grow by half (0 when max_size - capacity/2 < capacity), at least size + count
//     (hwreq_device_list_size 0x57b5b0), operator new(capacity * 0x38); _Ucopy the prefix, _Ufill count copies of
//     _Tmp, _Ucopy the suffix; destroy and free the old array; first / last / end set from the new array
//   - (last - where) / 0x38 < count: _Ucopy [where, last) to where + count, _Ufill the part past the old end,
//     last += count, fill [where, old last) with _Tmp
//   - otherwise: _Ucopy [last - count, last) to last, copy_backward [where, last - count) to the old last, fill
//     [where, where + count) with _Tmp
//   - _Tmp is destroyed.
//   Not reproduced: the two catch(...) funclets, which destroy the partial copy (and free the new array) and
//   rethrow. Nothing here throws on this path: operator new is the VC6 one that returns NULL, and the pair copies
//   cannot exceed the string max size.
// blam-cc: ECX -> value, stack -> this, where, count

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern hwreq_string_pair *uninit_copy_string_pair(hwreq_string_pair *source_begin, hwreq_string_pair *source_end,
    hwreq_string_pair *dest); // 0x57cf50, blam-cc: ECX -> source_begin, stack -> source_end, dest
extern void uninit_fill_n_string_pair(hwreq_string_pair *dest, uint32_t count, const hwreq_string_pair *value); // 0x57ce80
extern void destroy_range_string_pair(hwreq_string_pair *first, hwreq_string_pair *last); // 0x57be00, EAX first, EDI last
extern void fill_string_pair_range(hwreq_string_pair *first, hwreq_string_pair *last, const hwreq_string_pair *value); // 0x57cda0
extern hwreq_string_pair *string_pair_construct_empty(hwreq_string_pair *dest, const hwreq_string_pair *source); // 0x57c640 (a copy constructor)
extern void hwreq_string_pair_destruct(hwreq_string_pair *pair); // 0x5785b0
extern int32_t hwreq_device_list_size(const msvc_std_vector *this); // 0x57b5b0
extern hwreq_string_pair *copy_backward_string_pair(hwreq_string_pair *first, hwreq_string_pair *last,
    hwreq_string_pair *dest_end); // 0x57cf10, blam-cc: EBX -> first, ECX -> last, EAX -> dest_end
extern void hwreq_pair_vector_throw_length_error(void); // 0x57c130, throws length_error("vector<T> too long")
extern void *operator_new(uint32_t size); // 0x6277da, MSVC CRT
extern void free(void *block); // 0x6277e8, CRT free

static uint32_t pair_count(uint32_t from, uint32_t to)
{
    return (uint32_t)(((int32_t)to - (int32_t)from) / (int32_t)sizeof(hwreq_string_pair));
}

void hwreq_pair_vector_insert_n(msvc_std_vector *this, hwreq_string_pair *where, uint32_t count,
    const hwreq_string_pair *value)
{
    hwreq_string_pair temporary;
    uint32_t capacity = 0;
    uint32_t size;

    string_pair_construct_empty(&temporary, value);
    if (this->first != 0) {
        capacity = pair_count(this->first, this->end);
    }
    if (count != 0) {
        size = this->first != 0 ? pair_count(this->first, this->last) : 0;
        if (0x4924924 - size < count) {
            hwreq_pair_vector_throw_length_error();
        }
        size = this->first != 0 ? pair_count(this->first, this->last) : 0;
        if (capacity < size + count) {
            hwreq_string_pair *new_first;
            hwreq_string_pair *cursor;
            uint32_t half = capacity >> 1;

            capacity = 0x4924924 - half < capacity ? 0 : capacity + half;
            size = this->first != 0 ? pair_count(this->first, this->last) : 0;
            if (capacity < size + count) {
                capacity = (uint32_t)hwreq_device_list_size(this) + count;
            }
            new_first = (hwreq_string_pair *)operator_new(capacity * sizeof(hwreq_string_pair));
            cursor = uninit_copy_string_pair((hwreq_string_pair *)this->first, where, new_first);
            uninit_fill_n_string_pair(cursor, count, &temporary);
            uninit_copy_string_pair(where, (hwreq_string_pair *)this->last, cursor + count);
            if (this->first != 0) {
                count += pair_count(this->first, this->last);
                destroy_range_string_pair((hwreq_string_pair *)this->first, (hwreq_string_pair *)this->last);
                free((void *)this->first);
            }
            this->end = (uint32_t)(new_first + capacity);
            this->last = (uint32_t)(new_first + count);
            this->first = (uint32_t)new_first;
        } else if (pair_count((uint32_t)where, this->last) < count) {
            uninit_copy_string_pair(where, (hwreq_string_pair *)this->last, where + count);
            uninit_fill_n_string_pair((hwreq_string_pair *)this->last, count - pair_count((uint32_t)where, this->last),
                                      &temporary);
            this->last = (uint32_t)((hwreq_string_pair *)this->last + count);
            fill_string_pair_range(where, (hwreq_string_pair *)this->last - count, &temporary);
        } else {
            hwreq_string_pair *old_last = (hwreq_string_pair *)this->last;

            this->last = (uint32_t)uninit_copy_string_pair(old_last - count, old_last, old_last);
            copy_backward_string_pair(where, old_last - count, old_last);
            fill_string_pair_range(where, where + count, &temporary);
        }
    }
    hwreq_string_pair_destruct(&temporary);
}
