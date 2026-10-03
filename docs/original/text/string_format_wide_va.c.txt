// string_format_wide_va  (Ghidra: string_format_wide_va, already named)
// address 0x557930, size 20 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/text_types_notes.md: "0x557910 / 0x557930 wrap the CRT wide
//   vsnwprintf / vswprintf ... they operate on no struct." objdump 0x557930..0x557950
//   confirmed the forwarding order (dest, format, varargs) to the unbounded CRT wide
//   vswprintf at 0x627d49 (Ghidra: "_vswprintf").
// register convention: EDX = dest (in_EDX, never loaded from the stack in this
//   function). format is this function's own sole stack argument; this function's own
//   varargs (the 2nd stack slot onward) are forwarded unchanged.

#include "tags.h"
#include "memory.h"
#include "text.h"
#include <stdarg.h>

// blam-cc: 0x627d49, Ghidra: _vswprintf. The unbounded CRT wide vswprintf: dest, format,
// forwarded va_list, all cdecl stack args.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int _vswprintf(uint16_t *buffer, const uint16_t *format, va_list args); // CRT legacy _vswprintf (0x627d49: no count, the VC7.1 non-conforming form)

// blam-cc: EDX=dest, stack=(format, ...)
// Forwards to the CRT's unbounded wide vswprintf.
void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    _vswprintf(dest, format, args);
    va_end(args);
}

#if 0
Original Ghidra decompilation (0x557930):

void string_format_wide_va(wchar_t *param_1)

{
  wchar_t *in_EDX;

  _vswprintf(in_EDX,param_1,&stack0x00000008);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
