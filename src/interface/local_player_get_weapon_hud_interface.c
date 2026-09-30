// local_player_get_weapon_hud_interface  (Ghidra: FUN_00494560, unnamed; renamed in the phase 4
// review from first_person_weapon_get_zoom_overlay_object)
// address 0x494560, size 457 bytes
// name confidence: 0.7   rewrite confidence: 0.65
// evidence: the tag id returned is read from weapon tag +0x48c, which is
// Weapon.hud_interface.tag_id (0x480 + 0x0c), and the fallback is hud_globals tag +0x2cc,
// HUDGlobals.default_weapon_hud.tag_id (0x2c0 + 0x0c). Its caller 0x494730 reads the returned
// tag +0xac/+0xb0, WeaponHUDInterface.screen_effect. So this returns the weapon HUD interface
// tag of the current local player's weapon, not a zoom overlay object.
// register convention: out_intensity is the one stack parameter; returns a tag index or -1.
// Review pass (phase 4): rebuilt from the disassembly (0x494560..0x494728).
//  - camera_get_type_for_player takes the local player index in CX.
//  - unit_get_weapon_object_index (0x569970) takes EAX = unit and CX = weapon slot; the first
//    call is the player unit with its own current_weapon_index (+0x2f2). When that is -1 and the
//    unit sits in a seat whose UnitSeat flags have bit 3 set, the second call is the PARENT
//    vehicle with the parent current_weapon_index; the first rewrite lost that distinction.
//  - *out_intensity is unit +0x348 only when the unit holds its own weapon, else 0.
//  - unit_count_deployed_weapons (0x56d990) takes the unit in EAX.
// UNSURE: unit +0x348 is a 0..1 ramp (types/units.h unknown_348); the caller scales the
// connect_to_flashlight screen effects by it, so it is most likely the flashlight intensity.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern int16_t current_local_player_index; // 0x007c3108
extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480, "players"
extern hud_globals_flags *hud_flags;         // 0x00719420
extern data_array *object_data; // 0x008603b0, "objects"
extern tag_instance *tag_instances;          // 0x0087bc14
extern HUDGlobals *hud_globals_tag_data; // 0x0071941c

extern int16_t camera_get_type_for_player(int16_t player_index); // 0x445ac0; blam-cc: CX -> player_index
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970; blam-cc: EAX, CX
extern int16_t unit_count_deployed_weapons(uint32_t unit_index); // 0x56d990; blam-cc: EAX

static object *object_get(datum_index object_index)
{
    return *(object **)((char *)object_data->data + 8 + (object_index & 0xffff) * 0xc);
}

// Returns the weapon_hud_interface tag index for the current local player: the held weapon's,
// else (for a seat with flag bit 3) the parent vehicle's current weapon's, else the hud_globals
// default weapon HUD when the unit has no deployed weapons. Returns -1 when there is no local
// player unit, the HUD is off, or the camera type is 2 or 3. *out_intensity gets unit +0x348
// when the unit holds a weapon of its own, else 0.
int32_t local_player_get_weapon_hud_interface(float *out_intensity)
{
    datum_index player_handle;
    int32_t result = -1;
    float intensity = 0.0f;

    if (current_local_player_index == -1 || current_local_player_index >= 1) {
        player_handle = (datum_index)-1;
    } else {
        player_handle = local_player_globals->local_players[current_local_player_index];
    }

    if (player_handle != (datum_index)-1) {
        player *player_record = (player *)((char *)player_data->data +
                                           (player_handle & 0xffff) * 0x200);
        int16_t camera_type = camera_get_type_for_player(current_local_player_index);

        if (hud_flags != (hud_globals_flags *)0 && hud_flags->hud_enabled != 0 &&
            camera_type != 3 && camera_type != 2 && player_record->unit != (datum_index)-1) {
            datum_index unit_handle = player_record->unit;
            object *unit_obj = object_get(unit_handle);
            datum_index weapon_handle = unit_get_weapon_object_index(
                unit_handle, *(int16_t *)((uint8_t *)unit_obj + 0x2f2));

            if (weapon_handle == (datum_index)-1) {
                datum_index parent_handle = unit_obj->parent_object;
                int16_t seat_index = *(int16_t *)((uint8_t *)unit_obj + 0x2f0);
                object *parent_obj;
                uint8_t *seats;

                if (parent_handle == (datum_index)-1 || seat_index == -1) {
                    goto done;
                }
                parent_obj = object_get(parent_handle);
                seats = (uint8_t *)((Unit *)tag_instances[parent_obj->definition_tag & 0xffff].data)
                            ->seats.pointer;
                if ((seats[seat_index * 0x11c] & 8) == 0) {
                    goto done;
                }
                weapon_handle = unit_get_weapon_object_index(
                    unit_obj->parent_object, *(int16_t *)((uint8_t *)parent_obj + 0x2f2));
            } else {
                intensity = *(float *)((uint8_t *)unit_obj + 0x348);
            }

            if (weapon_handle != (datum_index)-1) {
                Weapon *weapon_tag = (Weapon *)tag_instances[object_get(weapon_handle)->definition_tag & 0xffff].data;
                datum_index hud_interface = *(datum_index *)&weapon_tag->hud_interface.tag_id;
                if (hud_interface != (datum_index)-1) {
                    *out_intensity = intensity;
                    return (int32_t)hud_interface;
                }
                if (unit_count_deployed_weapons(unit_handle) == 0) {
                    result = *(int32_t *)((uint8_t *)hud_globals_tag_data + 0x2cc); // default_weapon_hud.tag_id
                }
            }
        }
    }

done:
    *out_intensity = intensity;
    return result;
}

