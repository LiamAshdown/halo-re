// video_refresh_rate_compare  (Ghidra: already named, __cdecl)
// address 0x4bab80, size 30 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// phase-4 review: checked against objdump 0x4bab80..0x4bab9d (unsigned compare).
// evidence: qsort comparator ordering ascending by a single uint32 refresh-rate value.
// register convention: __cdecl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
uint32_t __cdecl video_refresh_rate_compare(const uint32_t *a, const uint32_t *b)
{
    if (*a < *b) {
        return 0xffffffffu;
    }
    return (uint32_t)(*b < *a);
}

#if 0
Original Ghidra decompilation (0x4bab80):

uint __cdecl video_refresh_rate_compare(uint *a,uint *b)

{
  if (*a < *b) {
    return 0xffffffff;
  }
  return (uint)(*b < *a);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
