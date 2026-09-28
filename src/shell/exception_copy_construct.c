// exception_copy_construct  (CRT library code: std::exception::exception(const exception &), MSVC 7.1)
// address 0x627dd2, size 74 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x627dd2..0x627e1b: stores the std::exception vtable (0x0064ef90), copies the
//   owns-message flag (+0x08); an owned message (+0x04) is duplicated (malloc(strlen + 1), strcpy; a failed malloc
//   leaves NULL), otherwise the pointer is shared. Returns this. Reached from hwreq_parse_exception_copy_construct,
//   which calls it as an ordinary C function (the binary passes this in ECX).
// blam-cc: ECX this, stack -> other (callee pops 4)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>
#include <stdlib.h>

extern void *std_exception_vtable; // 0x0064ef90

void *exception_copy_construct(void *this, const void *other)
{
    uint8_t *self = (uint8_t *)this;
    const uint8_t *source = (const uint8_t *)other;

    *(void **)self = &std_exception_vtable;
    *(uint32_t *)(self + 8) = *(const uint32_t *)(source + 8);
    if (*(const uint32_t *)(source + 8) != 0) {
        const char *message = *(const char *const *)(source + 4);
        char *copy = (char *)malloc(strlen(message) + 1);

        *(char **)(self + 4) = copy;
        if (copy != 0) {
            strcpy(copy, message);
        }
    } else {
        *(const char **)(self + 4) = *(const char *const *)(source + 4);
    }
    return this;
}
