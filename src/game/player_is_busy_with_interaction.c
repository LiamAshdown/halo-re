// player_is_busy_with_interaction  (Ghidra: FUN_00478820; renamed per
// out/phase4/game_functions.md: "Returns true when the player is currently busy with another
// action, so a new interaction prompt (board/swap/assassinate) should not be shown.")
// address 0x478820, size 114 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-27 static loop: objdump 0x478820..0x478891, helper conventions match)
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x478820
//   --stop-address=0x478890): two register-passed inputs (ESI, EDI), no stack parameters;
//   types/objects.h _object_mask_weapon (4).
// register convention: ESI -> candidate_object (used for unit_lacks_weapon_type_of/object_try_and_get/
//   game_engine_ctf_unit_weapon_must_be_readied), EDI -> unit_or_player_index (used only for unit_count_deployed_weapons and as unit_lacks_weapon_type_of's
//   second argument); no stack parameters.
//   // blam-cc: ESI -> candidate_object, EDI -> unit_or_player_index
// UNSURE: unit_count_deployed_weapons/unit_lacks_weapon_type_of/game_engine_ctf_unit_weapon_must_be_readied's real identities and argument meaning beyond
//   what this call site shows; the weapon tag field at +0x308 bit 0x10's meaning (types/units.h
//   documents an unrelated 0x308 field on unit_data -- this one is on the Weapon TAG's own
//   data, a different structure, and is not named anywhere in this batch's evidence).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern tag_instance *tag_instances;                 // 0x0087bc14
extern game_engine_definition *current_game_engine; // 0x006f1d20

extern int16_t unit_count_deployed_weapons(uint32_t unit_or_player_index); // 0x56d990, not in this batch
extern uint8_t unit_lacks_weapon_type_of(uint32_t candidate_object, uint32_t unit_or_player_index); // 0x56da80, not in this batch
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t game_engine_ctf_unit_weapon_must_be_readied(uint32_t candidate_object); // 0x466bc0, not in this batch

// blam-cc: ESI -> candidate_object, EDI -> unit_or_player_index
// Returns 1 (busy) if the candidate is a weapon whose tag data has bit 0x10 set at +0x308, or if
// unit_count_deployed_weapons reports a nonzero value AND (in single player, unit_lacks_weapon_type_of approves and that
// value is under 2, OR game_engine_ctf_unit_weapon_must_be_readied rejects the candidate). Otherwise returns 0.
uint8_t player_is_busy_with_interaction(uint32_t candidate_object, uint32_t unit_or_player_index)
{
    int16_t value = unit_count_deployed_weapons(unit_or_player_index);

    if (unit_lacks_weapon_type_of(candidate_object, unit_or_player_index)) {
        object *tag_data = object_try_and_get(candidate_object, 4);
        // UNSURE: `tag_data` here is actually the definition_tag's own tag_instance data
        // pointer, per Ghidra's `*(int*)eax` right after the call -- object_try_and_get's
        // return here is dereferenced as a datum_index, then re-looked-up in tag_instances.
        datum_index definition_tag = *(datum_index *)tag_data;
        uint8_t *weapon_tag_data = (uint8_t *)tag_instances[definition_tag & 0xffff].data;
        if ((weapon_tag_data[0x308] & 0x10) != 0) {
            return 1;
        }
    }

    if (value != 0) {
        if (current_game_engine == 0) {
            if (unit_lacks_weapon_type_of(candidate_object, unit_or_player_index) != 0 && value < 2) {
                return 1;
            }
        }
        if (game_engine_ctf_unit_weapon_must_be_readied(candidate_object) == 0) {
            return 0;
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x478820), from tools/pack.py 0x478820:

undefined1 FUN_00478820(void)

{
  char cVar1;
  short sVar2;
  uint *puVar3;

  sVar2 = FUN_0056d990();
  cVar1 = FUN_0056da80();
  if ((cVar1 != '\0') &&
     (puVar3 = (uint *)object_try_and_get(4),
     (*(byte *)(*(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x308) & 0x10) != 0)) {
    return 1;
  }
  if (sVar2 != 0) {
    if (((DAT_006f1d20 == 0) && (cVar1 = FUN_0056da80(), cVar1 != '\0')) && (sVar2 < 2)) {
      return 1;
    }
    cVar1 = FUN_00466bc0();
    if (cVar1 == '\0') {
      return 0;
    }
  }
  return 1;
}
#endif
