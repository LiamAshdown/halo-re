// decal_clear_flags  (Ghidra: FUN_0044e220; named per out/phase4/effects_types_notes.md, which
// refers to this address by this name: "decal_clear_flags 0x44e220 and decal_evict_object_decals
// 0x44e310 walk it")
// address 0x44e220, size 141 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 against objdump 0x44e220..0x44e2ac (BL clear_object_attached; iterator index starts at -1).)
// evidence: types/effects.h decal_flags (_decal_temporary_bit, _decal_object_attached_bit) and
// decal_grid.temporary_count/object_count; src/cache/sound_cache_dispose.c establishes the
// data_iterator full-table-scan idiom used here.
// register convention: a "also clear object-attached" flag in BL (unaff_BL).
//   // blam-cc: BL -> clear_object_attached
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *decal_data;       // 0x0087abe4
extern decal_grid *decal_grid_block; // 0x006b0ad8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator in EDI

// Clears the temporary flag (and its budget counter) on every live decal, and when
// `clear_object_attached` is set, also clears the object-attached flag and its counter.
void decal_clear_flags(uint8_t clear_object_attached)
{
    if (decal_data->valid) {
        data_iterator iterator;
        decal *self;

        iterator.data = decal_data;
        iterator.next_index = 0;
        iterator.index = k_datum_index_none; // 0x44e243
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
        self = (decal *)data_iterator_next(&iterator);

        while (self != 0) {
            if ((self->flags & _decal_temporary_bit) != 0) {
                self->flags = self->flags & ~_decal_temporary_bit;
                decal_grid_block->temporary_count = decal_grid_block->temporary_count - 1;
            }
            if (clear_object_attached != 0 && (self->flags & _decal_object_attached_bit) != 0) {
                self->flags = self->flags & ~_decal_object_attached_bit;
                decal_grid_block->object_count = decal_grid_block->object_count - 1;
            }
            self = (decal *)data_iterator_next(&iterator);
        }
    }
}

#if 0
Original Ghidra decompilation (0x44e220):

void FUN_0044e220(void)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  char unaff_BL;

  if (*(char *)(DAT_0087abe4 + 0x24) != '\0') {
    iVar3 = data_iterator_next();
    iVar2 = DAT_006b0ad8;
    while (iVar3 != 0) {
      if ((*(ushort *)(iVar3 + 2) & 1) != 0) {
        *(ushort *)(iVar3 + 2) = *(ushort *)(iVar3 + 2) & 0xfffe;
        piVar1 = (int *)(iVar2 + 0x2804);
        *piVar1 = *piVar1 + -1;
      }
      if ((unaff_BL != '\0') && ((*(ushort *)(iVar3 + 2) & 2) != 0)) {
        *(ushort *)(iVar3 + 2) = *(ushort *)(iVar3 + 2) & 0xfffd;
        piVar1 = (int *)(iVar2 + 0x2808);
        *piVar1 = *piVar1 + -1;
      }
      iVar3 = data_iterator_next();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
