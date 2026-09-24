// decals_update_fade  (Ghidra: chimera__decal_table; the "chimera__" prefix is a Chimera
// signature match artifact, not part of the retail symbol -- out/phase4/effects_types_notes.md:
// "chimera__decal_table 0x44e2b0 ... [is a] Chimera signature name for ... decals_update_fade")
// address 0x44e2b0, size 85 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: types/memory.h data_iterator.index (the handle of the element last returned); this
// module's own decal_update_fade 0x44dc30 takes a decal handle, not a pointer.
// register convention: __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"

extern data_array *decal_data; // 0x0087abe4

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, memory module; iterator in EDI
extern void decal_update_fade(datum_index decal_index); // 0x44dc30, this module; blam-cc: EAX -> decal_index

// Per-tick driver: recomputes the fade alpha of every live decal.
void decals_update_fade(void)
{
    if (decal_data->valid) {
        data_iterator iterator;

        iterator.data = decal_data;
        iterator.next_index = 0;
        iterator.index = 0;

        while (data_iterator_next(&iterator) != 0) {
            decal_update_fade(iterator.index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x44e2b0):

void __cdecl chimera__decal_table(void)

{
  int iVar1;

  if (*(char *)(DAT_0087abe4 + 0x24) != '\0') {
    iVar1 = data_iterator_next();
    while (iVar1 != 0) {
      decal_update_fade();
      iVar1 = data_iterator_next();
    }
  }
  return;
}
#endif
