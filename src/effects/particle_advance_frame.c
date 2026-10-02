// particle_advance_frame  (Ghidra: FUN_00456000, still unnamed there; named directly by
//   types/effects.h: "particle_advance_frame 0x456000 (frame_index)")
// address 0x456000, size 184 bytes
// name confidence: 0.6   rewrite confidence: 0.9 (VERIFIED against objdump 0x456000..0x4560b7)
// evidence: types/effects.h particle.animation_timer (+0x1c), frame_index (+0x26); types/tags.h
//   Particle.bitmap (TagDependency), Bitmap.bitmap_group_sequence (TagReflexive of
//   BitmapGroupSequence), BitmapGroupSequence.sprites (TagReflexive, its count is the per
//   sequence frame count read at +0x34 in the raw disassembly, distinct from
//   BitmapGroupSequence.bitmap_count at +0x20).
// register convention: particle handle in EAX (in_EAX).
//   // blam-cc: EAX -> particle_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *particle_data;   // 0x0087abd0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t particle_next_sequence(datum_index particle_handle); // 0x455e60, this module

// Steps a particle's frame index by one in the direction its flags request, resetting the per
// frame animation timer either way. When the walk runs off the end (or start) of the current
// sequence's sprite list it rolls the next sequence instead.
uint8_t particle_advance_frame(datum_index particle_handle)
{
    particle *self = &((particle *)particle_data->data)[(uint16_t)particle_handle];
    Particle *tag = (Particle *)tag_instances[(uint16_t)self->definition_index].data;
    Bitmap *bitmap = (Bitmap *)tag_instances[tag->bitmap.tag_id.index].data;
    BitmapGroupSequence *sequences = (BitmapGroupSequence *)bitmap->bitmap_group_sequence.pointer;

    self->animation_timer = 0.0f;

    if ((self->flags & _particle_animating_backwards_bit) == 0) {
        int32_t sprite_count = sequences[self->sequence_index].sprites.count;

        if (self->frame_index + 1 < sprite_count) {
            self->frame_index += 1;
            return 1;
        }
        {
            uint8_t has_frame = particle_next_sequence(particle_handle);
            self->frame_index = 0;
            return has_frame;
        }
    } else {
        if (self->frame_index > 0) {
            self->frame_index -= 1;
            return 1;
        }
        {
            uint8_t has_frame = particle_next_sequence(particle_handle);
            if (has_frame != 0) {
                self->frame_index = (int16_t)(sequences[self->sequence_index].sprites.count - 1);
                return has_frame;
            }
            return 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x456000):

char FUN_00456000(void)

{
  int iVar1;
  char cVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;

  iVar4 = (in_EAX & 0xffff) * 0x70 + *(int *)(DAT_0087abd0 + 0x34);
  iVar1 = *(int *)((*(uint *)(*(int *)((*(uint *)(iVar4 + 4) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)
                             + 0x10) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  *(undefined4 *)(iVar4 + 0x1c) = 0;
  if ((*(byte *)(iVar4 + 2) & 1) == 0) {
    if (*(short *)(iVar4 + 0x26) + 1 <
        *(int *)(*(short *)(iVar4 + 0x24) * 0x40 + 0x34 + *(int *)(iVar1 + 0x58))) {
      *(short *)(iVar4 + 0x26) = *(short *)(iVar4 + 0x26) + 1;
      return '\x01';
    }
    cVar3 = FUN_00455e60();
    *(undefined2 *)(iVar4 + 0x26) = 0;
  }
  else {
    if (0 < *(short *)(iVar4 + 0x26)) {
      *(short *)(iVar4 + 0x26) = *(short *)(iVar4 + 0x26) + -1;
      return '\x01';
    }
    cVar2 = FUN_00455e60();
    cVar3 = '\0';
    if (cVar2 != '\0') {
      *(short *)(iVar4 + 0x26) =
           *(short *)(*(short *)(iVar4 + 0x24) * 0x40 + 0x34 + *(int *)(iVar1 + 0x58)) + -1;
      return cVar2;
    }
  }
  return cVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
