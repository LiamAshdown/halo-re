// hs_cast_long_to_boolean  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48aab0, size 18 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entries 251 and 253 (to boolean from real / long).
// objdump 0x48aab0..0x48aac1: test eax,eax / sete al into the argument's low byte; the argument's upper bytes
//   are returned unchanged.
// blam-cc: stack -> value (cdecl)

#include "tags.h"

int32_t hs_cast_long_to_boolean(int32_t value)
{
    return (int32_t)(((uint32_t)value & 0xffffff00u) | (value == 0));
}
