// bitmap_group_sequence_get_bitmap_data  (Ghidra: bitmap_group_sequence_get_bitmap_data,
// already named)
// address 0x43f290, size 153 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/bitmaps_functions.md ("Resolves which bitmap_data entry corresponds to a
// given bitmap-group-sequence index and sub-index (handling looping/sprite sequences),
// returning a pointer to it."); out/phase4/bitmaps_types_notes.md register convention line for
// 0x43f290; types/tags.h Bitmap.bitmap_group_sequence (0x54 count / 0x58 pointer),
// BitmapGroupSequence (first_bitmap_index 0x20, bitmap_count 0x22, sprites 0x34/0x38),
// BitmapGroupSprite (bitmap_index 0x00).
// register convention: EAX = datum_index bitmap_tag_index, EDI (unaff_EDI, live-in) =
// int16_t frame_index, stack -> int16_t sequence_index.
//   // blam-cc: EAX -> bitmap_tag_index, EDI -> frame_index, stack -> sequence_index
// Note (verified against objdump 0x43f2e6..0x43f2f6): the sprite lookup indexes
// sprites[frame_index] with no bounds check against sprites.count; an out-of-range frame_index
// reads past the sprites block in the original too.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "bitmaps.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> bitmap_tag_index, EDI -> frame_index, stack -> sequence_index
BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag_index,
    int16_t frame_index, int16_t sequence_index)
{
    Bitmap *bitmap;
    int16_t bitmap_index;

    if (bitmap_tag_index == (datum_index)k_datum_index_none) {
        return 0;
    }
    bitmap = (Bitmap *)tag_instances[(uint16_t)bitmap_tag_index].data;
    if (bitmap == 0) {
        return 0;
    }

    // Falls back to frame_index directly whenever there is no sequence data to resolve
    // through, or the resolved index below comes back as k_datum_index_none.
    bitmap_index = frame_index;
    if (bitmap->bitmap_group_sequence.count > 0) {
        BitmapGroupSequence *sequence = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer +
            (int32_t)sequence_index % (int32_t)bitmap->bitmap_group_sequence.count;
        int16_t resolved;

        // bitmap_count is uint16_t in tags.h but the original tests and divides it as a signed
        // word (test ax,ax; jle / movsx ebp,ax; idiv ebp at 0x43f2d2..0x43f2de).
        if ((int16_t)sequence->bitmap_count > 0) {
            resolved = (int16_t)((int32_t)frame_index % (int32_t)(int16_t)sequence->bitmap_count +
                (int16_t)sequence->first_bitmap_index);
        } else if (sequence->sprites.count != 0) {
            resolved = ((BitmapGroupSprite *)sequence->sprites.pointer + frame_index)->bitmap_index;
        } else {
            resolved = frame_index;
        }

        if (resolved != (int16_t)k_datum_index_none) {
            bitmap_index = resolved;
        }
    }

    if (bitmap_index >= 0 && bitmap_index < (int32_t)bitmap->bitmap_data.count) {
        return (BitmapData *)bitmap->bitmap_data.pointer + bitmap_index;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x43f290):

int bitmap_group_sequence_get_bitmap_data(short param_1)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  short sVar3;
  short unaff_DI;

  if (in_EAX == 0xffffffff) {
    return 0;
  }
  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (iVar1 == 0) {
    return 0;
  }
  if (0 < *(int *)(iVar1 + 0x54)) {
    iVar2 = ((int)param_1 % *(int *)(iVar1 + 0x54)) * 0x40;
    sVar3 = *(short *)(iVar2 + 0x22 + *(int *)(iVar1 + 0x58));
    iVar2 = iVar2 + *(int *)(iVar1 + 0x58);
    if (sVar3 < 1) {
      if (*(int *)(iVar2 + 0x34) == 0) goto LAB_0043f300;
      sVar3 = *(short *)(unaff_DI * 0x20 + *(int *)(iVar2 + 0x38));
    }
    else {
      sVar3 = unaff_DI % sVar3 + *(short *)(iVar2 + 0x20);
    }
    if (sVar3 != -1) goto LAB_0043f302;
  }
LAB_0043f300:
  sVar3 = unaff_DI;
LAB_0043f302:
  if ((-1 < sVar3) && ((int)sVar3 < *(int *)(iVar1 + 0x60))) {
    return sVar3 * 0x30 + *(int *)(iVar1 + 100);
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
