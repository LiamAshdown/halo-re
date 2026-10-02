// hs_cast_short_to_boolean  (not a Ghidra function; an hs type cast reached only through the cast table)
// address 0x48aad0, size 18 bytes
// name confidence: 0.6  rewrite confidence: 0.95
// evidence: hs_thread_return / hs_coerce_value call dword [ecx*4+0x68bc10] with the value pushed; this is
//   table 0x68bc10 entry 252 (to boolean from short).
// objdump 0x48aad0..0x48aae1: cmp WORD [esp+4],0 / sete al into the argument's low byte; the upper bytes are
//   returned unchanged.
// blam-cc: stack -> value (cdecl)

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t hs_cast_short_to_boolean(int32_t value)
{
    return (int32_t)(((uint32_t)value & 0xffffff00u) | ((int16_t)value == 0));
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
