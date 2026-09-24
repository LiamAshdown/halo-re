// decal_update_fade  (Ghidra: decal_update_fade, already named)
// address 0x44dc30, size 244 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: types/effects.h decal (flags, lifetime 0x1c, decay_time 0x20, alpha 0x28) and
// decal_grid.temporary_count (0x2804); src/memory/cache_evict_entry.c establishes
// cache_evict_entry's (handle EBX, cache* EDI) convention; the game tick at 0x006f1d6c+0x0c and
// the 1/30 literal match out/phase4/effects_types_notes.md's "the fade is
// (now - this) * 1/30" note for decal.creation_game_time.
// register convention: decal handle in EAX (in_EAX).
//   // blam-cc: EAX -> decal_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *decal_data;      // 0x0087abe4
extern decal_grid *decal_grid_block; // 0x006b0ad8
extern cache *decal_geometry_cache; // 0x0071d1c0
extern int32_t *game_time;  // 0x006f1d6c; +0x0c is the current game tick

extern void cache_evict_entry(datum_index handle, cache *self); // 0x4d1c20,
    // blam-cc: EBX -> handle, EDI -> self

// Recomputes one decal's alpha from its age. A cluster decal (not object-attached) that has
// outlived its lifetime is evicted from the geometry cache; one inside its final decay_time
// window fades linearly to 0.
void decal_update_fade(datum_index decal_index)
{
    decal *self = &((decal *)decal_data->data)[(uint16_t)decal_index];
    real age = (real)(game_time[3] - self->creation_game_time) * 0.033333335f;

    self->alpha = 0xff;

    if ((self->flags & _decal_object_attached_bit) == 0) {
        if (self->lifetime != 0.0f && self->lifetime <= age) {
            if ((self->flags & _decal_temporary_bit) != 0) {
                self->flags = self->flags & ~_decal_temporary_bit;
                decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
            }
            cache_evict_entry(decal_index, decal_geometry_cache);
            return;
        }

        if (self->lifetime > 0.0f && self->decay_time > 0.0f) {
            real remaining = self->lifetime - age;

            if (remaining < self->decay_time) {
                self->alpha = (uint8_t)(int)((remaining / self->decay_time) * 255.0f + 0.5f);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x44dc30):

void decal_update_fade(void)

{
  ushort uVar1;
  int iVar2;
  int iVar3;
  float fVar4;
  int iVar5;
  uint in_EAX;
  int iVar6;
  undefined1 local_4;

  iVar6 = (in_EAX & 0xffff) * 0x38;
  iVar2 = *(int *)(DAT_006f1d6c + 0xc);
  iVar3 = *(int *)(iVar6 + 0x14 + *(int *)(DAT_0087abe4 + 0x34));
  iVar6 = iVar6 + *(int *)(DAT_0087abe4 + 0x34);
  uVar1 = *(ushort *)(iVar6 + 2);
  *(undefined1 *)(iVar6 + 0x28) = 0xff;
  iVar5 = DAT_006b0ad8;
  fVar4 = (float)(iVar2 - iVar3) * 0.033333335;
  if ((uVar1 & 2) == 0) {
    if ((*(float *)(iVar6 + 0x1c) != 0.0) && (*(float *)(iVar6 + 0x1c) <= fVar4)) {
      if ((uVar1 & 1) != 0) {
        *(ushort *)(iVar6 + 2) = uVar1 & 0xfffe;
        *(int *)(iVar5 + 0x2804) = *(int *)(iVar5 + 0x2804) + -1;
      }
      cache_evict_entry();
      return;
    }
    if (((0.0 < *(float *)(iVar6 + 0x1c)) && (0.0 < *(float *)(iVar6 + 0x20))) &&
       (fVar4 = *(float *)(iVar6 + 0x1c) - fVar4, fVar4 < *(float *)(iVar6 + 0x20))) {
      local_4 = (undefined1)(int)ROUND((fVar4 / *(float *)(iVar6 + 0x20)) * 255.0);
      *(undefined1 *)(iVar6 + 0x28) = local_4;
      return;
    }
  }
  return;
}
#endif
