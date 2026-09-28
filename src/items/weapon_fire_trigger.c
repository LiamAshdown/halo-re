// weapon_fire_trigger  (Ghidra: weapon_fire_trigger, already named)
// address 0x4c3f10, size 2226 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (second largest function in this batch)
// evidence: types/items.h weapon_data (heat/age/charged_fraction/alternate_shots_loaded/state,
//   triggers[]), weapon_trigger_state (all fields), weapon_magazine_state; types/tags.h
//   WeaponTrigger (magazine 0x20, rounds_per_shot 0x22, minimum_rounds_loaded 0x24,
//   heat_generated_per_round 0xb8, age_generated_per_round 0xbc, firing_effects 0x108,
//   projectile.tag_id 0xa0), WeaponTriggerFiringEffect (shot_count bounds, size 0x84,
//   {firing,misfire,empty}_damage.tag_id at +0x60/+0x70/+0x80), WeaponMagazineFlags (bit 1 =
//   every_round_must_be_chambered), Weapon (age_misfire_start 0x448, age_misfire_chance 0x44c,
//   heat_detonation_threshold 0x354, heat_detonation_fraction 0x358, secondary_trigger_mode,
//   weapon_type); types/objects.h object.bounding_center (0x0a0), damage_data (every field of
//   the self-damage/knockback record below matches its layout exactly). Every raw offset was
//   confirmed against types/items.h and types/tags.h with an offsetof probe.
// register convention: item index and trigger index are both Ghidra-recognized parameters.
// blam-cc: stack -> (item_index, trigger_index)
// UNSURE: the self-damage/knockback block (object_apply_damage on the holder, local_78) reads
// three floats off the HOLDER object at the same byte offsets weapon_data uses for
// heat/age/charged_fraction (0x23c/0x240/0x244) -- since the holder is a unit, not a weapon,
// those offsets fall inside types/objects.h's still-unresolved unknown_022[0x3a] region rather
// than any named unit field, and are kept as raw offsets rather than invented names. The final
// weapon_play_trigger_tag_effect(local_74, local_7c) call does not obviously match that
// function's (item_index, tag_id, slot, sub_index) signature established elsewhere in this
// module; local_74/local_7c are passed through positionally with a literal 0 sub_index, but
// this mapping is not verified.
// reconciled: R25 damage_data.unknown_4c -> material_type (int16 collision material of the damaged surface, 0xffff = none; indexes DamageEffect +0x200)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "items.h"
#include "game.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint8_t weapon_infinite_ammo;              // 0x0087abc9
extern random_seed random_seed_global;            // 0x00719cd0
extern int16_t network_game_mode;                 // 0x00719720
extern game_engine_definition *current_game_engine;
extern uint8_t weapon_bottomless_clip;             // 0x0087abc2
extern uint8_t weapon_client_side_projectiles;     // 0x006894c0
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void); // 0x4019f0, math module
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0
extern datum_index player_index_from_unit_index(datum_index unit_index); // 0x474db0, stack
extern void unit_update_active_camouflage_depower(datum_index player_handle); // 0x466420, EBX
extern uint32_t local_player_index_for_weapon(datum_index item_index); // 0x494010, outside this module, UNSURE signature
extern void first_person_weapon_process_action(uint32_t handle, int32_t action); // 0x4940f0
extern void hud_play_pickup_notification(uint32_t object_or_slot_index, int16_t item_type_code); // 0x492990, EBX, EAX
extern int32_t weapon_set_state(datum_index item_index, int16_t new_state, int8_t force); // 0x4c5670
extern void trigger_create_projectiles(datum_index item_index, int16_t trigger_index, int32_t role); // 0x4c4c40
extern void ai_refresh_unit_stimulus_and_alert(datum_index object_index, int16_t priority,
    int16_t stimulus_value); // 0x42c2a0, EDX, BX, DI (post-fire cue)
