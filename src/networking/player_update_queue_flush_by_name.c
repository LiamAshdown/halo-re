// player_update_queue_flush_by_name  (Ghidra: FUN_004e5fe0; renamed, no prior name)
// address 0x4e5fe0, size 209 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Finds a player by name and resets/drains one
// of that player's per-object circular update queues, likely for debug or disconnect cleanup."
// types/game.h player_update_queue (queue.read_index at +0x130, queue.write_index at +0x12c,
// relative to player+0x120's embedded circular_queue).
// register convention: disassembly (objdump -d -M intel) confirms the ASCII name argument
// arrives the same way as player_update_history_log_set_name_filter.c's ESI argument (`mov
// esi,eax` at entry, then the string is walked through EAX while ESI is kept for the later
// wcscmp), i.e. EAX carries the name here (not ESI).
//   // blam-cc: EAX -> name
// UNSURE (load-bearing, preserved as-is): the `for` loop that walks read_index to write_index
// (mod 0x78) has an empty body in both Ghidra's decompile and the disassembly -- it computes
// nothing and writes nothing back. players_find_local_owned_unclear (foreign, well below this module's start) is
// then called unconditionally once a name match is found, regardless of whether the loop ran
// zero or more iterations. This looks like dead/vestigial code (perhaps the real per-iteration
// work got hoisted into players_find_local_owned_unclear itself, or was optimized away entirely), but per the task's
// "no invented behaviour" rule it is transcribed literally rather than removed or "fixed".
// UNSURE: by the time players_find_local_owned_unclear is called, ESI holds the numeric write_index value (the
// player pointer that was in ESI earlier has been overwritten by `mov esi,[esi+0x12c]`), which
// does not look like a meaningful argument; players_find_local_owned_unclear is declared here as taking no
// parameters this rewrite models.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#include <wchar.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *player_data; // 0x0087a480

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: iterator in EDI
extern void players_find_local_owned_unclear(void); // foreign (< this module), 0x477280

// Converts name to UTF-16, then finds the (first) player whose name matches it and busy-walks
// that player's update-history queue's read_index to its write_index before calling
// players_find_local_owned_unclear -- see the UNSURE notes above for why the walk itself has no visible effect.
void player_update_queue_flush_by_name(char *name) // blam-cc: EAX -> name
{
    uint16_t filter_name[0x400];
    int32_t length;
    int32_t i;
    data_iterator iter;
    player *candidate;
    int32_t index;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x800) {
        length = 0x3ff;
    }
    if ((uint32_t)(length * 2 + 2) <= 0x800) {
        filter_name[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            filter_name[i] = (uint16_t)(uint8_t)name[i];
        }
    }

    iter.data = player_data;
    iter.next_index = 0;
    iter.index = k_datum_index_none;
    iter.signature = (uint32_t)(uintptr_t)iter.data ^ k_data_iterator_signature;
    candidate = (player *)data_iterator_next(&iter);
    while (candidate != 0) {
        if (wcscmp((wchar_t *)candidate->name, (wchar_t *)filter_name) == 0) {
            for (index = candidate->update_history.queue.read_index;
                 index != candidate->update_history.queue.write_index;
                 index = (index + 1) % 0x78) {
                // UNSURE: empty in the original, see file header
            }
            players_find_local_owned_unclear();
        }
        candidate = (player *)data_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x4e5fe0), from tools/pack.py 0x4e5fe0:

void FUN_004e5fe0(void)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  int iVar3;
  int iVar4;
  wchar_t local_800;
  ushort auStack_7fe [1023];

  pcVar2 = in_EAX;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = (int)pcVar2 - (int)(in_EAX + 1);
  if (0x800 < iVar3 * 2 + 2U) {
    iVar3 = 0x3ff;
  }
  if (iVar3 * 2 + 2U < 0x801) {
    auStack_7fe[iVar3 + -1] = 0;
    iVar3 = iVar3 + -1;
    while (-1 < iVar3) {
      auStack_7fe[iVar3 + -1] = (ushort)(byte)in_EAX[iVar3];
      iVar3 = iVar3 + -1;
    }
  }
  iVar3 = data_iterator_next();
  while (iVar3 != 0) {
    iVar4 = _wcscmp((wchar_t *)(iVar3 + 4),&local_800);
    if (iVar4 == 0) {
      for (iVar4 = *(int *)(iVar3 + 0x130); iVar4 != *(int *)(iVar3 + 300);
          iVar4 = (iVar4 + 1) % 0x78) {
      }
      FUN_00477280();
    }
    iVar3 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
