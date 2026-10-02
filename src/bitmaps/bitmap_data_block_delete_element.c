// bitmap_data_block_delete_element  (not a Ghidra function; the element delete proc of the "bitmap_data_block" tag
//   block definition at 0x00686048 (maximum 0x800 elements of 0x30 bytes), slot 0x0068606c; no C existed, so that
//   stored pointer trapped as unlisted_43f010)
// address 0x43f010, size 27 bytes
// name confidence: 0.7   rewrite confidence: 0.95
// WRITTEN 2026-09-28 from objdump 0x43f010..0x43f02a: frees element `index` of the block (block +0x04 is the
//   element array, 0x30 bytes each) with bitmap_data_free (0x43f880, ESI).
// blam-cc: stack -> block, index (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void bitmap_data_free(BitmapData *bitmap_data); // 0x43f880, blam-cc: ESI

void bitmap_data_block_delete_element(TagReflexive *block, int32_t index)
{
    bitmap_data_free((BitmapData *)((uint8_t *)block->pointer + index * 0x30));
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
