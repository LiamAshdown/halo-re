// recorded_animation_find_by_object  (Ghidra: recorded_animation_find_by_object, already named)
// address 0x44ad20, size 93 bytes
// name confidence: 0.6   rewrite confidence: 0.7
// evidence: types/cutscene.h recorded_animation struct ("searched by 0x44acc0 / 0x44ad20") and
//   out/phase4/devices_types_notes.md "Suggested names: ... recorded_animation_find_by_object
//   (0x44ad20)". Its only caller, recorded_animation_start (0x44a930, this batch), matches
//   `iVar4 = recorded_animation_find_by_object(local_4); ... if (iVar4 != 0) { *(iVar4+4) =
//   unit_index; ... }`, treating the return as the found record's pointer -- Ghidra's own
//   decompile of THIS function mis-declares it `void`, dropping the EAX return entirely (a
//   decompiler error, not a real void function); the disassembly below settles it.
// register convention: `objdump -d -M intel --start-address=0x44ad20 --stop-address=0x44ad80
//   bin/halo.exe`: EBX = unit_index to search for (unaff_EBX); one plain stack argument
//   (out_index, an optional datum_index * output). The function builds its OWN local
//   data_iterator over the recorded_animations data_array (matching types/memory.h
//   data_iterator field-for-field: data 0x00, next_index 0x04, index 0x08, signature 0x0c) --
//   it does not receive one from the caller. Returns the found element pointer in EAX (NULL if
//   none matched); *out_index (when non-NULL) receives the found element's own datum_index, or
//   k_datum_index_none if the search failed.
//   // blam-cc: EBX -> unit_index, stack -> out_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "units.h"
#include "cutscene.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *recorded_animations; // 0x006b0a10
extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: EBX -> unit_index, stack -> out_index
// Searches the recorded_animations data_array for the (at most one) record playing back on
// unit_index, returning it (or NULL) and, through the optional out_index, its own datum handle
// (or k_datum_index_none if not found).
recorded_animation *recorded_animation_find_by_object(datum_index unit_index, datum_index *out_index)
{
    data_iterator iterator;
    recorded_animation *entry;
    datum_index found_index = (datum_index)k_datum_index_none;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (recorded_animation *)data_iterator_next(&iterator);
    while ((entry != (recorded_animation *)0) && (entry->unit_index != unit_index)) {
        entry = (recorded_animation *)data_iterator_next(&iterator);
    }
    if (entry != (recorded_animation *)0) {
        found_index = iterator.index;
    }
    if (out_index != (datum_index *)0) {
        *out_index = found_index;
    }
    return entry;
}

#if 0
Original Ghidra decompilation (0x44ad20):

void recorded_animation_find_by_object(undefined4 *param_1)

{
  int iVar1;
  int unaff_EBX;

  iVar1 = data_iterator_next();
  while ((iVar1 != 0 && (*(int *)(iVar1 + 4) != unaff_EBX))) {
    iVar1 = data_iterator_next();
  }
  if (param_1 != (undefined4 *)0x0) {
    *param_1 = 0xffffffff;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
