// hs_dispose_dynamic_globals  (Ghidra: FUN_0048a130)
// address 0x48a130, size 110 bytes
// name confidence: 0.4 (out/phase4/hs_types_notes.md names this hs_dispose_dynamic_globals:
//   "deletes everything from 0x1eb upward")
// rewrite confidence: 0.55
// evidence: types/hs.h k_hs_builtin_global_count (0x1eb); src/memory/datum_delete.c's
//   established signature (array in EAX, handle in EDX) -- the decompiled `iVar2 =
//   datum_delete()` is EAX surviving the call unchanged (array untouched), not a real return
//   value, since datum_delete is void.
// register convention: none (void).
// UNSURE: the trailing `(-1 < sVar3 || (sVar1 == sVar3 >> 0xf))` term is tautologically true
// given the identical check earlier in the same && chain, so it is preserved verbatim but is
// dead weight, not a real second condition.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void datum_delete(data_array *array, datum_index handle); // blam-cc: EAX -> array,
    // EDX -> handle; memory module, 0x4d0510

extern data_array *hs_thread_data;  // 0x0087a470
extern data_array *hs_globals_data; // 0x0087a46c
extern uint8_t hs_runtime_active;   // 0x006b15e8

// Deletes every hs_globals_data slot from k_hs_builtin_global_count (0x1eb) up to
// hs_globals_data's current last_index (the scenario-defined globals), then marks the runtime
// inactive. hs_thread_data is marked invalid first (data_array::valid at +0x24).
void hs_dispose_dynamic_globals(void)
{
    hs_thread_data->valid = 0;

    if (k_hs_builtin_global_count < hs_globals_data->last_index) {
        int16_t slot;
        for (slot = k_hs_builtin_global_count; slot < hs_globals_data->last_index; slot++) {
            if (slot != k_datum_index_none && -1 < slot && slot < hs_globals_data->maximum_count) {
                hs_global *element = (hs_global *)((uint8_t *)hs_globals_data->data +
                    hs_globals_data->size * slot);
                if (element->identifier != 0 && (-1 < slot || element->identifier == (slot >> 0xf))) {
                    datum_delete(hs_globals_data, (datum_index)slot);
                }
            }
        }
    }

    hs_runtime_active = 0;
}

#if 0
Original Ghidra decompilation (0x48a130):

void FUN_0048a130(void)

{
  short sVar1;
  int iVar2;
  short sVar3;

  *(undefined1 *)(DAT_0087a470 + 0x24) = 0;
  sVar3 = 0x1eb;
  iVar2 = DAT_0087a46c;
  if (0x1eb < *(short *)(DAT_0087a46c + 0x2e)) {
    do {
      if ((((sVar3 != -1) && (-1 < sVar3)) && (sVar3 < *(short *)(iVar2 + 0x20))) &&
         ((sVar1 = *(short *)((int)*(short *)(iVar2 + 0x22) * (int)sVar3 + *(int *)(iVar2 + 0x34)),
          sVar1 != 0 && ((-1 < sVar3 || (sVar1 == sVar3 >> 0xf)))))) {
        iVar2 = datum_delete();
      }
      sVar3 = sVar3 + 1;
    } while (sVar3 < *(short *)(iVar2 + 0x2e));
  }
  DAT_006b15e8 = 0;
  return;
}
#endif
