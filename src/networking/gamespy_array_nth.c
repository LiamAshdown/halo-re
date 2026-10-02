// gamespy_array_nth  (Ghidra: FUN_0061dc00; statically linked GameSpy SDK darray.c ArrayNth)
// address 0x61dc00, size 16 bytes
// name confidence: 0.85  rewrite confidence: 0.95
// evidence: objdump 0x61dc00..0x61dc0f: list (+0x14) + index * element size (+0x08), with no bounds check.
// blam-cc: stack -> array, index (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
void *gamespy_array_nth(void *array, int32_t index)
{
    return *(uint8_t **)((uint8_t *)array + 0x14) + index * *(int32_t *)((uint8_t *)array + 0x08);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
