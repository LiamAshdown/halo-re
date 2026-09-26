// msvc_string_assign_n  (not a Ghidra function; MSVC 7.1 std::string::assign(const char *, size_type))
// address 0x57bc90, size 234 bytes
// name confidence: 0.8  rewrite confidence: 0.85
// evidence: every hwreq parser pass calls it with (pointer, strlen) to set a std::string; the body is the
//   Dinkumware assign: an aliasing source goes through assign(const string &, pos, count) (0x57b830), a count
//   above max_size (0xfffffffe) calls _Xlen (0x638eb4), a short capacity calls _Grow (string_grow_reserve
//   0x57c6d0) with the current size, then the bytes are copied and terminated.
//   First-boot track: the standalone exe needs it for shell_parse_config_txt's config.txt error message.
// Rewritten from objdump 0x57bc90..0x57bd79. Quirk reproduced: after _Grow the result is ignored, and a zero
//   count with enough capacity empties the string (size 0, terminator at the buffer start).
// blam-cc: ECX -> this, stack -> source, count; returns this

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern msvc_std_string *string_assign_substr(msvc_std_string *this, const msvc_std_string *right, uint32_t pos,
    uint32_t count); // 0x57b830, blam-cc: ECX -> this, stack -> right, pos, count
extern void string_throw_length_error(void); // 0x638eb4, _Xlen: throws length_error("string too long")
extern void string_grow_reserve(msvc_std_string *this, uint32_t new_capacity, uint32_t preserve_count); // 0x57c6d0

static char *string_data(msvc_std_string *s)
{
    return s->capacity >= 0x10 ? (char *)s->buffer.heap_buffer : s->buffer.inline_buffer;
}

msvc_std_string *msvc_string_assign_n(msvc_std_string *this, const char *source, uint32_t count)
{
    char *data = string_data(this);
    uint32_t i;

    if (source >= data && data + this->size > source) {
        return string_assign_substr(this, this, (uint32_t)(source - data), count);
    }
    if (count > 0xfffffffe) {
        string_throw_length_error();
    }
    if (this->capacity < count) {
        string_grow_reserve(this, count, this->size);
        if (count == 0) {
            return this;
        }
    } else if (count == 0) {
        this->size = 0;
        string_data(this)[0] = 0;
        return this;
    }
    data = string_data(this);
    for (i = 0; i < count; i++) {
        data[i] = source[i];
    }
    this->size = count;
    string_data(this)[count] = 0;
    return this;
}
