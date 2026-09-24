// unit_apply_starting_profile  (Ghidra: FUN_00473c50; named per this rewrite)
// address 0x473c50, size 278 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: types/tags.h ScenarioPlayerStartingProfile (0x68 bytes: starting_health_modifier
//   +0x20, starting_shield_modifier +0x24, primary_weapon TagDependency +0x28 (tag_id at its own
//   +0x0c, i.e. absolute +0x34), secondary_weapon TagDependency +0x3c (tag_id at absolute +0x48),
//   the four grenade-count bytes at +0x50); Scenario::player_starting_profile (a TagReflexive,
//   count at +0x348, pointer at +0x34c) on global_scenario (0x00746f8c); types/objects.h object
//   (body_vitality +0xe0, shield_vitality +0xe4); types/units.h unit_data (controlling_player
//   +0x218 absolute, grenade_counts[2] +0x31e absolute) and _object_mask_unit (3).
//   out/phase4/game_functions.md guessed "recoil/kick offset into a unit's camera state" (conf
//   0.4); CORRECTED here against every field it actually touches, which are squarely a starting
//   loadout: health/shield modifiers, the two starting weapons, and the two grenade counts.
//   objdump -d -M intel --start-address=0x473c50 --stop-address=0x473d70 bin/halo.exe pins the
//   registers and confirms object_try_and_get is called with the SAME handle this function
//   received in ECX (revalidated through the _object_mask_unit filter), not a different object.
// register convention: EAX -> starting_profile_index, ECX -> unit_handle, stack -> reset_stats.
//   // blam-cc: EAX -> starting_profile_index, ECX -> unit_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern Scenario *global_scenario; // 0x00746f8c

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern void object_delete(datum_index object_index); // 0x4f5bd0, established elsewhere in this module
extern void unit_drop_inventory_weapons_except_current(datum_index unit_handle); // 0x56d360, units module, not in this batch;
    // blam-cc: EAX -> unit_handle; UNSURE full behavior (resets vitality when reset_stats is set)
extern datum_index player_spawn_starting_profile_weapon(TagDependency *weapon_tag, datum_index owner_unit_handle); // 0x477810,
    // this module's next batch; blam-cc: ESI -> weapon_tag, stack -> owner_unit_handle;
    // "Creates a new object attached to an owning unit..." per out/phase4/game_functions.md
extern uint8_t unit_pickup_weapon(datum_index unit_handle, uint8_t is_primary); // 0x56d400, units module,
    // not in this batch; blam-cc: ECX -> unit_handle, stack -> is_primary; UNSURE full behavior
    // (attempts to give/equip the just-created weapon; false means it failed)

