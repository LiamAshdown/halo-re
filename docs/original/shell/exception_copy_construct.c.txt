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
#include "rasterizer.h"
#include "shell.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void *std_exception_vtable; // 0x0064ef90

void *exception_copy_construct(void *self_, const void *other)
{
    std_exception *self = (std_exception *)self_;
    const std_exception *source = (const std_exception *)other;

    self->vftable = &std_exception_vtable;
    self->do_free = source->do_free;
    if (source->do_free != 0) {
        const char *message = source->what;
        char *copy = (char *)malloc(strlen(message) + 1);

        self->what = copy;
        if (copy != 0) {
            strcpy(copy, message);
        }
    } else {
        self->what = source->what;
    }
    return self_;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
