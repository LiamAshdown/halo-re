// hwreq_key_string_construct_cstr  (Ghidra: FUN_0057b520; MSVC 7.1 std::string::string(const char *))
// address 0x57b520, size 57 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: hwreq_parser_parse_block builds its map keys with it; the _Xran/_Xlen throwers (0x638e74, 0x638eb4)
//   build their messages with it. objdump 0x57b520..0x57b558: capacity 0xf, size 0, empty inline buffer, then
//   msvc_string_assign_n(this, source, strlen(source)). Returns this.
// blam-cc: ECX -> this, stack -> source; returns this

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern msvc_std_string *msvc_string_assign_n(msvc_std_string *self, const char *source, uint32_t count); // 0x57bc90

msvc_std_string *hwreq_key_string_construct_cstr(msvc_std_string *self, const char *source)
{
    const char *end = source;

    self->capacity = 0xf;
    self->size = 0;
    self->buffer.inline_buffer[0] = 0;
    while (*end) {
        end++;
    }
    msvc_string_assign_n(self, source, (uint32_t)(end - source));
    return self;
}
