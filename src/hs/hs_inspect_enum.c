// hs_inspect_enum  (not a Ghidra function; entry types 0x20..0x24 (the enums) of the hs type inspector table 0x68bb48 that
//   hs_evaluate_inspect formats a result with; no C existed, so inspecting such a value trapped as unlisted_489b50)
// address 0x489b50, size 40 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x489b50..: sprintf(buffer, "%s", names[(int16_t)value]) from the enum record at 0x65b538 + type * 8.
// blam-cc: stack -> type, value, buffer (cdecl)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "hs.h"


extern hs_enum_definition hs_enum_definitions[5]; // 0x0065b638, indexed by (type - 0x20)

void hs_inspect_enum(int16_t type, int32_t value, char *buffer)
{
    sprintf(buffer, "%s", hs_enum_definitions[type - 0x20].names[(int16_t)value]);
}
