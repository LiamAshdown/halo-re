// player_check_vehicle_boarding_interaction_lightweight  (Ghidra: FUN_00478c40; renamed per
// out/phase4/game_functions.md: "A lightweight variant of the vehicle-boarding interaction check
// used by player_update_nearby_interactions_secondary.")
// address 0x478c40, size 444 bytes
// name confidence: 0.35   rewrite confidence: 0.9 (REWRITTEN from objdump 0x478c40..0x478dfb)
// evidence: a strict subset of the sibling player_check_vehicle_boarding_interaction.c (this
//   batch, 0x4788a0) -- it skips that function's ammo-transfer and device-interaction blocks and
//   uses unit_get_weapon_object_index (already established elsewhere) in place of manually
//   indexing unit::weapons[current_weapon_index]. Same UNSURE caveats apply: most of the tag
//   field offsets here (+0x200, +0x208, +0x308) are not named by any header this module owns.
// register convention: none -- both are genuine stack parameters (Ghidra's own
//   param_1/param_2).
// UNSURE: unit_get_weapon_object_index's slot_index argument (called here with zero visible
//   arguments; modeled as -1, "current weapon", matching this function's own established
//   default reading elsewhere in the module); everything else per the header note above.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_check_weapon_use_permission(uint32_t unit_index, uint32_t weapon_index); // 0x56da00, ESI, EDI
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern int16_t unit_count_deployed_weapons(uint32_t unit_index); // 0x56d990, EAX
extern uint8_t player_is_busy_with_interaction(uint32_t candidate_object, uint32_t unit_or_player_index); // 0x478820, ESI, EDI
extern uint8_t unit_weapon_is_best_of_type(uint32_t reference_weapon_index, uint32_t unit_index); // 0x56dae0, EAX, ECX
extern void player_set_pending_interaction_action(int16_t priority_type, int16_t seat,
    uint32_t player_index, uint32_t candidate_object); // this batch, 0x478e00

// Lightweight version of player_check_vehicle_boarding_interaction: no ammo-transfer or device
// handling, only the weapon-pickup/swap/assassination decision.
// REWRITTEN from objdump. The weapon-pickup prompt: an unparented weapon (type mask 4) that is not the player's
//   unit's own +0x200 object, which the unit may use (ESI unit, EDI weapon), unless the unit has flag 0x1800 and
//   the weapon tag +0x308 bit 3, or the player is busy (ESI weapon, EDI unit), or the unit already carries two
//   weapons and holds a +0x308 bit 4 weapon while this one is not, and only when it is the best of its type
//   (EAX weapon, ECX unit). The action is 7 when the unit holds exactly one weapon whose definition differs,
//   else 6. The draft called four helpers without arguments, asked for weapon slot -1, and re-fetched the
//   candidate where the binary fetches the current weapon.
void player_check_vehicle_boarding_interaction_lightweight(uint32_t player_index, uint32_t candidate_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit_handle = p->unit;
    uint8_t *unit_obj = (uint8_t *)((object_header *)object_data->data)[unit_handle & 0xffff].data;
    uint8_t *candidate = (uint8_t *)((object_header *)object_data->data)[candidate_object & 0xffff].data;
    object *weapon_candidate;
    uint8_t *weapon_tag;
    uint8_t unit_flag_1800;
    datum_index current_weapon;
    int32_t weapon_count;
    uint8_t holds_exclusive = 0;
    object *current_weapon_obj;

    if (*(datum_index *)(candidate + 0x11c) != (datum_index)0xffffffff ||
        *(uint32_t *)(candidate + 0x200) == (uint32_t)unit_handle) {
        return;
    }
    weapon_candidate = object_try_and_get(candidate_object, 4);
    if (weapon_candidate == 0 || unit_check_weapon_use_permission((uint32_t)unit_handle, candidate_object) == 0) {
        return;
    }
    weapon_tag = (uint8_t *)tag_instances[weapon_candidate->definition_tag & 0xffff].data;
    unit_flag_1800 = (uint8_t)((*(uint32_t *)(unit_obj + 0x208) & 0x1800) != 0);

    current_weapon = unit_get_weapon_object_index((uint32_t)p->unit,
        *(int16_t *)((uint8_t *)((object_header *)object_data->data)[p->unit & 0xffff].data + 0x2f2));
    weapon_count = unit_count_deployed_weapons((uint32_t)p->unit);
    if (weapon_count >= 2 && current_weapon != (datum_index)0xffffffff && (weapon_tag[0x308] & 0x10) == 0) {
        object *held = ((object_header *)object_data->data)[current_weapon & 0xffff].data;

        if ((((uint8_t *)tag_instances[held->definition_tag & 0xffff].data)[0x308] & 0x10) != 0) {
            holds_exclusive = 1;
        }
    }
    if (unit_flag_1800 && (weapon_tag[0x308] & 8) != 0) {
        return;
    }
    if (player_is_busy_with_interaction(candidate_object, (uint32_t)p->unit) != 0 || holds_exclusive) {
        return;
    }
    if (unit_weapon_is_best_of_type(candidate_object, (uint32_t)p->unit) == 0) {
        return;
    }
    current_weapon_obj = object_try_and_get(current_weapon, 4);
    if (weapon_count == 1 && current_weapon_obj != 0 &&
        current_weapon_obj->definition_tag != weapon_candidate->definition_tag) {
        player_set_pending_interaction_action(7, (int16_t)0xffff, player_index, candidate_object);
    } else {
        player_set_pending_interaction_action(6, (int16_t)0xffff, player_index, candidate_object);
    }
}

