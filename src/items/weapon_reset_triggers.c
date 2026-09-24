// weapon_reset_triggers  (Ghidra: FUN_004c4b50; named per types/items.h,
// "_weapon_trigger_effect_reset = 8 // weapon_reset_triggers (0x4c4b50)")
// address 0x4c4b50, size 233 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: types/items.h weapon_trigger_state.effect_state/.effect_state_ticks,
//   weapon_magazine_state.state/.state_ticks; types/tags.h Weapon.triggers (0x4fc),
//   Weapon.magazines (0x4f0).
// register convention: item index is a Ghidra-recognized parameter (passed in a register that
// most call sites in this module leave unchanged, hence showing no visible argument there).
// blam-cc: reg -> item_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "items.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t weapon_get_first_person_animation_time(datum_index item_index, int16_t animation_index,
    int16_t category, int16_t mode); // 0x4c2f80
extern void weapon_magazine_reload_tick(datum_index item_index, int16_t magazine_index); // 0x4c3900
extern void weapon_notify_reload_cancel(void); // 0x4c4a00, this module (see weapon_apply_ammo_correction_and_resync)

// Resets every trigger's effect state to the "reset" sentinel and every magazine back to idle,
// nudging along any magazine that was mid-reload so its animation isn't left stranded. Used by
// weapon_ready, weapon_put_away and the network ammo-correction resync path.
void weapon_reset_triggers(datum_index item_index)
{
    object *item_obj;
    weapon_data *wd;
    int16_t i;

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    Weapon *weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;

    for (i = 0; i < weapon_tag->triggers.count; i++) {
        wd->triggers[i].effect_state = _weapon_trigger_effect_reset;
        wd->triggers[i].effect_state_ticks = 0;
    }

    for (i = 0; i < weapon_tag->magazines.count; i++) {
        weapon_magazine_state *magazine = &wd->magazines[i];

        if (magazine->state == _weapon_magazine_reloading) {
            int16_t fresh_length = weapon_get_first_person_animation_time(item_index, 0, 0, -1); // UNSURE:
                // animation_index (CX) placeholder, see weapon_get_first_person_animation_time.c
            if (magazine->state_ticks * 2 < fresh_length) {
                if (item_obj->network_role != 1) {
                    weapon_magazine_reload_tick(item_index, i);
                }
            } else if (item_obj->network_role == 0) {
                weapon_notify_reload_cancel();
            }
        }
        magazine->state = 0;
        magazine->state_ticks = 0;
    }
}

#if 0
Original Ghidra decompilation (0x4c4b50):

void FUN_004c4b50(uint param_1)

{
  uint *puVar1;
  uint *puVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  short sVar6;

  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar6 = 0;
  sVar4 = 0;
  if (0 < *(int *)(iVar3 + 0x4fc)) {
    iVar5 = 0;
    do {
      sVar4 = sVar4 + 1;
      *(undefined1 *)((int)puVar2 + iVar5 * 0x28 + 0x261) = 8;
      *(undefined2 *)((int)puVar2 + iVar5 * 0x28 + 0x262) = 0;
      iVar5 = (int)sVar4;
    } while (iVar5 < *(int *)(iVar3 + 0x4fc));
  }
  if (0 < *(int *)(iVar3 + 0x4f0)) {
    iVar5 = 0;
    do {
      puVar1 = puVar2 + iVar5 * 3 + 0xac;
      if ((short)puVar2[iVar5 * 3 + 0xac] == 1) {
        sVar4 = FUN_004c2f80(0,0xffffffff);
        if (*(short *)((int)puVar1 + 2) * 2 < (int)sVar4) {
          if (puVar2[1] != 1) {
            FUN_004c3900(param_1);
          }
        }
        else if (puVar2[1] == 0) {
          FUN_004c4a00();
        }
      }
      sVar6 = sVar6 + 1;
      iVar5 = (int)sVar6;
      *(undefined2 *)puVar1 = 0;
      *(undefined2 *)((int)puVar1 + 2) = 0;
    } while (iVar5 < *(int *)(iVar3 + 0x4f0));
  }
  return;
}
#endif
