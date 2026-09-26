// hs_cast_long_to_real  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48ab30, size 13 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entry 302 (to real from long).
// objdump 0x48ab30..0x48ab3c: fild of the dword, fstp as a float, returned as its bits.
// blam-cc: stack -> value (cdecl)

#include "tags.h"

int32_t hs_cast_long_to_real(int32_t value)
{
    float result = (float)value;

    return *(int32_t *)&result;
}
