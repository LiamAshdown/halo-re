// hs_runtime_initialize  (Ghidra: hs_runtime_initialize, already named)
// address 0x489e70, size 115 bytes
// name confidence: 0.85   rewrite confidence: 0.6
// evidence: types/hs.h k_hs_thread_maximum_count/k_hs_global_maximum_count/k_hs_builtin_global_count;
//   src/memory/data_delete_all.c and src/memory/datum_new_at_index_with_salt.c for the two
//   established callee signatures.
// register convention: cc=__cdecl, no parameters.
// UNSURE: datum_new_at_index_with_salt is called 0x1eb times with zero visible arguments; the
// per-iteration index (ascending 0..0x1ea here) and salt (1 here) are inferred from types/hs.h's
// "hs_runtime_initialize reserves exactly 0x1eb runtime slots" note, not observed directly.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count
extern void data_delete_all(data_array *array); // blam-cc: ESI; memory module, 0x4d0580
extern datum_index datum_new_at_index_with_salt(datum_index requested_handle, data_array *array);
    // blam-cc: EAX -> requested_handle, EDX -> array; memory module, 0x4d03d0

extern data_array *hs_thread_data;  // 0x0087a470
extern data_array *hs_globals_data; // 0x0087a46c

// Allocates the hs_thread and hs_globals datum arrays. On success, marks hs_globals_data valid
// (data_array::valid at +0x24), resets it, and reserves its first k_hs_builtin_global_count
// (0x1eb) slots for the engine builtins, each with a distinct salt (see UNSURE above).
void hs_runtime_initialize(void)
{
    int32_t i;

    hs_thread_data = game_state_new((char *)"hs thread", k_hs_thread_maximum_count, 0x218 /* EBX at the original call */);
    hs_globals_data = game_state_new((char *)"hs globals", k_hs_global_maximum_count, 0x8 /* EBX at the original call */);
    if (hs_thread_data != 0 && hs_globals_data != 0) {
        hs_globals_data->valid = 1;
        data_delete_all(hs_globals_data);
        for (i = 0; i < k_hs_builtin_global_count; i++) {
            datum_new_at_index_with_salt((datum_index)i | 0x10000, hs_globals_data);
        }
    }
}

#if 0
Original Ghidra decompilation (0x489e70):

void __cdecl hs_runtime_initialize(void)

{
  int iVar1;

  DAT_0087a470 = game_state_new("hs thread",0x100);
  DAT_0087a46c = game_state_new("hs globals",0x400);
  if ((DAT_0087a470 != 0) && (DAT_0087a46c != 0)) {
    *(undefined1 *)(DAT_0087a46c + 0x24) = 1;
    data_delete_all();
    iVar1 = 0x1eb;
    do {
      datum_new_at_index_with_salt();
      iVar1 = iVar1 + -1;
    } while (iVar1 != 0);
  }
  return;
}
#endif
