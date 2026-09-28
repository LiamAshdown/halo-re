// hs_inspect_real  (not a Ghidra function; entry type 6 (real) of the hs type inspector table 0x68bb48 that
//   hs_evaluate_inspect formats a result with; no C existed, so inspecting such a value trapped as unlisted_489ad0)
// address 0x489ad0, size 29 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489ad0..: sprintf(buffer, "%f", (double)value-as-float).
// blam-cc: stack -> type, value, buffer (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int _sprintf(char *buffer, const char *format, ...); // 0x623693

void hs_inspect_real(int16_t type, int32_t value, char *buffer)
{
    union { int32_t i; float f; } bits;

    bits.i = value;
    _sprintf(buffer, "%f", (double)bits.f);
}
