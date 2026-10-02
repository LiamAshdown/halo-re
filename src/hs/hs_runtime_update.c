// hs_runtime_update  (Ghidra: hs_runtime_update, already named)
// address 0x48a1a0, size 166 bytes
// name confidence: 0.5 (out/phase4/hs_functions.md: "Per-tick HS scheduler: advances every
//   thread whose deadline has arrived and triggers a reload check when nothing else is pending")
// rewrite confidence: 0.6
// evidence: types/hs.h hs_thread (type 0x02, wake_tick 0x08); src/memory/data_iterator_next.c's
//   sibling datum_next (int16_t after_index in DX, data_array* in EDI), matching FUN_004d0630's
//   own established signature exactly; hs_syntax_node_garbage_collect (0x483310) and
//   object_lists_dispose_empty (0x48b340, this module).
// register convention: none (void).
// UNSURE: the `current_tick & 0x8000000f` pattern is a signed "tick % 16 == 0" test; preserved
// as the equivalent direct expression rather than the original's sign-handling bit trick, which
// is bit-for-bit equivalent for all int32_t inputs.
// reconciled: R32 hs_game_time_globals -> game.h game_time_globals (current_tick->game_time, budget_flag_1/2->active/paused, seconds_per_tick->leftover_time; same offsets)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630
extern void hs_thread_evaluate_step(datum_index thread_handle); // this module, 0x48a370
extern void object_lists_dispose_empty(void); // this module, 0x48b340
extern void hs_syntax_node_garbage_collect(void); // 0x483310, below this batch's range

extern uint8_t hs_runtime_active;  // 0x006b15e8
extern data_array *hs_thread_data; // 0x0087a470

// game_time_globals: defined in types/game.h (R32 replaced hs.h's partial game_time_globals)
extern game_time_globals *game_time; // 0x006f1d6c

// Runs one scheduler tick: steps every thread whose wake_tick has arrived (0 <= wake_tick <=
// current tick), noting whether any command thread (type 2) is live. Always disposes empty
// object lists afterward; if no command thread was seen, runs a syntax-node garbage collection
// pass every 16 ticks.
void hs_runtime_update(void)
{
    int32_t current_tick;
    datum_index thread_handle;
    hs_thread *thread;
    char command_thread_pending;

    if (hs_runtime_active == 0) {
        return;
    }

    current_tick = game_time->game_time;
    command_thread_pending = 0;
    thread_handle = datum_next(-1, hs_thread_data);
    while (thread_handle != k_datum_index_none) {
        thread = (hs_thread *)((uint8_t *)hs_thread_data->data + (thread_handle & 0xffff) * 0x218);
        if (thread->type == 2) {
            command_thread_pending = 1;
        }
        if (-1 < thread->wake_tick && thread->wake_tick <= current_tick) {
            hs_thread_evaluate_step(thread_handle);
        }
        thread_handle = datum_next((int16_t)thread_handle, hs_thread_data);
        if (hs_runtime_active == 0) {
            break;
        }
    }

    object_lists_dispose_empty();
    if (command_thread_pending == 0 && game_time->game_time % 16 == 0) {
        hs_syntax_node_garbage_collect();
    }
}

#if 0
Original Ghidra decompilation (0x48a1a0):

void hs_runtime_update(void)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  bool bVar5;

  iVar4 = DAT_0087a470;
  if (DAT_006b15e8 != '\0') {
    iVar1 = *(int *)(DAT_006f1d6c + 0xc);
    bVar5 = false;
    uVar2 = FUN_004d0630();
    do {
      if (uVar2 == 0xffffffff) break;
      iVar3 = (uVar2 & 0xffff) * 0x218;
      if (*(char *)(iVar3 + 2 + *(int *)(iVar4 + 0x34)) == '\x02') {
        bVar5 = true;
      }
      iVar3 = *(int *)(iVar3 + *(int *)(iVar4 + 0x34) + 8);
      if ((-1 < iVar3) && (iVar3 <= iVar1)) {
        FUN_0048a370(uVar2);
        iVar4 = DAT_0087a470;
      }
      uVar2 = FUN_004d0630();
    } while (DAT_006b15e8 != '\0');
    object_lists_dispose_empty();
    if (!bVar5) {
      uVar2 = *(uint *)(DAT_006f1d6c + 0xc) & 0x8000000f;
      bVar5 = uVar2 == 0;
      if ((int)uVar2 < 0) {
        bVar5 = (uVar2 - 1 | 0xfffffff0) == 0xffffffff;
      }
      if (bVar5) {
        hs_syntax_node_garbage_collect();
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