extern void object_apply_damage(damage_data *dd, uint32_t target_object_index, int16_t node_index,
    int16_t region_index, int16_t material_index, uint32_t plane); // 0x4ee5e0
extern void weapon_reload_recovery_finish(datum_index item_index, int16_t trigger_index); // 0x4c4940
extern void weapon_trigger_finish_shot(datum_index item_index, int16_t trigger_index); // 0x4c48f0
extern uint32_t weapon_play_trigger_tag_effect(datum_index item_index, datum_index tag_id, real scale_a,
    real scale_b); // 0x4c47d0, ECX item, EDI tag, stack (scale_a, scale_b)

// Fires one round of a weapon trigger: consumes ammo (or misfires), resolves the firing effect
// and heat/age gain, spawns projectiles (or defers to the host), applies a self-damage/knockback
// impulse to the holder while overheated, and updates the trigger's effect state.
uint32_t weapon_fire_trigger(datum_index item_index, int16_t trigger_index)
{
    object *item_obj;
    weapon_data *wd;
    item_data *id;
    Weapon *weapon_tag;
    WeaponTrigger *tag_trigger;
    weapon_trigger_state *trigger;
    datum_index holder_index;
    datum_index selected_damage_tag;
    real misfire_chance;
    int32_t is_alternate_shot;
    int32_t has_ammo;
    int32_t is_misfire;
    int32_t effect_variant; // 0 = fire, 1 = misfire, 2 = empty
    datum_index selected_effect_tag;  // [esp+0x34] firing / misfire / empty EFFECT of the chosen firing effect
    real effect_scale_a;              // [esp+0x30] trigger firing_rate (1.0 when empty)
    real effect_scale_b;              // [esp+0x28] heat / overheated_threshold (0 on misfire or empty)

    item_obj = ((object_header *)object_data->data)[(uint16_t)item_index].data;
    wd = (weapon_data *)((uint8_t *)item_obj + k_item_extension_offset);
    id = (item_data *)((uint8_t *)item_obj + k_item_data_offset);
    weapon_tag = (Weapon *)tag_instances[(uint16_t)item_obj->definition_tag].data;
    tag_trigger = (WeaponTrigger *)weapon_tag->triggers.pointer + trigger_index;
    trigger = &wd->triggers[trigger_index];

    holder_index = (datum_index)0xffffffff;
    if (item_obj->parent_object != (datum_index)0xffffffff &&
        object_try_and_get(item_obj->parent_object, _object_mask_unit) != 0) {
        holder_index = item_obj->parent_object;
    }

    selected_damage_tag = (datum_index)0xffffffff;
    selected_effect_tag = (datum_index)0xffffffff;
    effect_scale_a = 0.0f;
    effect_scale_b = 0.0f;
    misfire_chance = 0.0f;
    is_alternate_shot = 0;
    has_ammo = 0;
    is_misfire = 0;

    if (trigger_index == 1 && (weapon_tag->secondary_trigger_mode == 3 || weapon_tag->secondary_trigger_mode == 4)) {
        is_alternate_shot = 1;
    }

    if (tag_trigger->magazine == (uint16_t)-1) {
        has_ammo = 1;
    } else {
        int16_t magazine_index = tag_trigger->magazine;
        weapon_magazine_state *magazine = &wd->magazines[magazine_index];

        if (!is_alternate_shot || wd->alternate_shots_loaded < weapon_tag->maximum_alternate_shots_loaded) {
            int16_t rounds_loaded = magazine->rounds_loaded;

            if ((tag_trigger->rounds_per_shot <= rounds_loaded || (tag_trigger->flags & 4) != 0) &&
                ((weapon_tag->weapon_flags & 0x800) == 0 || wd->age < 1.0f) &&
                (tag_trigger->minimum_rounds_loaded <= rounds_loaded || (trigger->flags & _weapon_trigger_not_pulled_bit) == 0)) {
                // 0x4c4080..0x4c40b8: the chamber check runs whenever rounds REMAIN after the shot (and with infinite
                // ammo); an emptied magazine skips it. FIXED 2026-09-27: the draft ran it only with infinite ammo.
                uint8_t emptied = 0;

                if (weapon_infinite_ammo == 0) {
                    rounds_loaded = (int16_t)(rounds_loaded - tag_trigger->rounds_per_shot);
                    magazine->rounds_loaded = rounds_loaded;
                    if (rounds_loaded < 1) {
                        magazine->rounds_loaded = 0;
                        emptied = 1;
                    }
                }
                if (!emptied) {
                    WeaponMagazine *magazine_tag = (WeaponMagazine *)weapon_tag->magazines.pointer + magazine_index;
                    if (magazine_tag->flags & 2) { // every_round_must_be_chambered
                        magazine->state = _weapon_magazine_chamber_pending;
                        magazine->state_ticks = 0;
                    }
                }
                has_ammo = 1;
            }
        }
    }

    if (weapon_infinite_ammo != 0) {
        has_ammo = 1;
    }

    if (tag_trigger->firing_effects.count > 0) {
        WeaponTriggerFiringEffect *effects = (WeaponTriggerFiringEffect *)tag_trigger->firing_effects.pointer;

        if (trigger->firing_effect_rounds < 1) {
            uint16_t start_index = trigger->firing_effect_index;
            uint16_t chosen_index = start_index;

            if (tag_trigger->flags & 2) { // random_firing_effects
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                chosen_index = (uint16_t)((random_seed_global >> 0x10) % (uint32_t)tag_trigger->firing_effects.count);
            }
            do {
                uint16_t used_mask;
                int16_t lower, upper, rounds;

                if (trigger->firing_effect_used_mask == (1u << tag_trigger->firing_effects.count) - 1u) {
                    trigger->firing_effect_used_mask = 0;
                }
                used_mask = trigger->firing_effect_used_mask;
                do {
                    chosen_index = chosen_index + 1;
                    if (chosen_index >= tag_trigger->firing_effects.count) {
                        chosen_index = 0;
                    }
                } while ((used_mask & (1u << chosen_index)) != 0);

                trigger->firing_effect_index = chosen_index;
                trigger->firing_effect_used_mask = used_mask | (1u << chosen_index);

                lower = effects[chosen_index].shot_count_lower_bound;
                upper = effects[chosen_index].shot_count_upper_bound;
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                rounds = lower + (int16_t)(((int32_t)(upper - lower) * (int32_t)(random_seed_global >> 0x10)) >> 0x10);
                trigger->firing_effect_rounds = rounds;
            } while (trigger->firing_effect_rounds < 1 && chosen_index != start_index);
        }

        {
            int16_t chosen_index = trigger->firing_effect_index;

            trigger->firing_effect_rounds = trigger->firing_effect_rounds - 1;

            if (weapon_tag->age_misfire_start > 0.0f && weapon_tag->age_misfire_start < 1.0f &&
                weapon_tag->age_misfire_start < wd->age) {
                misfire_chance = ((wd->age - weapon_tag->age_misfire_start) * weapon_tag->age_misfire_chance) /
                                  (1.0f - weapon_tag->age_misfire_start);
                if (trigger->effect_state == _weapon_trigger_effect_spewing) {
                    misfire_chance = misfire_chance + misfire_chance;
                }
                if (random_real() < misfire_chance) {
                    is_misfire = 1;
                }
            }

            // 0x4c429d..0x4c4324: variant 0 fire / 1 misfire / 2 empty picks the effect tag (+0x30 + 0x10 * v) and the
            // damage tag (+0x60 + 0x10 * v); the effect is scaled by (firing_rate or 1.0 when empty, heat fraction).
            if (has_ammo) {
                effect_scale_a = trigger->firing_rate;
                if (is_misfire) {
                    effect_variant = 1;
                    effect_scale_b = 0.0f;
                } else {
                    effect_variant = 0;
                    effect_scale_b = (weapon_tag->overheated_threshold == 0.0f) ? 0.0f
                        : wd->heat / weapon_tag->overheated_threshold;
                }
            } else {
                effect_variant = 2;
                effect_scale_a = 1.0f;
                effect_scale_b = 0.0f;
            }
            selected_effect_tag = *(datum_index *)((uint8_t *)&effects[chosen_index] + (effect_variant + 3) * 0x10);
            selected_damage_tag = *(datum_index *)((uint8_t *)&effects[chosen_index] + (effect_variant + 6) * 0x10);
        }
    }

    if (!has_ammo) {
        goto tail;
    }

    if ((id->flags & _item_held_by_player_bit) != 0 && current_game_engine != 0) { // 0x4c433d: 0x6f1d20
        // 0x4c4346: the firing player's camouflage drops
        datum_index player = player_index_from_unit_index(holder_index);

        if (player != (datum_index)0xffffffff) {
            unit_update_active_camouflage_depower(player);
        }
    }

    wd->last_fire_game_time = game_time->game_time;

    {
        int8_t action = is_misfire ? (int8_t)((trigger_index != 0) + 2) : (int8_t)(trigger_index != 0);
        uint32_t action_handle = local_player_index_for_weapon(item_index);
        first_person_weapon_process_action(action_handle, action);
        if ((int16_t)action_handle == -1) {
            hud_play_pickup_notification(item_index, (int16_t)action); // 0x4c43c3: EBX weapon, EAX action
        }
    }

    if (tag_trigger->ejection_port_recovery_time > 0.0f && (*(uint8_t *)&tag_trigger->flags & 0x80) == 0) { // 0x4c441e
        trigger->ejection_port_recovery = 1.0f;
    }
    if (tag_trigger->illumination_recovery_time > 0.0f) {
        trigger->illumination_recovery = 1.0f;
    }

    if (weapon_infinite_ammo == 0) {
        wd->heat = wd->heat + tag_trigger->heat_generated_per_round;
    }
    if ((id->flags & _item_held_by_player_bit) != 0 || wd->heat <= weapon_tag->overheated_threshold) {
        if (wd->heat > 1.0f) {
            wd->heat = 1.0f;
        }
    } else {
        wd->heat = weapon_tag->overheated_threshold;
    }

    if ((id->flags & _item_held_by_player_bit) != 0 && weapon_bottomless_clip == 0) {
        real new_age = wd->age + tag_trigger->age_generated_per_round;
        wd->age = new_age;
        if (new_age > 1.0f) {
            wd->age = 1.0f;
            item_obj->flags = item_obj->flags | _object_changed_bit;
        }
    }

    weapon_set_state(item_index, (trigger_index != 0) + 1, 0);

    if (!is_misfire) {
        if (is_alternate_shot) {
            wd->alternate_shots_loaded = wd->alternate_shots_loaded + 1;
        } else {
            int32_t role;
            int32_t create_locally = 1;

            if (network_game_mode == 1) {
                if ((tag_trigger->flags & 0x2000) != 0 && weapon_client_side_projectiles == 1) {
                    role = 3;
                } else {
                    create_locally = 0;
                    role = 0;
                }
            } else if (network_game_mode == 2 && ((tag_trigger->flags & 0x2000) == 0 || weapon_client_side_projectiles != 1)) {
                role = 0;
            } else {
                role = 3;
            }
            if (create_locally) {
                trigger_create_projectiles(item_index, trigger_index, role);
            }
            // 0x4c456a: EDX holder, BX the trigger's +0x2e word, DI 1
            ai_refresh_unit_stimulus_and_alert(holder_index, *(int16_t *)((uint8_t *)tag_trigger + 0x2e), 1);
        }
    }

    if (holder_index != (datum_index)0xffffffff && selected_damage_tag != (datum_index)0xffffffff) {
        // UNSURE: see file header -- 0x23c/0x240/0x244 on the holder are not a named unit field.
        object *holder_obj = ((object_header *)object_data->data)[(uint16_t)holder_index].data;
        uint8_t *holder_bytes = (uint8_t *)holder_obj;
        damage_data dd;
        int32_t *zero = (int32_t *)&dd;
        int32_t i;

        for (i = 0; i < (int32_t)(sizeof(dd) / sizeof(int32_t)); i++) {
            zero[i] = 0;
        }
        dd.damage_effect_tag = selected_damage_tag;
        dd.flags = dd.flags | 8;
        dd.responsible_player = (datum_index)0xffffffff;
        dd.responsible_object = (datum_index)0xffffffff;
        dd.team_index = -1;
        dd.material_type = -1;
        dd.unknown_1a = -1; // UNSURE: matches local_44 (0xffff) at damage_data+0x18 (location_cluster_index)
        dd.location_cluster_index = -1;
        dd.random_blend = 1.0f; // 0x4c45ed / 0x4c45f8: the draft left both at 0 (no damage)
        dd.multiplier = 1.0f;
        dd.direction.i = -*(real *)(holder_bytes + 0x23c);
        dd.direction.j = -*(real *)(holder_bytes + 0x240);
        dd.direction.k = -*(real *)(holder_bytes + 0x244);
        dd.epicentre = holder_obj->bounding_center;
        dd.origin = holder_obj->bounding_center;
        object_apply_damage(&dd, holder_index, -1, -1, -1, 0);
    }

    if (weapon_tag->weapon_type == 3 && trigger_index == 1) {
        wd->flags = wd->flags | 4; // _weapon_alternate_shot_armed_bit
    }

tail:
    if (weapon_tag->heat_detonation_threshold < wd->heat) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        if ((real)(random_seed_global >> 0x10) * 1.5259022e-05f < weapon_tag->heat_detonation_fraction) {
            weapon_reload_recovery_finish(item_index, trigger_index);
        }
    }

    if (has_ammo) {
        if (trigger->effect_state != _weapon_trigger_effect_spewing || is_misfire) {
            if ((tag_trigger->flags & 1) == 0) { // tracks_fired_projectile
                weapon_trigger_finish_shot(item_index, trigger_index);
            } else {
                trigger->effect_state = _weapon_trigger_effect_tracking;
                trigger->effect_state_ticks = -1;
            }
        }
    } else {
        trigger->effect_state = _weapon_trigger_effect_out_of_ammo;
        trigger->effect_state_ticks = -1;
    }

    trigger->flags = trigger->flags & ~(uint32_t)_weapon_trigger_not_pulled_bit;
    // 0x4c479e..0x4c47af: ECX item, EDI = the chosen firing-effect EFFECT tag, stack (firing_rate, heat fraction).
    // FIXED 2026-09-27: the draft passed the damage tag and the misfire chance, so the muzzle effect was never played.
    return weapon_play_trigger_tag_effect(item_index, selected_effect_tag, effect_scale_a, effect_scale_b);
}

