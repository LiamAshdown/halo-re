// actor_iterator_new  (Ghidra: actor_iterator_new; named for this rewrite)
// address 0x436a30, size 62 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: initializes a types/ai.h actor_iterator_state exactly field-for-field (filter_
// array, cursor, signature, unknown_10, active, actor_index, unknown_18 all match the
// header's own established offsets), seeded to walk encounter_data. Its partner,
// actor_iterator_next (0x436a70, this batch), is already named and already documented by
// the header as reading this exact struct.
// register convention: Ghidra could not resolve the output pointer or the "active-only"
// flag at all.
//   // blam-cc: EAX -> out_iterator, stack -> active_only

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *encounter_data; // 0x008802c8
extern ai_globals *ai_globals_ptr; // 0x00880354

// blam-cc: EAX -> out_iterator, stack -> active_only
void actor_iterator_new(actor_iterator_state *out_iterator, uint8_t active_only)
{
    if (ai_globals_ptr->actors_valid != 0) {
        out_iterator->filter_array = encounter_data;
        out_iterator->next_index = 0;
        out_iterator->cursor = -1;
        out_iterator->signature = (uint32_t)encounter_data ^ 0x69746572;
        out_iterator->encounterless_done = 0;
        out_iterator->active = active_only;
        out_iterator->actor_index = (datum_index)k_datum_index_none;
        out_iterator->next_actor_index = -1;
    }
}

#if 0
Original Ghidra decompilation (0x436a30):

void FUN_00436a30(undefined1 param_1)

{
  uint uVar1;
  uint *in_EAX;

  uVar1 = DAT_008802c8;
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    *in_EAX = DAT_008802c8;
    in_EAX[2] = 0xffffffff;
    in_EAX[6] = 0xffffffff;
    in_EAX[5] = 0xffffffff;
    *(undefined2 *)(in_EAX + 1) = 0;
    in_EAX[3] = uVar1 ^ 0x69746572;
    *(undefined1 *)(in_EAX + 4) = 0;
    *(undefined1 *)((int)in_EAX + 0x11) = param_1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