#if 0
Original Ghidra decompilation (0x494560):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_00494560(undefined4 *param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined4 local_4;

  sVar4 = (short)_DAT_007c3108;
  if ((sVar4 == -1) || (0 < sVar4)) {
    uVar5 = 0xffffffff;
  }
  else {
    uVar5 = *(uint *)(DAT_0087a478 + 4 + sVar4 * 4);
  }
  iVar7 = -1;
  local_4 = 0;
  if (uVar5 != 0xffffffff) {
    iVar1 = *(int *)(DAT_0087a480 + 0x34);
    sVar4 = camera_get_type_for_player();
    if ((((DAT_00719420 != (char *)0x0) && (*DAT_00719420 != '\0')) && (sVar4 != 3)) &&
       ((sVar4 != 2 &&
        (uVar5 = *(uint *)((uVar5 & 0xffff) * 0x200 + iVar1 + 0x34), uVar5 != 0xffffffff)))) {
      iVar1 = *(int *)(DAT_008603b0 + 0x34);
      uVar6 = unit_get_weapon_object_index();
      iVar3 = DAT_0087bc14;
      if (uVar6 == 0xffffffff) {
        iVar2 = *(int *)(iVar1 + 8 + (uVar5 & 0xffff) * 0xc);
        uVar5 = *(uint *)(iVar2 + 0x11c);
        if (((uVar5 == 0xffffffff) || (sVar4 = *(short *)(iVar2 + 0x2f0), sVar4 == -1)) ||
           ((*(byte *)(sVar4 * 0x11c +
                      *(int *)(*(int *)((**(uint **)(iVar1 + 8 + (uVar5 & 0xffff) * 0xc) & 0xffff) *
                                        0x20 + 0x14 + DAT_0087bc14) + 0x2e8)) & 8) == 0))
        goto LAB_0049471a;
        uVar6 = unit_get_weapon_object_index();
      }
      else {
        local_4 = *(undefined4 *)(*(int *)(iVar1 + 8 + (uVar5 & 0xffff) * 0xc) + 0x348);
      }
      if (uVar6 != 0xffffffff) {
        iVar1 = *(int *)(*(int *)((**(uint **)(iVar1 + 8 + (uVar6 & 0xffff) * 0xc) & 0xffff) * 0x20
                                  + 0x14 + iVar3) + 0x48c);
        if (iVar1 != -1) {
          *param_1 = local_4;
          return iVar1;
        }
        sVar4 = FUN_0056d990();
        if (sVar4 == 0) {
          iVar7 = *(int *)(DAT_0071941c + 0x2cc);
        }
      }
    }
  }
LAB_0049471a:
  *param_1 = local_4;
  return iVar7;
}
#endif
