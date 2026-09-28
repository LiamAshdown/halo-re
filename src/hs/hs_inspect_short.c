// hs_inspect_short  (not a Ghidra function; entry type 7 (short) of the hs type inspector table 0x68bb48 that
//   hs_evaluate_inspect formats a result with; no C existed, so inspecting such a value trapped as unlisted_489af0)
// address 0x489af0, size 25 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489af0..: sprintf(buffer, "%d", (int16_t)value).
// blam-cc: stack -> type, value, buffer (cdecl)

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern int _sprintf(char *buffer, const char *format, ...); // 0x623693

void hs_inspect_short(int16_t type, int32_t value, char *buffer)
{
    _sprintf(buffer, "%d", (int32_t)(int16_t)value);
}
