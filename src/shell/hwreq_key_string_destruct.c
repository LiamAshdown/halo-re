// hwreq_key_string_destruct  (Ghidra: FUN_0057b560; MSVC 7.1 std::string::~string / _Tidy(true))
// address 0x57b560, size 38 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence: hwreq_parser_parse_block releases its map keys with it. objdump 0x57b560..0x57b585: a heap buffer
//   (capacity >= 0x10) is freed, then capacity 0xf, size 0 and an empty inline buffer.
// blam-cc: ECX -> this

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"

extern void free(void *block); // CRT: 0x6277e8

void hwreq_key_string_destruct(msvc_std_string *this)
{
    if (this->capacity >= 0x10) {
        free((void *)this->buffer.heap_buffer);
    }
    this->capacity = 0xf;
    this->size = 0;
    this->buffer.inline_buffer[0] = 0;
}