#if 0
Original Ghidra decompilation (0x478c40), from tools/pack.py 0x478c40:

void FUN_00478c40(uint param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  bool bVar4;
  char cVar5;
  short sVar6;
  uint *puVar7;
  uint uVar8;
  uint *puVar9;
  undefined4 uVar10;

  uVar1 = *(uint *)((param_1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x34);
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar1 & 0xffff) * 0xc);
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  if ((((*(int *)(iVar3 + 0x11c) == -1) && (*(uint *)(iVar3 + 0x200) != uVar1)) &&
      (puVar7 = (uint *)object_try_and_get(4), puVar7 != (uint *)0x0)) &&
     (cVar5 = FUN_0056da00(), cVar5 != '\0')) {
    iVar3 = *(int *)((*puVar7 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    uVar1 = *(uint *)(iVar2 + 0x208);
    iVar2 = *(int *)(DAT_008603b0 + 0x34);
    uVar8 = unit_get_weapon_object_index();
    sVar6 = FUN_0056d990();
    bVar4 = false;
    if (((1 < sVar6) && (uVar8 != 0xffffffff)) &&
       (((*(byte *)(iVar3 + 0x308) & 0x10) == 0 &&
        ((*(byte *)(*(int *)((**(uint **)(iVar2 + 8 + (uVar8 & 0xffff) * 0xc) & 0xffff) * 0x20 +
                             0x14 + DAT_0087bc14) + 0x308) & 0x10) != 0)))) {
      bVar4 = true;
    }
    if ((((uVar1 & 0x1800) == 0) || ((*(byte *)(iVar3 + 0x308) & 8) == 0)) &&
       ((cVar5 = FUN_00478820(), cVar5 == '\0' &&
        ((!bVar4 && (cVar5 = FUN_0056dae0(), cVar5 != '\0')))))) {
      puVar9 = (uint *)object_try_and_get(4);
      if ((sVar6 == 1) && ((puVar9 != (uint *)0x0 && (*puVar9 != *puVar7)))) {
        uVar10 = 7;
      }
      else {
        uVar10 = 6;
      }
      player_set_pending_interaction_action(uVar10,0xffffffff);
    }
  }
  return;
}
#endif
