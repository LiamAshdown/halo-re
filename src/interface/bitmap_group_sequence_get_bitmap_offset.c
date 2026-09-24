// bitmap_group_sequence_get_bitmap_offset  (Ghidra: bitmap_group_sequence_get_bitmap_offset,
// already named)
// address 0x4ab630, size 83 bytes
// name confidence: 0.5 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.5
// evidence: types/tags.h Bitmap::bitmap_group_sequence (TagReflexive at 0x54, matching the
// `+0x54`/`+0x58` count/pointer reads) and BitmapGroupSequence::sprites (TagReflexive at 0x34,
// matching the `+0x34`/`+0x38` reads); types/cache.h tag_instance for the tag lookup idiom.
// UNSURE: the returned address (`(frame_index % sprite_count) * 0x20 + 8 + sprites.pointer`)
// reaches 8 bytes into each 0x20 byte BitmapGroupSprite element; that struct is not declared in
// types/tags.h, so the offset is kept raw.
// register convention: bitmap tag in ECX (in_ECX), sequence_index in AX (in_AX), frame_index in
// DI (unaff_DI), all unresolved register reads.
//   // blam-cc: ECX -> bitmap_tag, AX -> sequence_index, DI -> frame_index
// FIXED (register inputs, objdump): DI (read at 0x4ab641, `cmp di,0xffff`) carries frame_index;
//   it was already a C parameter but the "blam-cc" note phrased it as "name -> REG" instead of
//   "REG -> name", so the checker's parser missed it.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: ECX -> bitmap_tag, AX -> sequence_index, DI -> frame_index
// Resolves animation frame frame_index of bitmap_tag's sequence_index'th BitmapGroupSequence to
// a byte offset 8 bytes into the matching BitmapGroupSprite element, or 0 if any handle is
// invalid or the sequence has no sprites.
int32_t bitmap_group_sequence_get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index, int16_t frame_index)
{
    if (bitmap_tag == (datum_index)-1 || sequence_index == -1 || frame_index == -1) {
        return 0;
    }

    {
        Bitmap *tag_data = (Bitmap *)tag_instances[bitmap_tag & 0xffff].data;
        if (sequence_index < (int32_t)tag_data->bitmap_group_sequence.count) {
            BitmapGroupSequence *sequence =
                (BitmapGroupSequence *)tag_data->bitmap_group_sequence.pointer + sequence_index;
            int32_t sprite_count = (int32_t)sequence->sprites.count;
            if (sprite_count != 0) {
                return (frame_index % sprite_count) * 0x20 + 8 + (int32_t)sequence->sprites.pointer;
            }
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x4ab630):

int bitmap_group_sequence_get_bitmap_offset(void)

{
  int iVar1;
  short in_AX;
  int iVar2;
  uint in_ECX;
  int iVar3;
  short unaff_DI;

  iVar2 = 0;
  if (((in_ECX != 0xffffffff) && (in_AX != -1)) && (unaff_DI != -1)) {
    iVar1 = *(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((int)in_AX < *(int *)(iVar1 + 0x54)) {
      iVar3 = in_AX * 0x40 + *(int *)(iVar1 + 0x58);
      iVar1 = *(int *)(iVar3 + 0x34);
      if (iVar1 != 0) {
        iVar2 = ((int)unaff_DI % iVar1) * 0x20 + 8 + *(int *)(iVar3 + 0x38);
      }
    }
  }
  return iVar2;
}
#endif
