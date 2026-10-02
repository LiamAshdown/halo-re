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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void free(void *block); // CRT: 0x6277e8

void hwreq_key_string_destruct(msvc_std_string *self)
{
    if (self->capacity >= 0x10) {
        free((void *)self->buffer.heap_buffer);
    }
    self->capacity = 0xf;
    self->size = 0;
    self->buffer.inline_buffer[0] = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
