// ai_encounter_stamp_team_from_unit  (Ghidra: ai_encounter_stamp_team_from_unit; named for this rewrite)
// address 0x436710, size 83 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: the first time (gated by encounter.team == 0 and a global current_game_engine) an
// encounter is populated, stamps its team (types/ai.h encounter.team) from the given unit's
// object+0xb8 field (the same offset actor_attach_to_unit.c already uses for team, there
// also UNSURE), then notifies (ai_recompute_all_relationship_flags, outside this rewrite's range) if the encounter
// has a valid unknown_10.
// register convention: Ghidra resolved neither parameter.
//   // blam-cc: EAX -> encounter_index, ECX -> unit_index
// reconciled: R04 0x006f1d20 int32_t DAT_006f1d20 -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern data_array *encounter_data; // 0x008802c8
extern data_array *object_data;    // 0x008603b0
extern game_engine_definition *current_game_engine;       // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void ai_recompute_all_relationship_flags(void); // 0x42bbb0, outside this rewrite's range, UNSURE signature

// blam-cc: EAX -> encounter_index, ECX -> unit_index
void ai_encounter_stamp_team_from_unit(datum_index encounter_index, datum_index unit_index)
{
    encounter *enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];

    if (current_game_engine == 0 && enc->team == 0) {
        object_header *header = &((object_header *)object_data->data)[unit_index & 0xffff];
        enc->team = *(int16_t *)((uint8_t *)header->data + 0xb8); // UNSURE offset, see actor_attach_to_unit.c
        if (enc->activation_tick != (datum_index)k_datum_index_none) {
            ai_recompute_all_relationship_flags();
        }
    }
}

#if 0
Original Ghidra decompilation (0x436710):

void FUN_00436710(void)

{
  uint in_EAX;
  int iVar1;
  uint in_ECX;

  iVar1 = (in_EAX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  if (((DAT_006f1d20 == 0) && (*(short *)(iVar1 + 2) == 0)) &&
     (*(undefined2 *)(iVar1 + 2) =
           *(undefined2 *)
            (*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc) + 0xb8),
     *(int *)(iVar1 + 0x14) != -1)) {
    FUN_0042bbb0();
    return;
  }
  return;
}
#endif
