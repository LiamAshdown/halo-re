// encounter_set_team  (Ghidra: encounter_set_team; named for this rewrite)
// address 0x435b30, size 136 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump; arguments FIXED)
// evidence: phase-4 summary ("updates a short field on a squad record and notifies all of
//   its current members"); the short it writes is encounter+0x02, which types/ai.h already
//   names team (encounter_new copies ScenarioEncounter.team_index into it and
//   encounter_add_actor compares actor.team against it). It then walks the member list
//   calling the same pair of notifiers encounter_add_actor uses for a team change
//   (0x4276e0 per member, 0x42bbb0 once at the end).
// register convention: EAX -> encounter_index, CX -> team.
//   // blam-cc: EAX -> encounter_index, CX -> team
//
// UNSURE: the member loop advances through actor.next_in_encounter but Ghidra shows
// actor_propagate_unit_field being called with no argument, so which actor each call
// refers to is inferred, not read. The final ai_recompute_all_relationship_flags (0x42bbb0) is outside the loop
// and is likewise argument-less in Ghidra.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360

extern void actor_propagate_unit_field(datum_index actor_index, int16_t value); // 0x4276e0, EAX, ESI
extern void ai_recompute_all_relationship_flags(void); // 0x42bbb0, no arguments (tail jump at 0x435bb3)

// blam-cc: EAX -> encounter_index, CX -> team
// Retargets a whole encounter onto a new team and tells every member about it.
void encounter_set_team(datum_index encounter_index, int16_t team)
{
    encounter *enc;
    datum_index actor_index;
    datum_index current;

    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    enc->team = team;

    actor_index = (datum_index)k_datum_index_none;
    if (ai_globals_ptr->actors_valid != 0) {
        if (encounter_index == (datum_index)0xffffffff) { // FIXED: the full handle is compared (0x435b63)
            actor_index = ai_globals_ptr->first_encounterless_actor;
        } else {
            actor_index = enc->first_actor;
        }
    }

    current = (datum_index)k_datum_index_none;
    while (ai_globals_ptr->actors_valid != 0 && actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        actor_index = ((actor *)actor_data->data)[current & 0xffff].next_in_encounter;
        actor_propagate_unit_field(current, team); // FIXED: ESI = the team (0x435b47)
    }
    ai_recompute_all_relationship_flags();
}

#if 0
Original Ghidra decompilation (0x435b30):

void FUN_00435b30(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  undefined2 in_CX;
  uint local_4;

  iVar2 = DAT_00880354;
  iVar1 = DAT_008802c8;
  iVar4 = (in_EAX & 0xffff) * 0x6c;
  *(undefined2 *)(*(int *)(DAT_008802c8 + 0x34) + 2 + iVar4) = in_CX;
  iVar3 = DAT_00880360;
  if (*(char *)(iVar2 + 1) != '\0') {
    if ((in_EAX & 0xffff) == 0xffffffff) {
      local_4 = *(uint *)(iVar2 + 8);
    }
    else {
      local_4 = *(uint *)(*(int *)(iVar1 + 0x34) + 0x14 + iVar4);
    }
  }
  while ((*(char *)(iVar2 + 1) != '\0' && (local_4 != 0xffffffff))) {
    local_4 = *(uint *)((local_4 & 0xffff) * 0x724 + 0x2c + *(int *)(iVar3 + 0x34));
    FUN_004276e0();
  }
  FUN_0042bbb0();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
