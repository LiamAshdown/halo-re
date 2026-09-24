// ai_build_priority_target_list  (Ghidra: ai_build_priority_target_list, already named)
// address 0x42acd0, size 372 bytes
// name confidence: 0.9   rewrite confidence: 0.45
// evidence: types/ai.h ai_globals.unknown_08 (head of the unassigned actor list),
//   actor.next_in_encounter(0x2c)/active(0x08)/unknown_0c(0x0c); encounter.units_active(0x0d)/
//   unknown_2a/unknown_10. Record layout {tiebreak, pad, handle, priority} confirmed against
//   ai_squad_priority_compare @0x42ac90 (this rewrite). Calls data_iterator_next (0x4d05d0,
//   memory module) and the C library _qsort.
//   UNSURE: the encounter-branch's record handle is written from a local that the original
//   only ever sets to -1 right before this loop and never updates inside it, so every
//   encounter-derived record's handle is genuinely always k_datum_index_none, not a real
//   encounter reference -- kept exactly as decompiled rather than assumed to be a bug. The
//   inner encounter-scan do-while's second exit condition is likewise always true (its flag
//   is set once to 0 and never changed), making that loop equivalent to a single
//   data_iterator_next() call per outer iteration; preserved literally.
// register convention: plain __cdecl, one stack argument.
//   // blam-cc: stack -> out_list
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>
#include <stdlib.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360
extern data_array *encounter_data; // 0x008802c8

// The output buffer: an int16 count, one more int16 (unused/padding), then up to 256
// records of 0xc bytes each.

extern int ai_squad_priority_compare(const ai_priority_target_record *record_a, const ai_priority_target_record *record_b); // 0x42ac90
extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: stack -> out_list
// Assembles a combined list of active unassigned actor-groups and eligible encounters (up to
// 256 entries), used to prioritize which AI groups to process or release, then sorts it by
// ai_squad_priority_compare.
void ai_build_priority_target_list(ai_priority_target_list *out_list)
{
    datum_index actor_index;

    out_list->count = 0;
    out_list->unknown_02 = 0;

    actor_index = ai_globals_ptr->actors_valid ? ai_globals_ptr->unknown_08 : (datum_index)k_datum_index_none;
    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
        datum_index next = a->next_in_encounter;

        if (out_list->count > 0xff) {
            break;
        }
        if (a->active == 0 && a->unknown_0c != (datum_index)k_datum_index_none) {
            ai_priority_target_record *rec = &out_list->records[out_list->count];
            rec->tiebreak = 1;
            rec->handle = actor_index;
            rec->priority = (int32_t)a->unknown_0c;
            out_list->count = out_list->count + 1;
        }
        actor_index = next;
    }

    {
        datum_index encounter_handle = (datum_index)k_datum_index_none;
        uint8_t scan_more = 0;
        data_iterator iterator;

        if (ai_globals_ptr->actors_valid != 0) {
            encounter_handle = (datum_index)k_datum_index_none;
            scan_more = 0;
        }

        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

        while (ai_globals_ptr->actors_valid != 0) {
            encounter *enc;

            do {
                enc = (encounter *)data_iterator_next(&iterator);
                if (enc == 0 || scan_more == 0) {
                    break;
                }
            } while (enc->units_active == 0);

            if (enc == 0 || out_list->count > 0xff) {
                break;
            }
            if (enc->units_active == 0 && enc->unknown_2a > 0 && enc->activation_tick != (datum_index)k_datum_index_none) {
                ai_priority_target_record *rec = &out_list->records[out_list->count];
                rec->tiebreak = 0;
                rec->handle = encounter_handle;
                rec->priority = (int32_t)enc->activation_tick;
                out_list->count = out_list->count + 1;
            }
        }
    }

    if (out_list->count > 0) {
        qsort(out_list->records, (size_t)out_list->count, sizeof(ai_priority_target_record),
              (int (*)(const void *, const void *))ai_squad_priority_compare);
    }
}

#if 0
Original Ghidra decompilation (0x42acd0):

void __cdecl ai_build_priority_target_list(short *out_list)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  uint uVar4;
  uint local_10;
  char local_4;

  iVar3 = DAT_00880354;
  *out_list = 0;
  out_list[1] = 0;
  iVar2 = DAT_00880360;
  uVar1 = local_10;
  if (*(char *)(iVar3 + 1) != '\0') {
    uVar1 = *(uint *)(iVar3 + 8);
  }
  while ((uVar4 = uVar1, *(char *)(DAT_00880354 + 1) != '\0' && (uVar4 != 0xffffffff))) {
    iVar3 = (uVar4 & 0xffff) * 0x724;
    uVar1 = *(uint *)(iVar3 + 0x2c + *(int *)(iVar2 + 0x34));
    iVar3 = iVar3 + *(int *)(iVar2 + 0x34);
    if (0xff < *out_list) break;
    if ((*(char *)(iVar3 + 8) == '\0') && (*(int *)(iVar3 + 0xc) != -1)) {
      *(undefined1 *)(out_list + *out_list * 6 + 2) = 1;
      *(uint *)(out_list + *out_list * 6 + 4) = uVar4;
      *(undefined4 *)(out_list + (*out_list + 1) * 6) = *(undefined4 *)(iVar3 + 0xc);
      *out_list = *out_list + 1;
    }
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_10 = 0xffffffff;
    local_4 = '\0';
  }
  while (*(char *)(DAT_00880354 + 1) != '\0') {
    do {
      iVar3 = data_iterator_next();
      if ((iVar3 == 0) || (local_4 == '\0')) break;
    } while (*(char *)(iVar3 + 0xd) == '\0');
    if ((iVar3 == 0) || (0xff < *out_list)) break;
    if ((*(char *)(iVar3 + 0xd) == '\0') &&
       ((0 < *(short *)(iVar3 + 0x2a) && (*(int *)(iVar3 + 0x10) != -1)))) {
      *(undefined1 *)(out_list + *out_list * 6 + 2) = 0;
      *(uint *)(out_list + *out_list * 6 + 4) = local_10;
      *(undefined4 *)(out_list + (*out_list + 1) * 6) = *(undefined4 *)(iVar3 + 0x10);
      *out_list = *out_list + 1;
    }
  }
  if (0 < *out_list) {
    _qsort(out_list + 2,(int)*out_list,0xc,ai_squad_priority_compare);
  }
  return;
}
#endif
