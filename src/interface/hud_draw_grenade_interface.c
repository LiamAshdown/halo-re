// hud_draw_grenade_interface  (Ghidra: FUN_004b2ac0; the first rewrite called it
// hud_unit_ammo_meters_update; renamed in the phase-4 review)
// address 0x4b2ac0, size 552 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.85
// evidence: rewritten from objdump 0x4b2ac0..0x4b2ce7 in the phase-4 review. Draws the
// GrenadeHUDInterface (types/tags.h) of the unit current grenade type (Globals grenades
// +0x12c, stride 0x44, hud_interface tag id +0x20): background (+0x24), total grenades
// background (+0x8c), total grenades number (+0xf4, count from unit_get_grenade_count) and
// the total grenades overlays (+0x14c). Nothing is drawn while the current weapon prevents
// grenade throwing (weapon_prevents_grenade_throwing, ECX weapon), without a grenade type,
// or while the unit is the driver or gunner of its parent (parent +0x324/+0x328). Flags: bit
// 0 count <= flash_cutoff (+0x148), bit 1 no grenades, bit 2 split screen; the flash start
// time is hud_weapon_state + index * 0x28 + 0x24. Overlay types: bit 0 flashing, bit 1
// empty, bit 2 default (none of them), bit 3 always.
// register convention: plain cdecl, two stack arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "objects.h"
#include "interface.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances;                  // 0x0087bc14
extern Globals *global_globals;                      // 0x00746fa0
extern player_globals *local_player_globals;         // 0x0087a478
extern game_time_globals *game_time;                 // 0x006f1d6c
extern hud_weapon_interface_state *hud_weapon_state; // 0x00719430

extern uint32_t weapon_prevents_grenade_throwing(datum_index item_index); // 0x4c2f30, blam-cc: ECX item_index
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index
extern int8_t unit_get_current_grenade_index(uint32_t unit_index); // 0x56e060, blam-cc: EAX unit_index
extern int32_t unit_get_grenade_count(uint32_t unit_index, int16_t grenade_type); // 0x56e030, blam-cc: EAX unit_index, CX grenade_type
extern void hud_draw_static_element(int16_t local_player_index, uint16_t *anchor,
                                    const hud_static_element_placement *element, uint32_t draw_flags,
                                    int32_t flash_start_time); // 0x4ac6f0
extern void hud_draw_number(void *unused, uint16_t *anchor, const hud_number_placement *placement, int16_t value,
                            int16_t fraction, uint32_t flags, int32_t flash_start_time, float scale); // 0x4ac0b0
extern void hud_draw_overlays(uint16_t *anchor, const hud_overlay_list *list, uint32_t type_mask,
                              int32_t flash_start_time, uint32_t draw_flags, uint8_t split_screen); // 0x4ac950

void hud_draw_grenade_interface(int16_t local_player_index, datum_index unit_index)
{
    uint8_t *unit = (uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    int16_t slot = *(int16_t *)(unit + 0x2f2);
    datum_index weapon = slot != -1 ? *(datum_index *)(unit + 0x2f8 + slot * 4) : (datum_index)-1;
    int8_t grenade = *(int8_t *)(unit + 0x31c); // current grenade index
    uint8_t *parent;
    datum_index hud_tag;
    GrenadeHUDInterface *hud;
    int32_t *flash_start_time;
    int8_t count;
    uint32_t flags;

    if (weapon_prevents_grenade_throwing(weapon) != 0 || grenade == -1) {
        return;
    }
    parent = (uint8_t *)object_try_and_get(*(datum_index *)(unit + 0x11c), 3);
    if (parent != 0 && (*(datum_index *)(parent + 0x324) == unit_index || *(datum_index *)(parent + 0x328) == unit_index)) {
        return;
    }
    hud_tag = *(datum_index *)(*(uint8_t **)((uint8_t *)global_globals + 0x12c) + grenade * 0x44 + 0x20);
    flash_start_time = (int32_t *)((uint8_t *)hud_weapon_state + local_player_index * 0x28 + 0x24);
    if (hud_tag == (datum_index)-1) {
        return;
    }
    hud = (GrenadeHUDInterface *)tag_instances[hud_tag & 0xffff].data;

    count = *(int8_t *)(unit + 0x31e + grenade);
    flags = (count <= hud->flash_cutoff ? 1 : 0) | (count == 0 ? 2 : 0) | (local_player_globals->unknown_0c > 1 ? 4 : 0);
    if ((flags & 1) != 0) {
        if (*flash_start_time == -1) {
            *flash_start_time = game_time->game_time;
        }
    } else {
        *flash_start_time = -1;
    }

    if (*(datum_index *)((uint8_t *)hud + 0x54) != (datum_index)-1) { // background bitmap
        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                (const hud_static_element_placement *)&hud->background_anchor_offset, flags,
                                *flash_start_time);
    }
    if (*(datum_index *)((uint8_t *)hud + 0xbc) != (datum_index)-1) { // total grenades background bitmap
        hud_draw_static_element(local_player_index, (uint16_t *)hud,
                                (const hud_static_element_placement *)&hud->total_grenades_background_anchor_offset,
                                flags, *flash_start_time);
    }
    if (hud->total_grenades_numbers_maximum_number_of_digits != 0) {
        int32_t total = unit_get_grenade_count(unit_index, unit_get_current_grenade_index(unit_index));
        hud_draw_number((void *)(int32_t)local_player_index, (uint16_t *)hud,
                        (const hud_number_placement *)&hud->total_grenades_numbers_anchor_offset, (int16_t)total, -1,
                        flags, *flash_start_time, 0.0f);
    }
    if (*(datum_index *)&hud->total_grenades_overlay_bitmap.tag_id != (datum_index)-1) {
        uint16_t types;

        count = *(int8_t *)(unit + 0x31e + *(int8_t *)(unit + 0x31c));
        types = (uint16_t)((count <= hud->flash_cutoff ? 1 : 0) | (count == 0 ? 2 : 0));
        types = types == 0 ? 4 : (uint16_t)(types & 0xfffb);
        hud_draw_overlays((uint16_t *)hud, (const hud_overlay_list *)&hud->total_grenades_overlay_bitmap,
                          (uint32_t)(int16_t)types | 8, *flash_start_time, flags,
                          local_player_globals->unknown_0c > 1);
    }
}

