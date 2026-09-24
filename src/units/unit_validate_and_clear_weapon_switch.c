// unit_validate_and_clear_weapon_switch  (Ghidra: unit_validate_and_clear_weapon_switch)
// address 0x5659c0, size 176 bytes
// name confidence: 0.35 (phase2 candidate: "unit_clear_weapon_change_flags")   rewrite
//   confidence: 0.25
// evidence: types/units.h unit_data.zoom_level/.desired_zoom_level (0x320/0x321),
//   .unknown_348 (0x348), .current_weapon_index (0x2f2), .weapons[4] (0x2f8),
//   unit_data.unknown_4bc via the target weapon *object's* saved_control source id (0x4bc).
//   Shares its tail with unit_clear_weapon_switch_state (0x565a70).
// register convention: unit index in EAX.
//   // blam-cc: param_1 (EAX) -> unit_index
// UNSURE: players_iterate_and_discard is called twice back to back with the same argument and no evidence of a
//   side effect between the calls; both are reproduced literally rather than deduplicated.
//   sound_start_unspatialized's argument is the float bit pattern 0x3f800000 (1.0f); its purpose and the
//   short at player+2 it gates on are unresolved outside this batch.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t players_iterate_and_discard(uint32_t unit_index); // 0x474db0, UNSURE: likely resolves the controlling player index
extern void sound_start_unspatialized(float amount);           // 0x543dd0, UNSURE signature
extern void unit_invalidate_local_player_zoom_level(void);                   // 0x4726f0, UNSURE: no traced args

void unit_validate_and_clear_weapon_switch(uint32_t unit_index) // blam-cc: param_1 (EAX) -> unit_index
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);

    int32_t player = players_iterate_and_discard(unit_index);
    if (player != -1) {
        uint32_t player2 = (uint32_t)players_iterate_and_discard(unit_index);
        int16_t *player_field = (int16_t *)((uint8_t *)player_data->data + (player2 & 0xffff) * 0x200 + 2);
        if (*player_field != -1 && unit->zoom_level != -1) {
            int16_t slot = unit->current_weapon_index;
            if (slot != -1 && unit->weapons[slot] != (datum_index)-1) {
                object *weapon = ((object_header *)object_data->data)[unit->weapons[slot] & 0xffff].data;
                void *weapon_tag = tag_instances[weapon->definition_tag & 0xffff].data;
                if (*(int32_t *)((uint8_t *)weapon_tag + 0x4bc) != -1) { // UNSURE: raw Weapon-tag field, not in types/tags.h by this offset here
                    sound_start_unspatialized(1.0f);
                }
            }
        }
    }
    unit->zoom_level = -1;
    unit->desired_zoom_level = -1;
    unit->unknown_348 = 0.0f;
    unit_invalidate_local_player_zoom_level();
}

#if 0
Original Ghidra decompilation (0x5659c0):

void FUN_005659c0(uint param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;

  iVar3 = DAT_008603b0;
  iVar6 = (param_1 & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  iVar4 = FUN_00474db0(param_1);
  if (iVar4 != -1) {
    uVar5 = FUN_00474db0(param_1);
    if ((*(short *)((uVar5 & 0xffff) * 0x200 + 2 + *(int *)(DAT_0087a480 + 0x34)) != -1) &&
       (*(char *)(iVar2 + 800) != -1)) {
      iVar4 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + iVar6);
      sVar1 = *(short *)(iVar4 + 0x2f2);
      if ((sVar1 != -1) &&
         ((uVar5 = *(uint *)(iVar4 + 0x2f8 + sVar1 * 4), uVar5 != 0xffffffff &&
          (*(int *)(*(int *)((**(uint **)(*(int *)(iVar3 + 0x34) + 8 + (uVar5 & 0xffff) * 0xc) &
                             0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x4bc) != -1)))) {
        FUN_00543dd0(0x3f800000);
      }
    }
  }
  *(undefined1 *)(iVar2 + 800) = 0xff;
  *(undefined1 *)(iVar2 + 0x321) = 0xff;
  *(undefined4 *)(iVar2 + 0x348) = 0;
  FUN_004726f0();
  return;
}
#endif
