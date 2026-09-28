// hs_inspect_long  (not a Ghidra function; entry type 8 (long) of the hs type inspector table 0x68bb48 that
//   hs_evaluate_inspect formats a result with; no C existed, so inspecting such a value trapped as unlisted_489b10)
// address 0x489b10, size 24 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489b10..: sprintf(buffer, "%ld", value).
// blam-cc: stack -> type, value, buffer (cdecl)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


void hs_inspect_long(int16_t type, int32_t value, char *buffer)
{
    sprintf(buffer, "%ld", value);
}
