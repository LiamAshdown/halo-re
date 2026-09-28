// unit_validate_and_clear_weapon_switch  (Ghidra: unit_validate_and_clear_weapon_switch)
// address 0x5659c0, size 226 bytes (0x5659c0..0x565aa1; 0x565a70 is its tail)
// name confidence: 0.35 (phase2 candidate: "unit_clear_weapon_change_flags")   rewrite
//   confidence: 0.9
// REWRITTEN from objdump 0x5659c0..0x565aa2 (the draft lost the zoom-out sound tag, EDX, and the tail call's
//   unit, EAX). Stack: unit. A zoomed-in (+0x320 != -1) unit of a local player plays its current weapon's
//   zoom-out sound (weapon tag +0x4bc, unspatialized, scale 1); then the zoom level (+0x320), desired zoom
//   (+0x321) and zoom blend (+0x348) are reset and the local player's zoom is invalidated (tail jump 0x4726f0).
// blam-cc: stack -> unit_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern data_array *player_data;     // 0x0087a480
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern datum_index sound_start_unspatialized(datum_index definition_index, float scale); // 0x543dd0, EDX, stack
extern void unit_invalidate_local_player_zoom_level(datum_index unit); // 0x4726f0, EAX

void unit_validate_and_clear_weapon_switch(uint32_t unit_index)
{
    uint8_t *obj = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (player_index_from_unit_index(unit_index) != k_datum_index_none) {
        datum_index player_index = player_index_from_unit_index(unit_index);

        if (*(int16_t *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200 + 2) != -1 &&
            obj[0x320] != 0xff) {
            uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
            int16_t slot = ((unit_object *)unit)->unit.current_weapon_index;

            if (slot != -1 && *(datum_index *)(unit + 0x2f8 + slot * 4) != k_datum_index_none) {
                uint8_t *weapon = (uint8_t *)((object_header *)object_data->data)
                    [*(datum_index *)(unit + 0x2f8 + slot * 4) & 0xffff].data;
                datum_index zoom_sound = *(datum_index *)((uint8_t *)tag_instances[*(datum_index *)weapon & 0xffff].data + 0x4bc);

                if (zoom_sound != k_datum_index_none) {
                    sound_start_unspatialized(zoom_sound, 1.0f);
                }
            }
        }
    }
    obj[0x320] = 0xff;
    obj[0x321] = 0xff;
    *(float *)(obj + 0x348) = 0.0f;
    unit_invalidate_local_player_zoom_level(unit_index);
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
