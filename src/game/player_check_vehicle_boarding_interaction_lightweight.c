// player_check_vehicle_boarding_interaction_lightweight  (Ghidra: FUN_00478c40; renamed per
// out/phase4/game_functions.md: "A lightweight variant of the vehicle-boarding interaction check
// used by player_update_nearby_interactions_secondary.")
// address 0x478c40, size 444 bytes
// name confidence: 0.35   rewrite confidence: 0.25
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
extern data_array *object_headers; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern uint8_t unit_check_weapon_use_permission(void); // 0x56da00, not in this batch
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970
extern int16_t unit_count_deployed_weapons(void); // 0x56d990, not in this batch
extern uint8_t player_is_busy_with_interaction(void); // this batch, 0x478820; UNSURE args elided, see sibling file
extern uint8_t unit_weapon_is_best_of_type(void); // 0x56dae0, not in this batch
extern void player_set_pending_interaction_action(int16_t priority_type, int16_t seat,
    uint32_t player_index, uint32_t candidate_object); // this batch, 0x478e00

// Lightweight version of player_check_vehicle_boarding_interaction: no ammo-transfer or device
// handling, only the weapon-pickup/swap/assassination decision.
void player_check_vehicle_boarding_interaction_lightweight(uint32_t player_index, uint32_t candidate_object)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    datum_index unit_handle = p->unit;
    object *unit_obj = (object *)((object_header *)object_headers->data)[unit_handle & 0xffff].data;
    object *candidate = (object *)((object_header *)object_headers->data)[candidate_object & 0xffff].data;

    if (candidate->parent_object == (datum_index)0xffffffff &&
        *(uint32_t *)((uint8_t *)candidate + 0x200) != (uint32_t)unit_handle) {
        object *weapon_candidate = object_try_and_get(candidate_object, 4);
        if (weapon_candidate != 0 && unit_check_weapon_use_permission() != 0) {
            tag_instance *weapon_tag = &tag_instances[weapon_candidate->definition_tag & 0xffff];
            uint32_t flags_208 = *(uint32_t *)((uint8_t *)unit_obj + 0x208);
            datum_index current_weapon = unit_get_weapon_object_index((uint32_t)unit_handle, -1);
            int16_t weapon_state = unit_count_deployed_weapons();
            uint8_t assassination_target = 0;

            if (1 < weapon_state && current_weapon != (datum_index)0xffffffff &&
                (*(uint8_t *)((uint8_t *)weapon_tag->data + 0x308) & 0x10) == 0) {
                object *current_weapon_obj = (object *)((object_header *)object_headers->data)[current_weapon & 0xffff].data;
                tag_instance *current_weapon_tag = &tag_instances[current_weapon_obj->definition_tag & 0xffff];
                if ((*(uint8_t *)((uint8_t *)current_weapon_tag->data + 0x308) & 0x10) != 0) {
                    assassination_target = 1;
                }
            }

            if (((flags_208 & 0x1800) == 0 || (*(uint8_t *)((uint8_t *)weapon_tag->data + 0x308) & 8) == 0) &&
                player_is_busy_with_interaction() == 0 && !assassination_target && unit_weapon_is_best_of_type() != 0) {
                object *reacquired = object_try_and_get(candidate_object, 4);
                uint32_t priority;
                if (weapon_state == 1 && reacquired != 0 &&
                    *(uint32_t *)reacquired != *(uint32_t *)weapon_candidate) {
                    priority = 7;
                } else {
                    priority = 6;
                }
                player_set_pending_interaction_action((int16_t)priority, (int16_t)0xffff, player_index, candidate_object);
            }
        }
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
