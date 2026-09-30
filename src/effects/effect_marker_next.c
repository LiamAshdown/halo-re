// effect_marker_next  (Ghidra: FUN_00453180, still unnamed there)
// address 0x453180, size 151 bytes
// name confidence: 0.4 (Ghidra left it unnamed; out/phase4/effects_types_notes.md calls it
//   "effect_marker_next 0x453180" while establishing effect_location_marker.marker_index, and
//   that description matches the body exactly)   rewrite confidence: 0.6
// evidence: types/effects.h effect.first_person_weapon_index (+0x4c) and
//   effect_location_marker.next_marker (+0x04) / marker_index (+0x02); types/game.h
//   player_globals (local_player_globals, 0x0087a478).
// register convention: none -- all three arguments are Ghidra-recognized stack parameters.
// UNSURE: the exact meaning of evaluation mode 1 vs 3, and of player_globals.local_player_count ("is the
//   local player in first person view"?); kept verbatim from the decompiled comparisons.
// reconciled: R34 player_globals.unknown_0c -> local_player_count (int16 at +0x0c, same width)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "effects.h"
#include "game.h"
#include "fn_effects.h"

extern data_array *effect_location_data;      // 0x0087abe0
extern player_globals *local_player_globals;  // 0x0087a478

// Walks the linked list of effect_location_marker records starting at *marker (advancing *marker
// to the next entry as it goes) and returns the first entry whose first-person-ness matches the
// requested evaluation mode: mode 1, or mode 3 while self is a first-person-weapon effect and
// the local player is in first person view, keeps only markers carrying the first-person bit
// (0x8000) or the "object origin" sentinel 0xffff; any other mode keeps only plain markers.
// Returns NULL once the list is exhausted without a match.
effect_location_marker *effect_marker_next(effect *self, datum_index *marker, int32_t mode)
{
    effect_location_marker *entry;

    if (*marker == (datum_index)0xffffffff) {
        return (effect_location_marker *)0;
    }

    entry = &((effect_location_marker *)effect_location_data->data)[*marker & 0xffff];
    *marker = entry->next_marker;

    if (mode == 1 ||
        (mode == 3 && self->first_person_weapon_index != -1 &&
         local_player_globals->local_player_count == 1)) {
        if (entry->marker_index == 0xffff || (entry->marker_index & 0x8000) == 0) {
            return effect_marker_next(self, marker, mode);
        }
    } else if (entry->marker_index != 0xffff && (entry->marker_index & 0x8000) != 0) {
        return effect_marker_next(self, marker, mode);
    }

    return entry;
}

#if 0
Original Ghidra decompilation (0x453180):

int FUN_00453180(int param_1,uint *param_2,undefined4 param_3)

{
  int iVar1;

  iVar1 = 0;
  if (*param_2 != 0xffffffff) {
    iVar1 = (*param_2 & 0xffff) * 0x3c + *(int *)(DAT_0087abe0 + 0x34);
    *param_2 = *(uint *)(iVar1 + 4);
    if (((short)param_3 == 1) ||
       ((((short)param_3 == 3 && (*(short *)(param_1 + 0x4c) != -1)) &&
        (*(short *)(DAT_0087a478 + 0xc) == 1)))) {
      if ((*(ushort *)(iVar1 + 2) == 0xffff) || ((*(ushort *)(iVar1 + 2) & 0x8000) == 0)) {
        iVar1 = FUN_00453180(param_1,param_2,param_3);
      }
    }
    else if ((*(ushort *)(iVar1 + 2) != 0xffff) && ((*(ushort *)(iVar1 + 2) & 0x8000) != 0)) {
      iVar1 = FUN_00453180(param_1,param_2,param_3);
      return iVar1;
    }
  }
  return iVar1;
}
#endif