#if 0
Original Ghidra decompilation (0x4c3f10):

void weapon_fire_trigger(uint param_1,undefined4 param_2)

{
  float fVar1;
  ushort uVar2;
  uint *puVar3;
  int iVar4;
  uint *puVar5;
  bool bVar6;
  bool bVar7;
  bool bVar8;
  uint uVar9;
  short sVar10;
  int iVar11;
  int iVar12;
  int iVar13;
  int iVar14;
  short *psVar15;
  undefined4 uVar16;
  short sVar17;
  ushort uVar18;
  uint uVar19;
  int iVar20;
  uint *puVar21;
  int *piVar22;
  char cVar23;
  float fVar24;
  float local_84;
  short local_80;
  float local_7c;
  uint local_78;
  uint local_74;
  int local_5c [4];
  undefined2 local_4c;
  undefined2 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  float local_28;
  float local_24;
  float local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined2 local_10;

  iVar11 = (param_1 & 0xffff) * 0xc;
  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
  uVar19 = puVar3[0x47];
  iVar12 = (int)(short)param_2;
  iVar13 = iVar12 * 0x114;
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar20 = iVar12 * 0x28;
  puVar21 = (uint *)(*(int *)(iVar4 + 0x500) + iVar13);
  local_78 = 0xffffffff;
  if ((uVar19 != 0xffffffff) && (iVar14 = object_try_and_get(3), iVar14 != 0)) {
    local_78 = uVar19;
  }
  local_84 = -NAN;
  local_7c = 0.0;
  local_74 = 0;
  bVar7 = false;
  bVar8 = false;
  bVar6 = false;
  if (((short)param_2 == 1) &&
     ((*(short *)(iVar4 + 0x32c) == 3 || (*(short *)(iVar4 + 0x32c) == 4)))) {
    bVar6 = true;
  }
  if ((short)puVar21[8] == -1) {
LAB_004c40b8:
    bVar7 = true;
  }
  else {
    uVar19 = puVar21[8];
    iVar14 = *(int *)(iVar4 + 0x4f4);
    puVar5 = puVar3 + (short)uVar19 * 3 + 0xac;
    if ((!bVar6) || ((short)puVar3[0x97] < *(short *)(iVar4 + 0x32e))) {
      sVar10 = (short)puVar5[2];
      if ((((*(short *)((int)puVar21 + 0x22) <= sVar10) || ((*puVar21 & 4) != 0)) &&
          (((*(uint *)(iVar4 + 0x308) & 0x800) == 0 || ((float)puVar3[0x90] < 1.0)))) &&
         (((short)puVar21[9] <= sVar10 || ((puVar3[iVar12 * 10 + 0x99] & 1) == 0)))) {
        if ((DAT_0087abc9 == '\0') &&
           (sVar10 = sVar10 - *(short *)((int)puVar21 + 0x22), *(short *)(puVar5 + 2) = sVar10,
           sVar10 < 1)) {
          *(undefined2 *)(puVar5 + 2) = 0;
        }
        else if ((*(byte *)((short)uVar19 * 0x70 + iVar14) & 2) != 0) {
          *(undefined2 *)puVar5 = 2;
          *(undefined2 *)((int)puVar5 + 2) = 0;
        }
        goto LAB_004c40b8;
      }
    }
  }
  if (DAT_0087abc9 != '\0') {
    bVar7 = true;
  }
  if (0 < (int)puVar21[0x42]) {
    if ((short)puVar3[iVar12 * 10 + 0x9b] < 1) {
      uVar2 = *(ushort *)(iVar20 + 0x26a + (int)puVar3);
      uVar19 = (uint)uVar2;
      if ((*puVar21 & 2) != 0) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        uVar19 = (int)(random_seed_global >> 0x10) % (int)puVar21[0x42];
      }
      do {
        if ((uint)(ushort)puVar3[iVar12 * 10 + 0x9a] == (1 << ((byte)puVar21[0x42] & 0x1f)) - 1U) {
          *(undefined2 *)(puVar3 + iVar12 * 10 + 0x9a) = 0;
        }
        uVar9 = puVar3[iVar12 * 10 + 0x9a];
        do {
          uVar19 = uVar19 + 1;
          if ((int)puVar21[0x42] <= (int)(short)uVar19) {
            uVar19 = 0;
          }
        } while (((uint)(ushort)uVar9 & 1 << ((byte)uVar19 & 0x1f)) != 0);
        uVar18 = (ushort)uVar19;
        psVar15 = (short *)((short)uVar18 * 0x84 + puVar21[0x43]);
        *(ushort *)(iVar20 + 0x26a + (int)puVar3) = uVar18;
        *(ushort *)(puVar3 + iVar12 * 10 + 0x9a) =
             (ushort)(1 << ((byte)uVar19 & 0x1f)) | (ushort)uVar9;
        sVar10 = *psVar15;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        sVar10 = sVar10 + (short)(((int)psVar15[1] - (int)sVar10) * (random_seed_global >> 0x10) >>
                                 0x10);
        *(short *)(puVar3 + iVar12 * 10 + 0x9b) = sVar10;
      } while ((sVar10 < 1) && (uVar18 != uVar2));
    }
    sVar10 = *(short *)(iVar20 + 0x26a + (int)puVar3);
    *(short *)(puVar3 + iVar12 * 10 + 0x9b) = (short)puVar3[iVar12 * 10 + 0x9b] + -1;
    uVar19 = puVar21[0x43];
    if ((0.0 < *(float *)(iVar4 + 0x448)) &&
       ((*(float *)(iVar4 + 0x448) < 1.0 && (*(float *)(iVar4 + 0x448) < (float)puVar3[0x90])))) {
      local_84 = (((float)puVar3[0x90] - *(float *)(iVar4 + 0x448)) * *(float *)(iVar4 + 0x44c)) /
                 (1.0 - *(float *)(iVar4 + 0x448));
      if (*(char *)(iVar20 + 0x261 + (int)puVar3) == '\x06') {
        local_84 = local_84 + local_84;
      }
      fVar24 = random_real();
      if (fVar24 < local_84) {
        bVar8 = true;
      }
    }
    if (bVar7) {
      local_74 = puVar3[iVar12 * 10 + 0x9c];
      if (bVar8) {
        sVar17 = 1;
        local_7c = 0.0;
      }
      else {
        sVar17 = 0;
        if (*(float *)(iVar4 + 0x350) == 0.0) {
          local_7c = 0.0;
        }
        else {
          local_7c = (float)puVar3[0x8f] / *(float *)(iVar4 + 0x350);
        }
      }
    }
    else {
      sVar17 = 2;
      local_74 = 0x3f800000;
      local_7c = 0.0;
    }
    local_84 = *(float *)((sVar17 + 6) * 0x10 + sVar10 * 0x84 + uVar19);
  }
  if (!bVar7) goto LAB_004c46a4;
  if ((((puVar3[0x7d] & 2) != 0) && (DAT_006f1d20 != 0)) &&
     (iVar14 = FUN_00474db0(local_78), iVar14 != -1)) {
    FUN_00466420();
  }
  puVar3[0xb4] = *(uint *)(DAT_006f1d6c + 0xc);
  if (bVar8) {
    cVar23 = ((short)param_2 != 0) + '\x02';
  }
  else {
    cVar23 = (short)param_2 != 0;
  }
  uVar16 = FUN_00494010(param_1);
  first_person_weapon_process_action(uVar16,cVar23);
  local_80 = (short)uVar16;
  if (local_80 == -1) {
    FUN_00492990();
  }
  puVar5 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
  iVar14 = *(int *)(*(int *)((*puVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x500);
  if ((0.0 < *(float *)(iVar14 + 0xa4 + iVar13)) && (-1 < *(char *)(iVar14 + iVar13))) {
    puVar5[iVar12 * 10 + 0x9d] = 0x3f800000;
  }
  if (0.0 < (float)puVar21[0x2a]) {
    puVar3[iVar12 * 10 + 0x9e] = 0x3f800000;
  }
  if (DAT_0087abc9 == '\0') {
    puVar3[0x8f] = (uint)((float)puVar21[0x2e] + (float)puVar3[0x8f]);
  }
  if (((puVar3[0x7d] & 2) != 0) || ((float)puVar3[0x8f] <= *(float *)(iVar4 + 0x350))) {
    if (1.0 < (float)puVar3[0x8f]) {
      puVar3[0x8f] = 0x3f800000;
    }
  }
  else {
    puVar3[0x8f] = *(uint *)(iVar4 + 0x350);
  }
  if ((((puVar3[0x7d] & 2) != 0) && (DAT_0087abc2 == '\0')) &&
     (fVar24 = (float)puVar21[0x2f], fVar1 = (float)puVar3[0x90],
     puVar3[0x90] = (uint)(fVar24 + fVar1), 1.0 < fVar24 + fVar1)) {
    puVar3[0x90] = 0x3f800000;
    puVar3[4] = puVar3[4] | 0x4000000;
  }
  weapon_set_state(((short)param_2 != 0) + '\x01',0);
  if (!bVar8) {
    if (bVar6) {
      *(short *)(puVar3 + 0x97) = (short)puVar3[0x97] + 1;
    }
    else {
      if (DAT_00719720 == 1) {
        if (((*puVar21 & 0x2000) != 0) && (DAT_006894c0 == '\x01')) goto LAB_004c4553;
      }
      else {
        if ((DAT_00719720 == 2) && (((*puVar21 & 0x2000) == 0) || (DAT_006894c0 != '\x01'))) {
          uVar16 = 0;
        }
        else {
LAB_004c4553:
          uVar16 = 3;
        }
        trigger_create_projectiles(param_1,param_2,uVar16);
      }
      FUN_0042c2a0();
    }
  }
  if ((local_78 != 0xffffffff) && (local_84 != -NAN)) {
    iVar13 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (local_78 & 0xffff) * 0xc);
    piVar22 = local_5c;
    for (iVar14 = 0x15; iVar14 != 0; iVar14 = iVar14 + -1) {
      *piVar22 = 0;
      piVar22 = piVar22 + 1;
    }
    local_5c[0] = (int)local_84;
    local_10 = 0xffff;
    local_5c[2] = 0xffffffff;
    local_5c[3] = 0xffffffff;
    local_4c = 0xffff;
    local_44 = 0xffff;
    local_1c = 0x3f800000;
    local_18 = 0x3f800000;
    local_5c[1] = local_5c[1] | 8;
    local_28 = -*(float *)(iVar13 + 0x23c);
    local_24 = -*(float *)(iVar13 + 0x240);
    local_20 = -*(float *)(iVar13 + 0x244);
    local_40 = *(undefined4 *)(iVar13 + 0xa0);
    local_3c = *(undefined4 *)(iVar13 + 0xa4);
    local_38 = *(undefined4 *)(iVar13 + 0xa8);
    local_34 = local_40;
    local_30 = local_3c;
    local_2c = local_38;
    object_apply_damage(local_5c,local_78,0xffffffff,0xffffffff,0xffffffff,0);
  }
  if ((*(short *)(iVar4 + 0x4e2) == 3) && ((short)param_2 == 1)) {
    puVar3[0x8b] = puVar3[0x8b] | 4;
  }
LAB_004c46a4:
  if ((*(float *)(iVar4 + 0x354) < (float)puVar3[0x8f]) &&
     (random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f,
     (float)(random_seed_global >> 0x10) * 1.5259022e-05 < *(float *)(iVar4 + 0x358))) {
    FUN_004c4940();
  }
  if (bVar7) {
    if ((*(char *)(iVar20 + 0x261 + (int)puVar3) != '\x06') || (bVar8)) {
      if ((*puVar21 & 1) == 0) {
        FUN_004c48f0(param_2);
      }
      else {
        iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
        *(undefined1 *)(iVar20 + 0x261 + iVar4) = 5;
        *(undefined2 *)(iVar20 + 0x262 + iVar4) = 0xffff;
      }
    }
  }
  else {
    iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
    *(undefined1 *)(iVar20 + 0x261 + iVar4) = 7;
    *(undefined2 *)(iVar20 + 0x262 + iVar4) = 0xffff;
  }
  puVar3[iVar12 * 10 + 0x99] = puVar3[iVar12 * 10 + 0x99] & 0xfffffffe;
  weapon_play_trigger_tag_effect(local_74,local_7c);
  return;
}
#endif
