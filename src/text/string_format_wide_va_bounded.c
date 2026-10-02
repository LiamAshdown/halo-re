// string_format_wide_va_bounded  (Ghidra: string_format_wide_va_bounded, already named)
// address 0x557910, size 25 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/text_types_notes.md: "0x557910 / 0x557930 wrap the CRT wide
//   vsnwprintf / vswprintf ... they operate on no struct." objdump 0x557910..0x557930
//   confirmed the exact forwarding order (dest, count, format, varargs) to the bounded
//   wide vswprintf at 0x627c73 (Ghidra's fid-conflict placeholder name for _vsnwprintf).
// register convention: EDX = count (in_EDX, never loaded from the stack in this
//   function). dest and format are this function's own two stack arguments; this
//   function's own varargs (the 3rd stack slot onward) are forwarded unchanged.

#include "tags.h"
#include "memory.h"
#include "text.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// blam-cc: 0x627c73, Ghidra's fid-conflict placeholder name for the CRT bounded wide
// vswprintf (_vsnwprintf): dest, count, format, forwarded va_list, all cdecl stack args
extern int _vsnwprintf(uint16_t *buffer, uint32_t count, const uint16_t *format, va_list args); // CRT _vsnwprintf (0x627c73: count in characters, doubled to bytes)

// blam-cc: EDX=count, stack=(dest, format, ...)
// Forwards to the CRT's bounded wide vswprintf (writes at most count code units,
// including the terminating NUL, into dest).
void string_format_wide_va_bounded(uint32_t count, uint16_t *dest, const uint16_t *format, ...)
{
    va_list args;

    va_start(args, format);
    _vsnwprintf(dest, count, format, args);
    va_end(args);
}

#if 0
Original Ghidra decompilation (0x557910):

void string_format_wide_va_bounded(wchar_t *param_1,wchar_t *param_2)

{
  size_t in_EDX;

  FID_conflict_vswprintf(param_1,in_EDX,param_2,&stack0x0000000c);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
