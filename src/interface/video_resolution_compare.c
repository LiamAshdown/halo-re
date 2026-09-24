// video_resolution_compare  (Ghidra: already named, __cdecl)
// address 0x4bab50, size 48 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// phase-4 review: checked against objdump 0x4bab50..0x4bab7f (unsigned width then height compare).
// evidence: types/interface.h video_resolution (width at +0x00, height at +0x04); qsort
// comparator ordering ascending by width then height.
// register convention: __cdecl.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

int __cdecl video_resolution_compare(const video_resolution *a, const video_resolution *b)
{
    uint32_t aw = (uint32_t)a->width, bw = (uint32_t)b->width;
    uint32_t ah = (uint32_t)a->height, bh = (uint32_t)b->height;

    if (aw < bw) {
        return -1;
    }
    if (aw <= bw) {
        if (ah < bh) {
            return -1;
        }
        if (ah <= bh) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4bab50):

int __cdecl video_resolution_compare(uint *a,uint *b)

{
  if (*a < *b) {
    return -1;
  }
  if (*a <= *b) {
    if (a[1] < b[1]) {
      return -1;
    }
    if (a[1] <= b[1]) {
      return 0;
    }
  }
  return 1;
}
#endif
