// hs_reposition_players_outside_trigger_volume  (Ghidra: FUN_00487750)
// address 0x487750, size 200 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Iterates all entries of an object-related
//   datum array and resets/detaches each one whose predicate check fails"; the array is
//   hardcoded to `players` and the predicate is specifically trigger-volume containment)
// rewrite confidence: 0.45
// evidence: types/hs.h globals (players 0x0087a480 stride 0x200, unit handle at +0x34);
//   src/memory/datum_next.c for the iteration; this module's hs_object_detach_and_place_at_location
//   (0x487f50) for the teleport call, whose (object, detach=1, reorient=1) argument pattern
//   matches exactly.
// register convention: location index in AX (in_AX, passed straight through to
//   hs_object_detach_and_place_at_location); trigger volume index in ECX (in_ECX, passed straight
//   through to scenario_trigger_volume_contains_point). Neither is read or written locally.
//   // blam-cc: AX -> location_index, ECX -> trigger_volume_index
// UNSURE: both register arguments are pure pass-through with no local use, so their exact source
//   registers are inferred from the two callees' own (also register-implicit) parameters rather
//   than observed directly; the overall purpose (move every player whose unit has strayed outside
//   a trigger volume back to a named location) is inferred from the combination of the two calls.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern datum_index datum_next(int16_t after_index, data_array *array);
    // blam-cc: DX -> after_index, EDI -> array; memory module, 0x4d0630
extern char scenario_trigger_volume_contains_point(int32_t trigger_volume_index,
    datum_index object_index);
    // blam-cc: register args UNSURE, see hs_object_list_test_trigger_volume.c;
    // scenario module, 0x53f020, not yet rewritten
extern void hs_object_detach_and_place_at_location(int16_t location_index, datum_index object_index,
    char detach_from_parent, char reorient); // this module, 0x487f50

extern data_array *players; // 0x0087a480, stride 0x200

// hs_player_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// For every live player whose unit is outside `trigger_volume_index`, detaches and repositions
// that unit onto Scenario::cutscene_flags[location_index] (see hs_object_detach_and_place_at_location).
void hs_reposition_players_outside_trigger_volume(int16_t location_index, int32_t trigger_volume_index)
{
    datum_index player_index;
    datum_index unit;

    player_index = datum_next(-1, players);
    while (player_index != k_datum_index_none) {
        unit = ((hs_player_record *)((uint8_t *)players->data +
            (player_index & 0xffff) * 0x200))->unit;
        if (unit != k_datum_index_none &&
            scenario_trigger_volume_contains_point(trigger_volume_index, unit) == 0) {
            hs_object_detach_and_place_at_location(location_index, unit, 1, 1);
        }
        player_index = datum_next((int16_t)player_index, players);
    }
}

#if 0
Original Ghidra decompilation (0x487750):

void FUN_00487750(void)

{
  char cVar1;
  uint uVar2;
  short *psVar3;
  short sVar4;
  int iVar5;
  int iVar6;
  int iVar7;

  iVar7 = DAT_0087a480;
  uVar2 = FUN_004d0630();
  do {
    do {
      if (uVar2 == 0xffffffff) {
        return;
      }
      iVar5 = *(int *)(iVar7 + 0x34);
      iVar6 = (uVar2 & 0xffff) * 0x200;
      if ((*(int *)(iVar6 + 0x34 + iVar5) != -1) &&
         (cVar1 = scenario_trigger_volume_contains_point(), cVar1 == '\0')) {
        FUN_00487f50(*(undefined4 *)(iVar6 + iVar5 + 0x34),1,1);
        iVar7 = DAT_0087a480;
      }
      iVar5 = uVar2 + 1;
      uVar2 = 0xffffffff;
      sVar4 = (short)iVar5;
    } while ((sVar4 < 0) || (*(short *)(iVar7 + 0x2e) <= sVar4));
    psVar3 = (short *)((int)sVar4 * (int)*(short *)(iVar7 + 0x22) + *(int *)(iVar7 + 0x34));
    do {
      if (*psVar3 != 0) {
        uVar2 = (int)*psVar3 << 0x10 | (int)(short)iVar5;
        break;
      }
      iVar5 = iVar5 + 1;
      psVar3 = (short *)((int)psVar3 + (int)*(short *)(iVar7 + 0x22));
    } while ((short)iVar5 < *(short *)(iVar7 + 0x2e));
  } while( true );
}
#endif
