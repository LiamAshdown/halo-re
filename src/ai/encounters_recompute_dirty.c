// encounters_recompute_dirty  (Ghidra: encounters_recompute_dirty; named for this rewrite)
// address 0x435f00, size 129 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: phase-4 summary ("scans all squads and re-runs the morale/combat-status
//   recompute routine for any squad flagged dirty since the last pass"); types/ai.h already
//   documents encounter.dirty at +0x28 as "set by every member add / remove; 0x435f00
//   re-runs morale for dirty encounters", and encounter_recompute_morale @0x437940 is the
//   function that clears it again at its end.
// register convention: no arguments. The encounter_iterator layout is taken straight from
//   the disassembly (objdump -d -M intel --start-address=0x435f00 --stop-address=0x435f85
//   bin/halo.exe).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940

// Re-runs the aggregate morale / combat-status recompute for every encounter whose member
// list changed since the last pass.
void encounters_recompute_dirty(void)
{
    encounter_iterator iterator;
    encounter *enc;

    if (ai_globals_ptr->actors_valid != 0) {
        iterator.data = encounter_data;
        iterator.next_index = 0;
        iterator.index = (datum_index)k_datum_index_none;
        iterator.signature = (uint32_t)encounter_data ^ 0x69746572;
        iterator.active_only = 0;
    }

    for (;;) {
        if (ai_globals_ptr->actors_valid == 0) {
            return;
        }
        do {
            enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
            if (enc == 0 || iterator.active_only == 0) {
                break;
            }
        } while (enc->units_active == 0);
        iterator.encounter_index = iterator.index;
        if (enc == 0) {
            return;
        }
        if (enc->dirty != 0) {
            encounter_recompute_morale(iterator.encounter_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x435f00):

void FUN_00435f00(void)

{
  int iVar1;
  undefined4 local_10;
  char local_4;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_10 = 0xffffffff;
    local_4 = '\0';
  }
  do {
    if (*(char *)(DAT_00880354 + 1) == '\0') {
      return;
    }
    do {
      iVar1 = data_iterator_next();
      if ((iVar1 == 0) || (local_4 == '\0')) break;
    } while (*(char *)(iVar1 + 0xd) == '\0');
    if (iVar1 == 0) {
      return;
    }
    if (*(char *)(iVar1 + 0x28) != '\0') {
      FUN_00437940(local_10);
    }
  } while( true );
}
#endif
