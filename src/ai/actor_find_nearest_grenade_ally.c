// actor_find_nearest_grenade_ally  (Ghidra: actor_find_nearest_grenade_ally, renamed)
// address 0x40e540, size 530 bytes
// name confidence: 0.35   rewrite confidence: 0.25
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

extern double sqrt(double x); // FSQRT, Ghidra's SQRT() pseudo-function; see src/math for the convention
extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0
extern ai_globals *ai_globals_ptr; // 0x00880354

extern uint8_t actor_validate_grenade_ally_candidate(datum_index candidate_actor, uint8_t caller_type_flag); // 0x40e4a0, this module
extern datum_index ai_reference_actor_iterator_init_cursor(void); // UNSURE: "head of the unassigned actor list" per types/ai.h ai_globals+0x08
extern datum_index actor_find_prop_for_object(datum_index object_index); // UNSURE signature
extern datum_index actor_find_or_create_shared_prop(datum_index actor_index, uint32_t flag_a, uint32_t flag_b); // UNSURE signature

// FIXED (register inputs, objdump: each stack slot's first use checked against the parameter): the original never reads EAX; actor_index arrive(s) on the stack (2 stack argument(s)).
// blam-cc: stack -> actor_index, widen_search
int32_t actor_find_nearest_grenade_ally(datum_index actor_index, uint8_t widen_search)
{
    actor *self;
    int32_t count;
    int16_t minimum_needed;
    datum_index best;
    float best_distance;
    datum_index prop_cursor;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    count = 0;
    minimum_needed = (widen_search != 0) + 1;
    best = (datum_index)k_datum_index_none;
    best_distance = 3.4028235e+38f;

    prop_cursor = self->first_prop;
    while (prop_cursor != (datum_index)k_datum_index_none) {
        prop *p = (prop *)((uint8_t *)prop_data->data + (prop_cursor & 0xffff) * sizeof(prop));
        datum_index next = p->next_in_actor;

        if (p->is_unit == 0 && p->is_vault == 0 && p->owner_actor_index != (datum_index)k_datum_index_none &&
            (widen_search == 0 || (1 < p->kind && p->kind < 4)) &&
            actor_validate_grenade_ally_candidate(p->owner_actor_index, self->swarm) != 0) {
            count++;
            if (p->distance < best_distance) {
                best_distance = p->distance;
                best = prop_cursor;
            }
        }
        prop_cursor = next;
    }

    if (count < minimum_needed && self->encounter_index != (datum_index)k_datum_index_none) {
        datum_index cursor = ai_reference_actor_iterator_init_cursor();
        for (;;) {
            actor *candidate;
            datum_index unit_index;
            datum_index membership;

            for (;;) {
                if (ai_globals_ptr->actors_valid == 0 || cursor == (datum_index)k_datum_index_none) {
                    goto done;
                }
                candidate = (actor *)((uint8_t *)actor_data->data + (cursor & 0xffff) * sizeof(actor));
                unit_index = candidate->unit_index;
                cursor = candidate->next_in_encounter;
                if (unit_index != (datum_index)k_datum_index_none &&
                    actor_validate_grenade_ally_candidate(unit_index, self->swarm) != 0) {
                    membership = actor_find_prop_for_object(unit_index);
                    if (membership != (datum_index)k_datum_index_none) break;
                    membership = actor_find_or_create_shared_prop(actor_index, 1, 0);
                    if (membership != (datum_index)k_datum_index_none) break;
                }
            }

            {
                float dx = candidate->body_position.x - self->body_position.x;
                float dy = candidate->body_position.y - self->body_position.y;
                float dz = candidate->body_position.z - self->body_position.z;
                float dist;

                count++;
                dist = (float)sqrt((double)(dy * dy + dx * dx + dz * dz));
                if (dist < best_distance) {
                    best_distance = dist;
                    best = membership;
                }
            }
            if (count >= minimum_needed) break;
        }
    }
done:
    self->unknown_1d0 = best;
    return count;
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
