// actor_attempt_grenade_throw  (Ghidra: actor_attempt_grenade_throw, already named)
// address 0x428ab0, size 645 bytes
// name confidence: 0.55   rewrite confidence: 0.2
// evidence: types/ai.h actor.awareness_level(0x6a)/unknown_6e/unit_index(0x18)/
//   grenade_impact_point(0x6a8)/grenade_recheck_ticks(0x6ce)/unknown_378(stance selector)/
//   unknown_60c/unknown_648(inside unknown_63c[16]). types/units.h controlling_player-style
//   flag at unit+0x204 bit 6 (UNSURE, no established name); unit+0x28c (a byte grenade
//   count/timer this shares with actor_died_unit_grenade_count_mod's sibling routine). Calls
//   random_real/random_real_range (0x4019f0/0x401050), unit_get_weapon_object_index
//   (0x569970), actor_died_unit_grenade_count_mod (0x428d35, this rewrite) and
//   actor_delete (0x427e60, both already established), plus unit_set_control_countdown and encounter_recompute_morale,
//   neither established elsewhere in this repo.
//   UNSURE: this is one of the least-confident rewrites in this pass, sharing all the
//   caveats actor_died_unit_grenade_count_mod's header documents about the tail these two
//   functions share. The four Actor-tag floats/int16s at +0x94/+0x1d4/+0x1d8/+0x1dc/+0x1e0/
//   +0x1e2 fall inside that struct's large pad_14c[268] run in types/tags.h and have no
//   individual names; accessed as raw offsets from the tag data pointer.
// register convention: EAX -> actor_index (Ghidra's own "param_1").
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern ai_globals *ai_globals_ptr;  // 0x00880354

extern real random_real(void); // 0x4019f0
extern real random_real_range(real min, real max); // 0x401050
extern datum_index unit_get_weapon_object_index(void); // 0x569970, UNSURE signature
extern void unit_set_control_countdown(int32_t param_a, int32_t param_b); // 0x563b20, UNSURE signature
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940, UNSURE signature, not in this rewrite range
extern void actor_died_unit_grenade_count_mod(object *unit_object, const uint8_t *actor_tag_data,
                                              datum_index weapon_object_index, datum_index actor_index,
                                              datum_index encounter_index); // 0x428d35
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60
extern void weapon_set_loaded_ammo_fraction(float fraction); // 0x4c58c0, UNSURE signature
extern void weapon_set_ammo_counts(int16_t *counts); // 0x4c5820, UNSURE signature

