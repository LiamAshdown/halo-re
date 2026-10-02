// actor_iterator_next  (Ghidra: actor_iterator_next, already named)
// address 0x436a70, size 154 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: reads/writes exactly the types/ai.h actor_iterator_state fields
// (unknown_18/unknown_10/active/actor_index), and its first 0x0c bytes double as a
// types/memory.h data_iterator over encounter_data (actor_iterator_new, 0x436a30, this
// batch, seeds filter_array/cursor/signature identically to a fresh data_iterator).
// Advances through the live encounters' member lists (encounter.units_active/first_actor,
// gated by the iterator's active-only flag) and, once they are exhausted, falls back to the
// global unassigned-actor list (ai_globals.unknown_08), returning actors one at a time
// (skipping inactive ones when active-only is set).
// register convention: Ghidra could not resolve the iterator pointer at all.
//   // blam-cc: EAX -> iterator

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: EAX -> iterator
actor *actor_iterator_next(actor_iterator_state *iterator)
{
    datum_index next;
    actor *a;

    if (ai_globals_ptr->actors_valid == 0) {
        return 0;
    }

    next = iterator->next_actor_index;
    while (next == (datum_index)k_datum_index_none) {
        encounter *enc = (encounter *)data_iterator_next((data_iterator *)iterator);
        if (enc == 0) {
            if (iterator->encounterless_done == 0) {
                iterator->next_actor_index = ai_globals_ptr->first_encounterless_actor;
                iterator->encounterless_done = 1;
            }
            break;
        }
        if (iterator->active == 0 || enc->units_active != 0) {
            iterator->next_actor_index = enc->first_actor;
        }
        next = iterator->next_actor_index;
    }

    do {
        next = iterator->next_actor_index;
        iterator->actor_index = next;
        if (next == (datum_index)k_datum_index_none) {
            return 0;
        }
        a = &((actor *)actor_data->data)[next & 0xffff];
        iterator->next_actor_index = a->next_in_encounter;
    } while (iterator->active != 0 && a->active == 0);

    return a;
}

#if 0
Original Ghidra decompilation (0x436a70):

int actor_iterator_next(void)

{
  uint uVar1;
  int iVar2;
  int in_EAX;
  int iVar3;
  int iVar4;

  iVar4 = DAT_00880354;
  if (*(char *)(DAT_00880354 + 1) == '\0') {
LAB_00436b04:
    iVar3 = 0;
  }
  else {
    iVar3 = *(int *)(in_EAX + 0x18);
    while (iVar2 = DAT_00880360, iVar3 == -1) {
      iVar3 = data_iterator_next();
      if (iVar3 == 0) {
        iVar2 = DAT_00880360;
        if (*(char *)(in_EAX + 0x10) == '\0') {
          *(undefined4 *)(in_EAX + 0x18) = *(undefined4 *)(iVar4 + 8);
          *(undefined1 *)(in_EAX + 0x10) = 1;
          iVar2 = DAT_00880360;
        }
        break;
      }
      if ((*(char *)(in_EAX + 0x11) == '\0') || (*(char *)(iVar3 + 0xd) != '\0')) {
        *(undefined4 *)(in_EAX + 0x18) = *(undefined4 *)(iVar3 + 0x14);
      }
      iVar3 = *(int *)(in_EAX + 0x18);
    }
    do {
      uVar1 = *(uint *)(in_EAX + 0x18);
      *(uint *)(in_EAX + 0x14) = uVar1;
      if (uVar1 == 0xffffffff) goto LAB_00436b04;
      iVar4 = (uVar1 & 0xffff) * 0x724;
      iVar3 = iVar4 + *(int *)(iVar2 + 0x34);
      *(undefined4 *)(in_EAX + 0x18) = *(undefined4 *)(iVar4 + 0x2c + *(int *)(iVar2 + 0x34));
    } while ((*(char *)(in_EAX + 0x11) != '\0') && (*(char *)(iVar3 + 8) == '\0'));
  }
  return iVar3;
}
#endif
