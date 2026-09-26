// hs_cast_short_to_real  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48ab10, size 22 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entry 301 (to real from short).
// objdump 0x48ab10..0x48ab25: movsx of the word, fild, fstp as a float, returned as its bits.
// blam-cc: stack -> value (cdecl)

#include "tags.h"

int32_t hs_cast_short_to_real(int32_t value)
{
    float result = (float)(int16_t)value;

    return *(int32_t *)&result;
}
