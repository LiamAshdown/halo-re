// exception_destruct  (CRT library code: std::exception::~exception, MSVC 7.1)
// address 0x627e1c, size 22 bytes
// name confidence: 0.9   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x627e1c..0x627e31: restores the std::exception vtable (0x0064ef90) and frees the
//   message (+0x04) when the owns-message flag (+0x08) is set. Reached from hwreq_parse_exception_destruct as an
//   ordinary C function (the binary passes this in ECX).
// blam-cc: ECX this

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

void exception_destruct(void *self_)
{
    std_exception *self = (std_exception *)self_;

    self->vftable = &std_exception_vtable;
    if (self->do_free != 0) {
        free((void *)self->what);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
