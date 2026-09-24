// bitmap_group_get_bitmap_data  (Ghidra: bitmap_group_get_bitmap_data, already named)
// address 0x43f250, size 53 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Given a bitmap tag id (EAX) and a bitmap_data
// index (DX), returns a pointer to that bitmap_data element, or 0 if the index/tag is
// invalid."); types/cache.h tag_instance.data; types/tags.h Bitmap.bitmap_data (TagReflexive,
// 0x60 count / 0x64 pointer) and BitmapData (size 0x30).
// register convention: EAX = datum_index bitmap_tag_index, DX = int16_t bitmap_data_index.
//   // blam-cc: EAX -> bitmap_tag_index, EDX (low half, DX) -> bitmap_data_index

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "bitmaps.h"

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> bitmap_tag_index, EDX (low half, DX) -> bitmap_data_index
BitmapData *bitmap_group_get_bitmap_data(datum_index bitmap_tag_index, int16_t bitmap_data_index)
{
    Bitmap *bitmap = (Bitmap *)tag_instances[(uint16_t)bitmap_tag_index].data;

    if (bitmap != 0 && bitmap_data_index >= 0) {
        if (bitmap_data_index < (int32_t)bitmap->bitmap_data.count) {
            return (BitmapData *)bitmap->bitmap_data.pointer + bitmap_data_index;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43f250):

int bitmap_group_get_bitmap_data(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  short in_DX;

  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar2 = 0;
  if ((iVar1 != 0) && (-1 < in_DX)) {
    if ((int)in_DX < *(int *)(iVar1 + 0x60)) {
      iVar2 = in_DX * 0x30 + *(int *)(iVar1 + 100);
    }
  }
  return iVar2;
}
#endif