#if 0
Original Ghidra decompilation (0x4b2ac0):

void FUN_004b2ac0(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  int iVar3;
  char cVar4;
  byte bVar5;
  int iVar6;
  undefined4 uVar7;
  undefined4 extraout_EDX;
  undefined2 uVar8;
  undefined2 extraout_var;
  byte bVar9;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_2 & 0xffff) * 0xc);
  cVar4 = FUN_004c2f30();
  if ((((cVar4 == '\0') && (cVar4 = *(char *)(iVar1 + 0x31c), cVar4 != -1)) &&
      ((iVar6 = object_try_and_get(3), iVar6 == 0 ||
       ((*(uint *)(iVar6 + 0x324) != param_2 && (*(uint *)(iVar6 + 0x328) != param_2)))))) &&
     (*(char *)(iVar1 + 0x31c) != -1)) {
    uVar2 = *(uint *)((short)*(char *)(iVar1 + 0x31c) * 0x44 + 0x20 + *(int *)(DAT_00746fa0 + 300));
    iVar6 = DAT_00719430 + (short)param_1 * 0x28;
    if (uVar2 != 0xffffffff) {
      iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      cVar4 = *(char *)(cVar4 + 0x31e + iVar1);
      bVar9 = (short)cVar4 <= *(short *)(iVar3 + 0x148);
      if (cVar4 == '\0') {
        bVar9 = bVar9 | 2;
      }
      if (1 < *(short *)(DAT_0087a478 + 0xc)) {
        bVar9 = bVar9 | 4;
      }
      if ((bVar9 & 1) == 0) {
        *(undefined4 *)(iVar6 + 0x24) = 0xffffffff;
      }
      else if (*(int *)(iVar6 + 0x24) == -1) {
        *(undefined4 *)(iVar6 + 0x24) = *(undefined4 *)(DAT_006f1d6c + 0xc);
      }
      if (*(int *)(iVar3 + 0x54) != -1) {
        FUN_004ac6f0(param_1,iVar3,iVar3 + 0x24,bVar9,*(undefined4 *)(iVar6 + 0x24));
      }
      uVar7 = param_1;
      if (*(int *)(iVar3 + 0xbc) != -1) {
        FUN_004ac6f0(param_1,iVar3,iVar3 + 0x8c,bVar9,*(undefined4 *)(iVar6 + 0x24));
        uVar7 = extraout_EDX;
      }
      uVar8 = (undefined2)((uint)uVar7 >> 0x10);
      if (*(char *)(iVar3 + 0x138) != '\0') {
        unit_get_current_grenade_index(0xffffffff,bVar9,*(undefined4 *)(iVar6 + 0x24),0);
        uVar7 = unit_get_grenade_count();
        FUN_004ac0b0(param_1,iVar3,iVar3 + 0xf4,uVar7);
        uVar8 = extraout_var;
      }
      if (*(int *)(iVar3 + 0x158) != -1) {
        cVar4 = *(char *)(*(char *)(iVar1 + 0x31c) + 0x31e + iVar1);
        bVar5 = (short)cVar4 <= *(short *)(iVar3 + 0x148);
        if (cVar4 == '\0') {
          bVar5 = bVar5 | 2;
        }
        if (bVar5 == 0) {
          bVar5 = 4;
        }
        FUN_004ac950(iVar3,iVar3 + 0x14c,bVar5 | 8,*(undefined4 *)(iVar6 + 0x24),bVar9,
                     CONCAT31((int3)(CONCAT22(uVar8,(short)cVar4) >> 8),
                              1 < *(short *)(DAT_0087a478 + 0xc)));
      }
    }
  }
  return;
}
#endif
