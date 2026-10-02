// std_length_error_copy_construct  (not a Ghidra function; the copy constructor in the std::length_error catchable type at 0x0067355c;
//   no C existed, so the stored pointer trapped)
// address 0x57c6b0, size 25 bytes
// name confidence: 0.8   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x57c6b0..0x57c6c6: copy constructs the logic_error part
//   (hwreq_parse_exception_copy_construct 0x57bc20), then stores the std::length_error vtable (0x0065508c); returns this.
// The C++ runtime calls it through the catchable type with __thiscall (this in ECX, arguments on the stack, callee
//   pops); __fastcall has exactly that shape for a first pointer argument (the EDX slot is unused).
// blam-cc: ECX this, stack -> other (callee pops 4)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

typedef struct hwreq_parse_exception {
    uint32_t vtable;         // 0x00
    uint32_t dofree;         // 0x04
    uint32_t legacy_what;    // 0x08
    msvc_std_string message; // 0x0c
} hwreq_parse_exception; // size 0x28

extern hwreq_parse_exception *hwreq_parse_exception_construct(hwreq_parse_exception *self,
    const msvc_std_string *message); // 0x5782b0, blam-cc: ECX -> this, stack -> message

extern void hwreq_parse_exception_copy_construct(hwreq_parse_exception *self, const hwreq_parse_exception *other); // 0x57bc20
extern void *length_error_vtable; // 0x0065508c

hwreq_parse_exception *__fastcall std_length_error_copy_construct(hwreq_parse_exception *self, void *unused_edx,
    const hwreq_parse_exception *other)
{
    (void)unused_edx;
    hwreq_parse_exception_copy_construct(self, other);
    self->vtable = (uint32_t)&length_error_vtable;
    return self;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
