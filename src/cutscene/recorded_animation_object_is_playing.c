// recorded_animation_object_is_playing  (Ghidra: recorded_animation_object_is_playing, already named)
// address 0x44acc0, size 91 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/cutscene.h recorded_animation struct ("searched by 0x44acc0 / 0x44ad20") and
//   out/phase4/devices_types_notes.md "Suggested names: ... recorded_animation_object_is_playing
//   (0x44acc0)"; types/cutscene.h recorded_animation_flags._recorded_animation_flag_finished
//   comment "0x44acc0 skips finished ones". Confirmed by
//   `objdump -d -M intel --start-address=0x44acc0 --stop-address=0x44ad20 bin/halo.exe`: no
//   stack arguments, a fresh local data_iterator over recorded_animations (same field layout as
//   its sibling recorded_animation_find_by_object).
// register convention: ESI = unit_index (unaff_ESI), never modified by this function. No stack
//   arguments.
//   // blam-cc: ESI -> unit_index

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

// blam-cc: ESI -> unit_index
// Returns true if unit_index has a not-yet-finished recorded_animation record currently
// playing back on it.
uint8_t recorded_animation_object_is_playing(datum_index unit_index)
{
    data_iterator iterator;
    recorded_animation *entry;

    iterator.data = recorded_animations;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    entry = (recorded_animation *)data_iterator_next(&iterator);
    for (;;) {
        if (entry == (recorded_animation *)0) {
            return 0;
        }
        if ((entry->unit_index == unit_index) &&
            ((entry->flags & _recorded_animation_flag_finished) == 0)) {
            break;
        }
        entry = (recorded_animation *)data_iterator_next(&iterator);
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x44acc0):

undefined4 recorded_animation_object_is_playing(void)

{
  int iVar1;
  int unaff_ESI;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 0;
    }
    if ((*(int *)(iVar1 + 4) == unaff_ESI) && ((*(byte *)(iVar1 + 10) & 1) == 0)) break;
    iVar1 = data_iterator_next();
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
