// std_exception_what  (not a Ghidra function; the what() slot of the std::logic_error / length_error / out_of_range
//   vtables 0x00655084, 0x00655090, 0x0065509c; no C existed, so those stored pointers trapped)
// address 0x578380, size 14 bytes
// name confidence: 0.8   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x578380..0x57838d: returns the message string's c_str() -- the heap pointer at
//   +0x10 when its capacity (+0x24) is 0x10 or more, else the inline buffer at +0x10.
// The C++ runtime calls it through the vtable with __thiscall (this in ECX, arguments on the stack, callee
//   pops); __fastcall has exactly that shape for a first pointer argument (the EDX slot is unused).
// blam-cc: ECX this

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

const char *__fastcall std_exception_what(uint8_t *self, void *unused_edx)
{
    (void)unused_edx;
    return ((std_runtime_error *)self)->message.capacity >= 0x10 ? ((std_runtime_error *)self)->message.bx.pointer : ((std_runtime_error *)self)->message.bx.buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