// Applies ScenarioPlayerStartingProfile[starting_profile_index] to unit_handle: optionally (when
// reset_stats is set) resets the unit's vitality and zeroes its accumulated shield/health
// modifiers and both grenade counts, then gives it the profile's primary and secondary weapons
// (deleting the created weapon object if unit_pickup_weapon reports it could not be equipped), and
// finally adds the profile's health/shield modifiers and grenade counts into the unit's own.
// No-op if unit_handle or starting_profile_index is the wildcard, or if the (revalidated) unit
// has no controlling player.
void unit_apply_starting_profile(int16_t starting_profile_index, datum_index unit_handle,
                                  uint8_t reset_stats)
    // blam-cc: EAX -> starting_profile_index, ECX -> unit_handle, stack -> reset_stats
{
    object *obj;
    unit_data *unit;
    ScenarioPlayerStartingProfile *profile;
    datum_index weapon_object;
    int8_t *profile_grenade_counts;
    int32_t i;

    if (unit_handle == (datum_index)-1 || starting_profile_index == -1) {
        return;
    }

    obj = object_try_and_get(unit_handle, _object_mask_unit);
    unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    if (unit->controlling_player == (datum_index)-1) {
        return;
    }

    profile = (ScenarioPlayerStartingProfile *)((uint8_t *)global_scenario->player_starting_profile.pointer
                                                 + (uint32_t)(uint16_t)starting_profile_index * sizeof(ScenarioPlayerStartingProfile));

    if (reset_stats != 0) {
        unit_drop_inventory_weapons_except_current(unit_handle);
        obj->shield_vitality = 0.0f;
        obj->body_vitality = 0.0f;
        unit->grenade_counts[0] = 0;
        unit->grenade_counts[1] = 0;
    }

    if (profile->primary_weapon.tag_id.index != 0xffff || profile->primary_weapon.tag_id.id != 0xffff) {
        weapon_object = player_spawn_starting_profile_weapon(&profile->primary_weapon, unit_handle);
        if (weapon_object != (datum_index)-1) {
            if (unit_pickup_weapon(unit_handle, (uint8_t)(reset_stats != 0)) == 0) {
                object_delete(weapon_object);
            }
        }
    }

    if (profile->secondary_weapon.tag_id.index != 0xffff || profile->secondary_weapon.tag_id.id != 0xffff) {
        weapon_object = player_spawn_starting_profile_weapon(&profile->secondary_weapon, unit_handle);
        if (weapon_object != (datum_index)-1) {
            if (unit_pickup_weapon(unit_handle, 0) == 0) {
                object_delete(weapon_object);
            }
        }
    }

    obj->shield_vitality = obj->shield_vitality + profile->starting_shield_modifier;
    obj->body_vitality = obj->body_vitality + profile->starting_health_modifier;
    // The profile's first two grenade-count bytes (fragmentation, plasma) are contiguous with
    // unit::grenade_counts, so this is a plain 2-byte accumulate, exactly matching Ghidra's loop.
    profile_grenade_counts = &profile->starting_fragmentation_grenade_count;
    for (i = 0; i < 2; i = i + 1) {
        unit->grenade_counts[i] = unit->grenade_counts[i] + profile_grenade_counts[i];
    }
}

#if 0
Original Ghidra decompilation (0x473c50), from tools/pack.py 0x473c50:

void FUN_00473c50(char param_1)

{
  char cVar1;
  short in_AX;
  int iVar2;
  int iVar3;
  char *pcVar4;
  int in_ECX;
  char *pcVar5;
  int iVar6;

  if ((in_ECX != -1) && (in_AX != -1)) {
    iVar2 = object_try_and_get(3);
    if (*(int *)(iVar2 + 0x218) != -1) {
      iVar6 = in_AX * 0x68 + *(int *)(global_scenario + 0x34c);
      if (param_1 != '\0') {
        FUN_0056d360();
        *(undefined4 *)(iVar2 + 0xe4) = 0;
        *(undefined4 *)(iVar2 + 0xe0) = 0;
        *(undefined2 *)(iVar2 + 0x31e) = 0;
      }
      if (*(int *)(iVar6 + 0x34) != -1) {
        iVar3 = FUN_00477810();
        if (iVar3 != -1) {
          cVar1 = FUN_0056d400(param_1 != '\0');
          if (cVar1 == '\0') {
            object_delete();
          }
        }
      }
      if (*(int *)(iVar6 + 0x48) != -1) {
        iVar3 = FUN_00477810();
        if (iVar3 != -1) {
          cVar1 = FUN_0056d400(0);
          if (cVar1 == '\0') {
            object_delete();
          }
        }
      }
      pcVar4 = (char *)(iVar2 + 0x31e);
      pcVar5 = (char *)(iVar6 + 0x50);
      iVar3 = 2;
      *(float *)(iVar2 + 0xe4) = *(float *)(iVar6 + 0x24) + *(float *)(iVar2 + 0xe4);
      *(float *)(iVar2 + 0xe0) = *(float *)(iVar6 + 0x20) + *(float *)(iVar2 + 0xe0);
      do {
        cVar1 = *pcVar5;
        pcVar5 = pcVar5 + 1;
        *pcVar4 = *pcVar4 + cVar1;
        pcVar4 = pcVar4 + 1;
        iVar3 = iVar3 + -1;
      } while (iVar3 != 0);
    }
  }
  return;
}
#endif