// blam-cc: EAX -> actor_index
// Per-tick decision that rolls a difficulty-scaled random chance for a suitable actor (alert
// enough, has a grenade-holding controlled unit, no active grenade timer) to throw a
// grenade, then always randomizes that unit's ammo/grenade-count housekeeping and finally
// deletes the actor and releases it from its encounter -- this function is a "unit is being
// destroyed" cleanup helper, not a repeatable per-tick check (see actor_delete_or_release_unit,
// which calls it right before deleting the unit's object).
void actor_attempt_grenade_throw(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    datum_index encounter_index = self->encounter_index;
    const uint8_t *actor_tag_data = (const uint8_t *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    object *unit_object;
    datum_index weapon_object_index = (datum_index)k_datum_index_none;

    if (self->awareness_level == 3 && self->unknown_6e > 1) {
        unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;

        if ((*(uint32_t *)((uint8_t *)unit_object + 0x204) & 0x40) != 0) {
            weapon_object_index = unit_get_weapon_object_index();
            if (weapon_object_index != (datum_index)k_datum_index_none &&
                *(int8_t *)((uint8_t *)unit_object + 0x28c) > 0) {
                float chance = *(const float *)(actor_tag_data + 0x94); // UNSURE offset
                if (chance < 0.1f) {
                    chance = 0.1f;
                } else if (chance > 0.6f) {
                    chance = 0.6f;
                }

                if (self->unknown_378 != 0 ||
                    (self->unknown_60c > 0 && *(float *)&self->unknown_63c[0xc] < 3.0f)) { // offset 0x648
                    float boosted = chance * 4.0f;
                    if (boosted > 0.6f) {
                        boosted = 0.6f;
                    }
                    if (chance <= boosted) {
                        chance = boosted;
                    }
                }

                if (random_real() < chance) {
                    int32_t ticks;
                    if (self->grenade_unknown_6c8 == 0.0f) { // UNSURE: reusing a named field for the *0x98 read
                        random_real_range(0.8f, 1.3f);
                    }
                    ticks = (int32_t)0; // UNSURE: __ftol's operand (the just-computed random range) is
                                        // not tracked by Ghidra at this call site; see file header
                    unit_set_control_countdown(ticks, 0x800);
                    *(int8_t *)((uint8_t *)unit_object + 0x28c) = (int8_t)ticks;
                }
            }
        }
    } else {
        unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;

    {
        object *weapon_unit = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
        int16_t slot = *(int16_t *)((uint8_t *)weapon_unit + 0x2f2); // UNSURE offset
        datum_index slot_weapon = (slot != -1) ? *(datum_index *)((uint8_t *)weapon_unit + 0x2f8 + slot * 4)
                                                : (datum_index)k_datum_index_none;

        if (ai_globals_ptr->initialized == 0) {
            *(int16_t *)((uint8_t *)weapon_unit + 0x31e) = 0;
        } else if ((float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f < *(const float *)(actor_tag_data + 0x1d4)) { // UNSURE offset
            actor_died_unit_grenade_count_mod(weapon_unit, actor_tag_data, weapon_object_index, actor_index, encounter_index);
            return;
        }

        if (slot_weapon != (datum_index)k_datum_index_none) {
            float min_fraction = *(const float *)(actor_tag_data + 0x1d8); // UNSURE offset
            float max_fraction = *(const float *)(actor_tag_data + 0x1dc); // UNSURE offset

            if (min_fraction > 0.0f || max_fraction > 0.0f) {
                random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                weapon_set_loaded_ammo_fraction(
                    (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f * (max_fraction - min_fraction) + min_fraction);
            }

            {
                int16_t min_count = *(const int16_t *)(actor_tag_data + 0x1e0); // UNSURE offset
                int16_t max_count = *(const int16_t *)(actor_tag_data + 0x1e2); // UNSURE offset

                if (min_count > 0 || max_count > 0) {
                    int16_t count;
                    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                    count = (int16_t)((((int32_t)(int16_t)(max_count + 1) - min_count) * (int32_t)(random_seed_global >> 0x10)) >> 0x10) + min_count;
                    weapon_set_ammo_counts(&count);
                }
            }
        }
    }

    actor_delete(actor_index, 1);
    if (encounter_index != (datum_index)k_datum_index_none) {
        encounter_recompute_morale(encounter_index);
    }
}

#if 0
Original Ghidra decompilation (0x428ab0):

void actor_attempt_grenade_throw(uint param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float fVar7;
  float local_c;
  uint local_8;
  int local_4;

  iVar2 = DAT_008603b0;
  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar5 = (param_1 & 0xffff) * 0x724;
  local_4 = *(int *)(iVar5 + 0x34 + iVar1);
  iVar6 = iVar5 + iVar1;
  iVar1 = *(int *)((*(uint *)(iVar5 + 0x5c + iVar1) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((((*(short *)(iVar6 + 0x6a) == 3) && (1 < *(short *)(iVar6 + 0x6e))) &&
      (iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc
                       ), (*(uint *)(iVar5 + 0x204) >> 6 & 1) != 0)) &&
     ((local_8 = iVar5, iVar4 = unit_get_weapon_object_index(), iVar4 != -1 &&
      ('\0' < *(char *)(iVar5 + 0x28c))))) {
    if (0.1 <= *(float *)(iVar1 + 0x94)) {
      if (*(float *)(iVar1 + 0x94) <= 0.6) {
        local_c = *(float *)(iVar1 + 0x94);
      }
      else {
        local_c = 0.6;
      }
    }
    else {
      local_c = 0.1;
    }
    if ((*(char *)(iVar6 + 0x378) != '\0') ||
       ((0 < *(short *)(iVar6 + 0x60c) && (*(float *)(iVar6 + 0x648) < 3.0)))) {
      fVar7 = local_c * 4.0;
      if (0.6 < fVar7) {
        fVar7 = 0.6;
      }
      if (local_c <= fVar7) {
        local_c = fVar7;
      }
    }
    fVar7 = random_real();
    if (fVar7 < local_c) {
      if (*(float *)(iVar1 + 0x98) == 0.0) {
        random_real_range(0.8,1.3);
      }
      sVar3 = __ftol();
      FUN_00563b20((int)sVar3,0x800);
      *(char *)(local_8 + 0x28c) = (char)sVar3;
    }
  }
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  local_8 = random_seed_global >> 0x10;
  iVar5 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc);
  sVar3 = *(short *)(iVar5 + 0x2f2);
  iVar4 = -1;
  if (sVar3 != -1) {
    iVar4 = *(int *)(iVar5 + 0x2f8 + sVar3 * 4);
  }
  if (*(char *)(DAT_00880354 + 0x3b4) == '\0') {
    *(undefined2 *)
     (*(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar6 + 0x18) & 0xffff) * 0xc) + 0x31e) = 0;
  }
  else if ((float)local_8 * 1.5259022e-05 < *(float *)(iVar1 + 0x1d4)) {
    actor_died_unit_grenade_count_mod();
    return;
  }
  if (iVar4 != -1) {
    if ((0.0 < *(float *)(iVar1 + 0x1d8)) || (0.0 < *(float *)(iVar1 + 0x1dc))) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      local_8 = random_seed_global >> 0x10;
      weapon_set_loaded_ammo_fraction
                ((float)local_8 * 1.5259022e-05 *
                 (*(float *)(iVar1 + 0x1dc) - *(float *)(iVar1 + 0x1d8)) + *(float *)(iVar1 + 0x1d8)
                );
    }
    sVar3 = *(short *)(iVar1 + 0x1e0);
    if ((0 < sVar3) || (0 < *(short *)(iVar1 + 0x1e2))) {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
      local_c = (float)(uint)(ushort)((short)(((int)(short)(*(short *)(iVar1 + 0x1e2) + 1) -
                                              (int)sVar3) * (random_seed_global >> 0x10) >> 0x10) +
                                     sVar3);
      weapon_set_ammo_counts(&local_c);
    }
  }
  actor_delete(1);
  if (local_4 != -1) {
    FUN_00437940(local_4);
  }
  return;
}
#endif
