// gamespy_array_length  (Ghidra: FUN_006175f0; statically linked GameSpy SDK darray.c ArrayLength)
// address 0x6175f0, size 7 bytes
// name confidence: 0.85  rewrite confidence: 0.95
// evidence: objdump 0x6175f0..0x6175f6: returns the first dword of the array (DArrayImplementation.count). Other
//   GameSpy code calls it on its own records for the same first-dword read.
// blam-cc: stack -> array (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"

int32_t gamespy_array_length(void *array)
{
    return *(int32_t *)array;
}
