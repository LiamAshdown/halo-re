// actor_find_nearest_grenade_ally  (Ghidra: actor_find_nearest_grenade_ally, renamed)
// address 0x40e540, size 530 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: phase-4 summary "counts and finds the nearest eligible ally actor for a
// coordinated grenade attack and records it on the actor"; walks actor.first_prop looking
// for eligible props, then falls back to walking the unassigned-actor list (chained through
// the same next_in_encounter field) if not enough were found, and always writes the
// closest candidate's actor handle to actor.unknown_1d0.
// register that Ghidra did recognize as char param_2.
// UNSURE: actor_validate_grenade_ally_candidate (actor_validate_grenade_ally_candidate), actor_find_prop_for_object and
// actor_find_or_create_shared_prop are all called with fewer visible arguments than their declared signatures
// need; the candidate actor / unit handle and self->swarm are threaded through here as the
// most plausible reconstruction. local_4 (the unassigned-list cursor) is never visibly
// assigned before its first use in the decompiled C; ai_reference_actor_iterator_init_cursor's return value almost
// certainly seeds it (a classic Ghidra register-return omission), reconstructed here as such.
// prop.is_unit (0x60) and prop.kind (0x24) meanings are inferred, not proven, for this use.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;   // 0x00880360
extern data_array *prop_data;    // 0x008802c0
extern ai_globals *ai_globals_ptr;

extern uint8_t actor_validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag); // 0x40e4a0, ECX, BL
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX, ECX
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX
extern datum_index actor_find_or_create_shared_prop(datum_index object_index, datum_index actor_index,
    char create_if_missing, uint32_t flag); // 0x43eb30, EAX, stack

extern double sqrt(double x);

#define PROP(h) ((uint8_t *)prop_data->data + ((h) & 0xffff) * 0x138)
#define ACTOR(h) ((uint8_t *)actor_data->data + ((h) & 0xffff) * 0x724)

// REWRITTEN from objdump 0x40e540..0x40e751. Stack: (actor, units only). Picks the nearest ally the actor could
//   team up with (0x40e4a0 validates each) into +0x1d0 and returns how many it looked at: first its own props of
//   owned allies (kinds 2..3 only when asked, nearest by the prop's distance +0x11c), then, while fewer than 1 (2
//   when asked) were seen, the actors of its encounter by body distance (their prop, found or created). The
//   draft called the validator, the cursor init and the prop lookups without operands.
// blam-cc: stack -> actor_index, widen_search
int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search)
{
    uint8_t *self = ACTOR(actor_index);             // [esp+0x1c]
    int32_t seen = 0;                               // [esp+0x10]
    int32_t limit = (widen_search != 0) + 1;        // [esp+0x20]
    datum_index best = k_datum_index_none;          // [esp+0x18]
    float best_distance = 3.4028235e+38f;           // [esp+0x14]
    datum_index prop_index;

    for (prop_index = ((struct actor *)self)->first_prop; prop_index != k_datum_index_none;) {
        uint8_t *p = PROP(prop_index);
        datum_index current = prop_index;

        prop_index = *(datum_index *)(p + 0x8);
        if (p[0x60] || p[0x127] || *(datum_index *)(p + 0x1c) == k_datum_index_none) {
            continue;
        }
        if (widen_search && !(*(int16_t *)(p + 0x24) >= 2 && *(int16_t *)(p + 0x24) <= 3)) {
            continue;
        }
        if (!actor_validate_grenade_ally_candidate(*(datum_index *)(p + 0x1c), widen_search)) {
            continue;
        }
        seen++;
        if (*(float *)(p + 0x11c) < best_distance) {
            best = current;
            best_distance = *(float *)(p + 0x11c);
        }
    }
    if (seen < (int16_t)limit && ((struct actor *)self)->encounter_index != k_datum_index_none) {
        datum_index cursor[3];                      // [esp+0x24]
        datum_index candidate;

        ai_reference_actor_iterator_init_cursor(*(int32_t *)&((struct actor *)self)->encounter_index, cursor);
        candidate = cursor[2];
        while (ai_globals_ptr->actors_valid && candidate != k_datum_index_none) {
            uint8_t *other = ACTOR(candidate);
            datum_index unit = ((struct actor *)other)->unit_index;
            datum_index current = candidate;
            datum_index prop;

            candidate = ((struct actor *)other)->next_in_encounter;
            if (unit == k_datum_index_none || !actor_validate_grenade_ally_candidate(current, widen_search)) {
                continue;
            }
            prop = actor_find_prop_for_object(unit, actor_index);
            if (prop == k_datum_index_none) {
                prop = actor_find_or_create_shared_prop(unit, actor_index, 1, 0);
                if (prop == k_datum_index_none) {
                    continue;
                }
            }
            {
                float dx = ((struct actor *)other)->body_position.x - ((struct actor *)self)->body_position.x;
                float dy = ((struct actor *)other)->body_position.y - ((struct actor *)self)->body_position.y;
                float dz = ((struct actor *)other)->body_position.z - ((struct actor *)self)->body_position.z;
                float distance = (float)sqrt(dz * dz + dx * dx + dy * dy);

                seen++;
                if (distance < best_distance) {
                    best_distance = distance;
                    best = prop;
                }
            }
            if (!(seen < (int16_t)limit)) {
                break;
            }
        }
    }
    *(datum_index *)(self + 0x1d0) = best;
    return seen;
}

