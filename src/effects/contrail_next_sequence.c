// contrail_next_sequence  (Ghidra: FUN_0044ced0; named per out/phase2/results/effects_00.json
// "contrail_pick_random_marker_index", renamed here -- the function does not touch a marker
// index at all, it walks the Contrail's Bitmap sequence/frame pair, matching types/effects.h's
// own description: "contrail_next_sequence 0x44ced0, against Contrail.first_sequence_index +0x40,
// sequence_count +0x42 and BitmapSequence.bitmap_count +0x22")
// address 0x44ced0, size 167 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/tags.h Contrail (bitmap TagDependency, first_sequence_index, sequence_count),
// Bitmap (bitmap_group_sequence TagReflexive at 0x54), BitmapGroupSequence (bitmap_count at
// 0x22, stride 0x40); types/effects.h contrail.sequence_index / frame_index.
// register convention: contrail pointer in EAX (in_EAX), no other arguments.
//   // blam-cc: EAX -> self

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

extern tag_instance *tag_instances;    // 0x0087bc14
extern random_seed effect_random_seed; // 0x00719cd4

// Advances to the next frame of the current bitmap sequence, and once the sequence or frame runs
// past the end of the Bitmap's sequence table, rerolls a new random sequence within
// Contrail.first_sequence_index/sequence_count and resets frame_index to 0.
void contrail_next_sequence(contrail *self)
{
    Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmap.tag_id.index].data;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;
    int16_t sequence_index;

    self->frame_index = self->frame_index + 1;
    sequence_index = self->sequence_index;
    self->animation_timer = 0.0f;

    if (sequence_index >= 0 && (uint32_t)(int32_t)sequence_index < bitmap->bitmap_group_sequence.count &&
        self->frame_index >= 0 &&
        self->frame_index < (int16_t)sequences[sequence_index].bitmap_count) {
        return;
    }

    {
        int16_t first = tag->first_sequence_index;

        effect_random_seed = effect_random_seed * k_random_multiplier + k_random_increment;
        self->sequence_index = (int16_t)(((int16_t)(tag->sequence_count + first) - first) *
            (int32_t)(effect_random_seed >> k_random_value_shift) >> 16) + first;
        self->frame_index = 0;
    }
}

#if 0
Original Ghidra decompilation (0x44ced0):

void FUN_0044ced0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int in_EAX;
  
  iVar2 = *(int *)((*(uint *)(in_EAX + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = *(int *)((*(uint *)(iVar2 + 0x3c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(short *)(in_EAX + 0x16) = *(short *)(in_EAX + 0x16) + 1;
  sVar1 = *(short *)(in_EAX + 0x14);
  *(undefined4 *)(in_EAX + 0x24) = 0;
  if ((((-1 < sVar1) && ((int)sVar1 < *(int *)(iVar3 + 0x54))) && (-1 < *(short *)(in_EAX + 0x16)))
     && (*(short *)(in_EAX + 0x16) < *(short *)(sVar1 * 0x40 + 0x22 + *(int *)(iVar3 + 0x58)))) {
    return;
  }
  sVar1 = *(short *)(iVar2 + 0x40);
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  *(short *)(in_EAX + 0x14) =
       (short)(((int)(short)(*(short *)(iVar2 + 0x42) + sVar1) - (int)sVar1) *
               (DAT_00719cd4 >> 0x10) >> 0x10) + sVar1;
  *(undefined2 *)(in_EAX + 0x16) = 0;
  return;
}
#endif
