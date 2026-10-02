// hud_meter_resolve_bitmap_frame  (Ghidra: FUN_004ab8d0, renamed)
// address 0x4ab8d0, size 166 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4ab8d0..0x4ab975 in the phase-4 review. The first
// rewrite took the frame from the wrong place (it is EAX, the stack holds tag, sequence and
// the two outputs), cleared *out_data first (the binary never does; callers preset it) and
// called 0x43f290 and 0x4ab630 without their register arguments.
//   For a valid tag and a sequence inside Bitmap::bitmap_group_sequence, the frame (masked
// to 15 bits) selects sprite frame % sprite_count and *out_data becomes the BitmapData of
// its bitmap_index; a sequence without sprites uses bitmap_group_sequence_get_bitmap_data
// (EAX tag, DI frame, stack sequence). An invalid tag or sequence leaves *out_data as the
// caller set it. Then *out_offset gets the sprite bounds (&sprite + 8,
// bitmap_group_sequence_get_bitmap_offset: ECX tag, AX sequence, DI frame) while *out_data is
// not NULL, else NULL. The frame passed on is the masked one only on the sprite path.
// register convention: EAX frame; stack bitmap_tag, sequence_index, out_data, out_offset (the
// C parameter order below follows the callers; frame_index is the EAX argument).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances; // 0x0087bc14

extern BitmapData *bitmap_group_sequence_get_bitmap_data(datum_index bitmap_tag, int16_t frame, int16_t sequence); // 0x43f290, blam-cc: EAX tag, DI frame
extern int32_t bitmap_group_sequence_get_bitmap_offset(datum_index bitmap_tag, int16_t sequence_index,
                                                         int16_t frame_index); // 0x4ab630, blam-cc: ECX bitmap_tag, AX sequence_index, DI frame_index

// blam-cc: EAX -> frame_index, stack -> bitmap_tag, sequence_index, out_data, out_offset
void hud_meter_resolve_bitmap_frame(datum_index bitmap_tag, int16_t sequence_index, uint16_t frame_index,
                                    void **out_data, int32_t *out_offset)
{
    int32_t frame = frame_index; // the whole EAX; only the low 15 bits survive the mask

    if (bitmap_tag != (datum_index)-1) {
        Bitmap *bitmap = (Bitmap *)tag_instances[bitmap_tag & 0xffff].data;
        int16_t sequence = (int16_t)sequence_index;

        if (sequence < (int32_t)bitmap->bitmap_group_sequence.count) {
            BitmapGroupSequence *group = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer + sequence;
            int32_t sprite_count = (int32_t)group->sprites.count;

            frame &= 0x7fff;
            if (sprite_count != 0) {
                BitmapGroupSprite *sprite = (BitmapGroupSprite *)group->sprites.pointer + (int16_t)frame % sprite_count;

                *out_data = (BitmapData *)bitmap->bitmap_data.pointer + (int16_t)sprite->bitmap_index;
            } else {
                *out_data = bitmap_group_sequence_get_bitmap_data(bitmap_tag, (int16_t)frame, sequence);
            }
        }
    }
    if (*out_data != 0) {
        *out_offset = bitmap_group_sequence_get_bitmap_offset(bitmap_tag, (int16_t)sequence_index, (int16_t)frame);
    } else {
        *out_offset = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4ab8d0):

void FUN_004ab8d0(uint param_1,undefined4 param_2,int *param_3,undefined4 *param_4)

{
  int iVar1;
  ushort in_AX;
  int iVar2;
  int iVar3;
  undefined4 uVar4;

  if (param_1 != 0xffffffff) {
    iVar3 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if ((int)(short)param_2 < *(int *)(iVar3 + 0x54)) {
      iVar2 = (short)param_2 * 0x40 + *(int *)(iVar3 + 0x58);
      iVar1 = *(int *)(iVar2 + 0x34);
      if (iVar1 == 0) {
        iVar3 = bitmap_group_sequence_get_bitmap_data(param_2);
        *param_3 = iVar3;
      }
      else {
        *param_3 = *(short *)((int)((longlong)(ulonglong)(uint)(int)(short)(in_AX & 0x7fff) %
                                   (longlong)iVar1) * 0x20 + *(int *)(iVar2 + 0x38)) * 0x30 +
                   *(int *)(iVar3 + 100);
      }
    }
  }
  if (*param_3 == 0) {
    *param_4 = 0;
    return;
  }
  uVar4 = bitmap_group_sequence_get_bitmap_offset();
  *param_4 = uVar4;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