#if 0
Original Ghidra decompilation (0x40e540):

int FUN_0040e540(uint param_1,char param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char cVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  uint uVar10;
  int local_20;
  float local_1c;
  uint local_18;
  short local_10;
  uint local_4;

  iVar6 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_20 = 0;
  local_10 = (param_2 != '\0') + 1;
  local_18 = 0xffffffff;
  local_1c = 3.4028235e+38;
  uVar7 = *(uint *)(iVar6 + 0x50);
  while (uVar10 = uVar7, uVar10 != 0xffffffff) {
    iVar1 = *(int *)(DAT_008802c0 + 0x34);
    iVar8 = (uVar10 & 0xffff) * 0x138;
    uVar7 = *(uint *)(iVar8 + 8 + iVar1);
    iVar9 = iVar8 + iVar1;
    if ((((((*(char *)(iVar8 + 0x60 + iVar1) == '\0') && (*(char *)(iVar9 + 0x127) == '\0')) &&
          (*(int *)(iVar9 + 0x1c) != -1)) &&
         ((param_2 == '\0' || ((1 < *(short *)(iVar9 + 0x24) && (*(short *)(iVar9 + 0x24) < 4))))))
        && (cVar5 = FUN_0040e4a0(), cVar5 != '\0')) &&
       (local_20 = local_20 + 1, *(float *)(iVar9 + 0x11c) < local_1c)) {
      local_1c = *(float *)(iVar9 + 0x11c);
      local_18 = uVar10;
    }
  }
  if ((local_20 < local_10) && (*(int *)(iVar6 + 0x34) != -1)) {
    FUN_004369f0();
    do {
      do {
        if ((*(char *)(DAT_00880354 + 1) == '\0') || (local_4 == 0xffffffff)) goto LAB_0040e738;
        iVar1 = *(int *)(DAT_00880360 + 0x34);
        iVar9 = (local_4 & 0xffff) * 0x724;
        iVar8 = *(int *)(iVar9 + 0x18 + iVar1);
        local_4 = *(uint *)(iVar9 + 0x2c + iVar1);
        iVar9 = iVar9 + iVar1;
      } while (((iVar8 == -1) || (cVar5 = FUN_0040e4a0(), cVar5 == '\0')) ||
              ((uVar7 = FUN_0043ea80(iVar8), uVar7 == 0xffffffff &&
               (uVar7 = FUN_0043eb30(param_1,1,0), uVar7 == 0xffffffff))));
      fVar2 = *(float *)(iVar9 + 300) - *(float *)(iVar6 + 300);
      fVar3 = *(float *)(iVar9 + 0x130) - *(float *)(iVar6 + 0x130);
      fVar4 = *(float *)(iVar9 + 0x134) - *(float *)(iVar6 + 0x134);
      local_20 = local_20 + 1;
      fVar2 = SQRT(fVar3 * fVar3 + fVar2 * fVar2 + fVar4 * fVar4);
      if (fVar2 < local_1c) {
        local_1c = fVar2;
        local_18 = uVar7;
      }
    } while (local_20 < local_10);
  }
LAB_0040e738:
  *(uint *)(iVar6 + 0x1d0) = local_18;
  return local_20;
}
#endif
