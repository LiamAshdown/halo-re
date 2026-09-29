// actor_attempt_grenade_throw  (Ghidra: actor_attempt_grenade_throw; really the actor's death handling)
// address 0x428ab0, size 926 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// REWRITTEN from objdump 0x428ab0..0x428e4d (the whole function; 0x428d35 "actor_died_unit_grenade_count_mod" is
//   its tail, marked FRAGMENT). Stack: actor. A fighting actor (awake 3, grade 2+) whose unit may throw (+0x204 bit
//   6), is armed and still holds grenades (+0x28c) pulls a grenade as it dies with the variant's chance (+0x94,
//   0.1..0.6, raised to 4x up to 0.6 when it was committed or had a target within 3), the throw timed by +0x98
//   (0.8..1.3 s, random when 0) through the control countdown (0x563b20, flag 0x800). Its unit then drops its
//   grenades (unless the globals keep them and the variant's +0x1d4 chance says so), its weapon's loaded fraction
//   is drawn from +0x1d8..+0x1dc and its reserve from +0x1e0..+0x1e2, the actor is deleted (0x427e60) and its
//   encounter's morale recomputed.
// blam-cc: stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern ai_globals *ai_globals_ptr;

extern real random_real(void); // 0x4019f0
extern real random_real_range(real min, real max); // 0x401050
extern datum_index unit_get_weapon_object_index(uint32_t unit_index, int16_t slot_index); // 0x569970, EAX, CX
extern void unit_set_control_countdown(uint32_t unit_index, int32_t countdown, uint32_t extra_control_flags); // 0x563b20, EAX, stack
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940
extern void actor_delete(datum_index actor_index, uint32_t flag); // 0x427e60, EBX, stack
extern void weapon_set_loaded_ammo_fraction(datum_index item_index, real fraction); // 0x4c58c0, EAX, stack
extern void weapon_set_ammo_counts(datum_index item_index, int16_t *reserve_counts); // 0x4c5820, EAX, stack

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

static uint32_t actor_death_random_16(void)
{
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    return random_seed_global >> 16;
}

void actor_attempt_grenade_throw(datum_index actor_index)
{
    uint8_t *a = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;       // esi
    uint8_t *variant = (uint8_t *)tag_instances[((actor *)a)->actor_variant_tag & 0xffff].data; // ebp
    datum_index encounter = ((actor *)a)->encounter_index;                             // [esp+0x18]
    uint8_t *unit;
    datum_index weapon;
    real roll;

    // 0x428ae7: a fighter may pull a grenade as it dies
    if (((actor *)a)->awareness_level == 3 && ((struct actor *)a)->combat_status >= 2) {
        unit = OBJECT_DATA(((actor *)a)->unit_index);
        if (((((unit_object *)unit)->unit.flags >> 6) & 1) &&
            unit_get_weapon_object_index(((actor *)a)->unit_index, ((unit_object *)unit)->unit.current_weapon_index) != k_datum_index_none &&
            ((struct unit_object *)unit)->unit.delayed_weapon_drop_ticks > 0) {
            float chance = ((ActorVariant *)variant)->death_fire_wildly_chance;

            if (!(chance >= 0.1f)) {
                chance = 0.1f;
            } else if (!(chance <= 0.6f)) {
                chance = 0.6f;
            }
            if (a[0x378] || (((struct actor *)a)->firing_target_type > 0 && *(float *)(a + 0x648) < 3.0f)) {
                float boosted = chance * 4.0f;

                if (!(boosted <= 0.6f)) {
                    boosted = 0.6f;
                }
                if (!(chance > boosted)) {
                    chance = boosted;
                }
            }
            if (random_real() < chance) {
                float seconds = ((ActorVariant *)variant)->death_fire_wildly_time;
                int16_t ticks;

                if (seconds == 0.0f) {
                    seconds = random_real_range(0.8f, 1.3f);
                } else if (!(seconds >= 0.8f)) {
                    seconds = 0.8f;
                } else if (!(seconds <= 1.3f)) {
                    seconds = 1.3f;
                }
                ticks = (int16_t)(int32_t)(seconds * 30.0f);
                unit_set_control_countdown(((actor *)a)->unit_index, ticks, 0x800);
                unit[0x28c] = (uint8_t)ticks;
            }
        }
    }

    // 0x428cab: what the corpse leaves behind
    roll = (real)(int32_t)actor_death_random_16() * 1.5259022e-05f;
    unit = OBJECT_DATA(((actor *)a)->unit_index);
    weapon = ((unit_object *)unit)->unit.current_weapon_index != -1 ? *(datum_index *)(unit + 0x2f8 + ((unit_object *)unit)->unit.current_weapon_index * 4)
                                              : k_datum_index_none;
    if (!ai_globals_ptr->grenades_enabled || roll < ((ActorVariant *)variant)->don_t_drop_grenades_chance) {
        *(int16_t *)(unit + 0x31e) = 0;
    }
    if (weapon != k_datum_index_none) {
        float lo = *(float *)(variant + 0x1d8);
        float hi = *(float *)(variant + 0x1dc);
        int16_t least = *(int16_t *)(variant + 0x1e0);
        int16_t most = *(int16_t *)(variant + 0x1e2);

        if (lo > 0.0f || hi > 0.0f) {
            real r = (real)(int32_t)actor_death_random_16() * 1.5259022e-05f;

            weapon_set_loaded_ammo_fraction(weapon, (hi - lo) * r + lo);
        }
        if (least > 0 || most > 0) {
            int16_t counts[2] = {0, 0};
            uint32_t r = actor_death_random_16();

            counts[0] = (int16_t)((uint32_t)(((int32_t)(int16_t)(most + 1) - least) * (int32_t)r) >> 16) + least;
            weapon_set_ammo_counts(weapon, counts);
        }
    }
    actor_delete(actor_index, 1);
    if (encounter != k_datum_index_none) {
        encounter_recompute_morale(encounter);
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
