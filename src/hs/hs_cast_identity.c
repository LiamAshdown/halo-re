// hs_cast_identity  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48ab90, size 15 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entries 351 and 400 (to short from long, long to long).
// objdump 0x48ab90..0x48ab9e: copies the low word over itself and returns the dword unchanged.
// blam-cc: stack -> value (cdecl)

#include "tags.h"

int32_t hs_cast_identity(int32_t value)
{
    return value;
}
