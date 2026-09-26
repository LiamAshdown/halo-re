// string_assign_substr  (not a Ghidra function; MSVC 7.1 std::string::assign(const string &, size_type pos,
//   size_type count))
// address 0x57b830, size 227 bytes
// name confidence: 0.8  rewrite confidence: 0.85
// evidence: hwreq_parse_exception_construct 0x5782b0 copies its message with (right, 0, npos); msvc_string_assign_n
//   0x57bc90 forwards an aliasing source here. The body is the Dinkumware assign: pos past right's size calls _Xran
//   (0x638e74), the count is clamped to size - pos, self-assignment erases the tail then the head (string_erase
//   0x57bd80 twice), otherwise _Xlen above 0xfffffffe, _Grow when the capacity is short, copy, terminate.
// Rewritten from objdump 0x57b830..0x57b912. Quirk reproduced as in msvc_string_assign_n: _Grow's result is
//   ignored and a zero count with enough capacity empties the string.
// blam-cc: ECX -> this, stack -> right, pos, count; returns this

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void string_throw_out_of_range(void); // 0x638e74, _Xran: throws out_of_range("invalid string position")
extern void string_throw_length_error(void); // 0x638eb4, _Xlen: throws length_error("string too long")
extern void string_erase(msvc_std_string *this, uint32_t pos, uint32_t count); // 0x57bd80
extern void string_grow_reserve(msvc_std_string *this, uint32_t new_capacity, uint32_t preserve_count); // 0x57c6d0

static char *string_data(const msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : (char *)s->buffer.inline_buffer;
}

msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right, uint32_t pos,
    uint32_t count)
{
    uint32_t available;
    const char *source;
    char *data;
    uint32_t i;

    if (right->size < pos) {
        string_throw_out_of_range();
    }
    available = right->size - pos;
    if (count < available) {
        available = count;
    }
    if (this == right) {
        string_erase(this, pos + available, 0xffffffff);
        string_erase(this, 0, pos);
        return this;
    }
    if (available > 0xfffffffe) {
        string_throw_length_error();
    }
    if (this->capacity < available) {
        string_grow_reserve(this, available, this->size);
        if (available == 0) {
            return this;
        }
    } else if (available == 0) {
        this->size = 0;
        string_data(this)[0] = 0;
        return this;
    }
    source = string_data(right) + pos;
    data = string_data(this);
    for (i = 0; i < available; i++) {
        data[i] = source[i];
    }
    this->size = available;
    string_data(this)[available] = 0;
    return this;
}
