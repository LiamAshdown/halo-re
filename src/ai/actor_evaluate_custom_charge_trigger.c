// actor_evaluate_custom_charge_trigger  (Ghidra: actor_evaluate_custom_charge_trigger, renamed)
// address 0x424090, size 1267 bytes
// name confidence: 0.4   rewrite confidence: 0.15
// evidence: phase-4 summary "Evaluates the 'custom' charge-trigger condition for melee/charge
// behavior, combining distance thresholds, aggression flags, and a randomized roll weighted
// by nearby allies and enemies." types/ai.h actor.awareness_level(0x6a)/unknown_6e/
// unit_index(0x18)/target_unit_index(0x270)/first_prop(0x50); prop.kind(0x24)/is_unit(0x60)/
// is_vault(0x127)/owner_actor_index(0x1c)/distance(0x11c). Calls actor_get_actor_definition
// (0x40fa70), actor_has_unshielded_threat_weapon (0x428370, already rewritten in this
// module), random_real_range (0x401050), and actor_prop_iterator_init/unit_is_in_busy_animation_state, neither established
// elsewhere in this repo.
//   UNSURE: this is one of the least-confident rewrites in this pass, and by a wide margin
//   the largest and least-understood function in this rewrite's range. actor.unknown_362/
//   363/364/366 (a cached decision, a timer, and a recheck-cooldown, all inside
//   types/ai.h's opaque unknown_350/mode_data-adjacent region) and every ActorType-tag
//   (`iVar6`/`actor_definition`) and ActorVariant-tag (`pbVar2`) offset are kept as raw
//   pointer arithmetic; none of them are individually named in types/tags.h or types/ai.h.
//   The control flow is a literal goto-preserving transliteration of the decompile rather
//   than a restructured version, to minimize the risk of subtly changing behavior in a
//   function this tangled.
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
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0

extern real random_real_range(real min, real max); // 0x401050
extern int32_t __ftol(double x); // FISTP-based float-to-int truncation
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, UNSURE signature
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370
extern uint8_t unit_is_in_busy_animation_state(void); // 0x569c90, UNSURE signature (no traced args here either)
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator);
                                    // 0x43ecd0, already rewritten as
                                    // src/ai/actor_prop_iterator_init.c; blam-cc: EAX ->
                                    // actor_index, stack -> iterator. It seeds only the
                                    // iterator's SECOND dword (.next) from actor.first_prop,
                                    // which is the field the loop below reads.

// blam-cc: EAX -> actor_index
uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    const uint8_t *variant_tag_data = (const uint8_t *)(tag_instances[self->actor_variant_tag & 0xffff].data);
    const uint8_t *actor_def;
    object *unit_object;
    prop *target = 0;
    uint8_t decision;
    uint32_t scan_cursor;

    actor_def = actor_get_actor_definition(actor_index);
    unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;

    if ((unit_is_in_busy_animation_state() == 0 && self->movement_action_complete == 0) || self->awareness_level < 3) {
        goto return_true;
    }
    if (self->unknown_6e < 5) {
        goto return_false;
    }

    if (self->target_unit_index != (datum_index)k_datum_index_none) {
        target = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
    }

    if (*((const uint8_t *)unit_object + 0x2a3) == 0x17 && self->unknown_378 == 0) { // UNSURE offset
        goto return_true;
    }
    if ((target != 0 && *(const float *)(actor_def + 0x74) < target->distance) ||
        (self->unknown_378 != 0 && target != 0 && *(const float *)(actor_def + 0x16c) < target->distance)) { // UNSURE offsets
        goto return_false;
    }
    if ((int8_t)(*((const uint8_t *)unit_object + 0x106)) < 0) {
        goto return_true;
    }
    if (self->unknown_378 != 0 ||
        (self->mode == _actor_mode_vehicle && (*(int16_t *)&self->mode_data[4] == 2 || *(int16_t *)&self->mode_data[4] == 3)) ||
        actor_has_unshielded_threat_weapon(actor_index) == 0 ||
        (self->unknown_15d != 0 || *(const int16_t *)(variant_tag_data + 0x4c) == 0)) { // UNSURE offset
        goto return_false;
    }
    if (*(const int16_t *)(variant_tag_data + 0x4c) == 1 || // UNSURE offset
        (target != 0 && target->distance < *(const float *)(actor_def + 0xa0))) { // UNSURE offset
        goto return_true;
    }

    if ((*(uint8_t *)((uint8_t *)self + 0x362)) == 0) { // UNSURE offset
        float base_chance = *(const float *)(variant_tag_data + 0x50); // UNSURE offset

        if ((int8_t)(*(int8_t *)((uint8_t *)self + 0x200)) > 0) { // UNSURE: reusing unknown_1fc/0x200 area, see header
            datum_index prop_index = self->first_prop;
            int16_t friendly = 0, enemy = 0;

            while (prop_index != (datum_index)k_datum_index_none) {
                prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
                int16_t kind = p->kind;
                prop_index = p->next_in_actor;

                if (kind > 1 && kind < 4 && p->is_unit == 0 && p->is_vault == 0 &&
                    p->owner_actor_index != (datum_index)k_datum_index_none) {
                    actor *other = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                    if (other->type == self->type && other->unknown_6e > 4) {
                        if (other->unknown_378 == 0) {
                            friendly = friendly + 1;
                        } else {
                            enemy = enemy + 1;
                        }
                    }
                }
            }
            base_chance = base_chance - (-base_chance * (float)friendly + (1.0f - base_chance) * (float)enemy) * 0.5f;
        }

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        (*(uint8_t *)((uint8_t *)self + 0x362)) = 1; // UNSURE offset
        // The original is `if (base_chance <= rand) false; else true;`, i.e. the actor charges
        // when the rolled value is strictly BELOW base_chance. An earlier draft had the
        // comparison the other way round, which inverts the whole trigger.
        decision = base_chance > (float)(random_seed_global >> 0x10) * 1.5259022e-05f;
    } else {
        if ((*(int16_t *)((uint8_t *)self + 0x366)) >= 1) { // UNSURE offset
            (*(int16_t *)((uint8_t *)self + 0x366)) = (*(int16_t *)((uint8_t *)self + 0x366)) - 1;
            goto decrement_timer;
        }

        if ((*variant_tag_data & 8) == 0 || (int8_t)(*(int8_t *)((uint8_t *)self + 0x245)) < 1) { // UNSURE offsets
            goto decrement_timer;
        }

        {
            actor_prop_iterator iterator;
            int16_t far_count = 0, near_friendly = 0, near_enemy = 0;

            // The cursor the original walks is the iterator's +4 dword (Ghidra's `local_4`,
            // four bytes past the `local_8` it hands to the initializer), i.e. .next. An
            // earlier draft read the record's first dword, which is never written at all.
            actor_prop_iterator_init(actor_index, &iterator);
            scan_cursor = iterator.next;
            while (scan_cursor != 0xffffffff) {
                prop *p = &((prop *)prop_data->data)[scan_cursor & 0xffff];
                int16_t kind = p->kind;
                scan_cursor = p->next_in_actor;

                if (kind > 1 && kind < 4 && p->is_unit == 0 && p->is_vault == 0 &&
                    p->distance < 15.0f && p->owner_actor_index != (datum_index)k_datum_index_none) {
                    actor *other = &((actor *)actor_data->data)[p->owner_actor_index & 0xffff];
                    if ((*(uint8_t *)((uint8_t *)other + 0x362)) != 0) { // UNSURE offset
                        float dot = (other->body_position.x - self->body_position.x) * *(const float *)((const uint8_t *)target + 0xe0) // UNSURE: 0xe0 relative to target_unit_index prop, see below
                                  + (other->body_position.y - self->body_position.y) * *(const float *)((const uint8_t *)target + 0xe4)
                                  + (other->body_position.z - self->body_position.z) * *(const float *)((const uint8_t *)target + 0xe8);
                        if (dot > 1.4f) {
                            far_count = far_count + 1;
                        } else if (dot >= -1.4f) {
                            near_friendly = near_friendly + 1;
                        } else {
                            near_enemy = near_enemy + 1;
                        }
                    }
                }
            }

            if ((*(uint8_t *)((uint8_t *)self + 0x363)) != 0) { // UNSURE offset
                if (near_enemy == 0 && near_friendly < far_count) {
                    decision = (*(uint8_t *)((uint8_t *)self + 0x363)) == 0;
                    goto set_decision;
                }
                goto decrement_timer;
            }
            if (far_count != 0 || near_enemy <= near_friendly) {
                goto decrement_timer;
            }
        }

        goto set_decision_true;

    decrement_timer:
        (*(int16_t *)((uint8_t *)self + 0x364)) = (*(int16_t *)((uint8_t *)self + 0x364)) - 1; // UNSURE offset
        if ((*(int16_t *)((uint8_t *)self + 0x364)) != 0) {
            goto return_cached;
        }
        goto set_decision_from_cache;
    }

set_decision:
    (*(uint8_t *)((uint8_t *)self + 0x363)) = decision; // UNSURE offset
    goto apply_decision;

set_decision_true:
    decision = 1;
    goto set_decision;

set_decision_from_cache:
    decision = (*(uint8_t *)((uint8_t *)self + 0x363)) == 0;
    (*(uint8_t *)((uint8_t *)self + 0x363)) = decision;

apply_decision:
    {
        float lo, hi;
        int32_t ticks;

        if (decision != 0) {
            lo = *(const float *)(variant_tag_data + 0x54); // UNSURE offset
            hi = *(const float *)(variant_tag_data + 0x58);
        } else {
            lo = *(const float *)(variant_tag_data + 0x5c); // UNSURE offset
            hi = *(const float *)(variant_tag_data + 0x60);
        }
        random_real_range(lo, hi);
        ticks = __ftol(0.0); // UNSURE: the random_real_range result Ghidra feeds __ftol here
                             // is not tracked at this call site
        (*(int16_t *)((uint8_t *)self + 0x364)) = (int16_t)ticks;
        (*(int16_t *)((uint8_t *)self + 0x366)) = 0x1e;
    }

return_cached:
    return (*(uint8_t *)((uint8_t *)self + 0x363));

return_true:
    (*(uint8_t *)((uint8_t *)self + 0x362)) = 0;
    return 1;

return_false:
    (*(uint8_t *)((uint8_t *)self + 0x362)) = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x424090):

undefined1 FUN_00424090(uint param_1)

{
  short sVar1;
  byte *pbVar2;
  char cVar3;
  short sVar4;
  undefined2 uVar5;
  int iVar6;
  uint uVar7;
  float min;
  int iVar8;
  float fVar9;
  int iVar10;
  short sVar11;
  int iVar12;
  short sVar13;
  int iVar14;
  bool bVar15;
  undefined1 local_8 [4];
  uint local_4;

  iVar10 = (param_1 & 0xffff) * 0x724;
  iVar12 = *(int *)(DAT_00880360 + 0x34) + iVar10;
  pbVar2 = *(byte **)((*(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x5c + iVar10) & 0xffff) * 0x20 +
                      0x14 + DAT_0087bc14);
  iVar6 = actor_get_actor_definition();
  uVar7 = *(uint *)(iVar12 + 0x18);
  cVar3 = FUN_00569c90();
  if (((cVar3 == '\0') && (*(char *)(iVar10 + 0x4a8 + *(int *)(DAT_00880360 + 0x34)) == '\0')) ||
     (*(short *)(iVar12 + 0x6a) < 3)) {
LAB_00424217:
    *(undefined1 *)(iVar12 + 0x362) = 0;
    return 1;
  }
  if (*(short *)(iVar12 + 0x6e) < 5) {
LAB_00424228:
    *(undefined1 *)(iVar12 + 0x362) = 0;
    return 0;
  }
  iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc);
  if (*(uint *)(iVar12 + 0x270) == 0xffffffff) {
    iVar14 = 0;
  }
  else {
    iVar14 = (*(uint *)(iVar12 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  }
  if ((*(char *)(iVar8 + 0x2a3) == '\x17') && (*(char *)(iVar12 + 0x378) == '\0'))
  goto LAB_00424217;
  if (((iVar14 != 0) && (*(float *)(iVar6 + 0x74) < *(float *)(iVar14 + 0x11c))) ||
     (((*(char *)(iVar12 + 0x378) != '\0' && (iVar14 != 0)) &&
      (*(float *)(iVar6 + 0x16c) < *(float *)(iVar14 + 0x11c))))) goto LAB_00424228;
  if (*(char *)(iVar8 + 0x106) < '\0') goto LAB_00424217;
  if ((((*(char *)(iVar12 + 0x378) != '\0') ||
       ((*(short *)(iVar12 + 0x6c) == 10 &&
        ((*(short *)(iVar12 + 0xa0) == 2 || (*(short *)(iVar12 + 0xa0) == 3)))))) ||
      (cVar3 = FUN_00428370(), cVar3 == '\0')) ||
     ((*(char *)(iVar12 + 0x15d) != '\0' || (*(short *)(pbVar2 + 0x4c) == 0)))) goto LAB_00424228;
  if ((*(short *)(pbVar2 + 0x4c) == 1) ||
     ((iVar14 != 0 && (*(float *)(iVar14 + 0x11c) < *(float *)(iVar6 + 0xa0))))) goto LAB_00424217;
  if (*(char *)(iVar12 + 0x362) == '\0') {
    fVar9 = *(float *)(pbVar2 + 0x50);
    if ('\0' < *(char *)(iVar12 + 0x200)) {
      iVar6 = *(int *)(DAT_00880360 + 0x34);
      uVar7 = *(uint *)(iVar6 + 0x50 + iVar10);
      sVar13 = 0;
      sVar11 = 0;
      while (uVar7 != 0xffffffff) {
        iVar10 = (uVar7 & 0xffff) * 0x138;
        sVar4 = *(short *)(iVar10 + 0x24 + *(int *)(DAT_008802c0 + 0x34));
        iVar10 = iVar10 + *(int *)(DAT_008802c0 + 0x34);
        uVar7 = *(uint *)(iVar10 + 8);
        if (((((1 < sVar4) && (sVar4 < 4)) && (*(char *)(iVar10 + 0x60) == '\0')) &&
            ((*(char *)(iVar10 + 0x127) == '\0' && (*(uint *)(iVar10 + 0x1c) != 0xffffffff)))) &&
           ((iVar10 = (*(uint *)(iVar10 + 0x1c) & 0xffff) * 0x724, iVar8 = iVar10 + iVar6,
            *(short *)(iVar10 + 4 + iVar6) == *(short *)(iVar12 + 4) &&
            (4 < *(short *)(iVar8 + 0x6e))))) {
          if (*(char *)(iVar8 + 0x358) == '\0') {
            sVar13 = sVar13 + 1;
          }
          else {
            sVar11 = sVar11 + 1;
          }
        }
      }
      fVar9 = fVar9 - (-fVar9 * (float)(int)sVar13 + (1.0 - fVar9) * (float)(int)sVar11) * 0.5;
    }
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    uVar7 = random_seed_global >> 0x10;
    *(undefined1 *)(iVar12 + 0x362) = 1;
    if (fVar9 <= (float)uVar7 * 1.5259022e-05) {
      bVar15 = false;
    }
    else {
      bVar15 = true;
    }
  }
  else {
    if (*(short *)(iVar12 + 0x366) < 1) {
      if (((*pbVar2 & 8) == 0) || (*(char *)(iVar12 + 0x245) < '\x01')) goto LAB_004243e9;
      iVar6 = *(int *)(DAT_008802c0 + 0x34);
      sVar4 = 0;
      sVar13 = 0;
      sVar11 = 0;
      iVar10 = (*(uint *)(iVar12 + 0x270) & 0xffff) * 0x138 + iVar6;
      FUN_0043ecd0(local_8);
      while (local_4 != 0xffffffff) {
        iVar8 = (local_4 & 0xffff) * 0x138;
        sVar1 = *(short *)(iVar8 + 0x24 + iVar6);
        local_4 = *(uint *)(iVar8 + 8 + iVar6);
        iVar8 = iVar8 + iVar6;
        if ((((1 < sVar1) && (sVar1 < 4)) && (*(char *)(iVar8 + 0x60) == '\0')) &&
           (((*(char *)(iVar8 + 0x127) == '\0' && (*(float *)(iVar8 + 0x11c) < 15.0)) &&
            ((*(uint *)(iVar8 + 0x1c) != 0xffffffff &&
             (iVar8 = (*(uint *)(iVar8 + 0x1c) & 0xffff) * 0x724,
             iVar14 = iVar8 + *(int *)(DAT_00880360 + 0x34),
             *(char *)(iVar8 + 0x362 + *(int *)(DAT_00880360 + 0x34)) != '\0')))))) {
          fVar9 = (*(float *)(iVar14 + 300) - *(float *)(iVar12 + 300)) * *(float *)(iVar10 + 0xe0)
                  + (*(float *)(iVar14 + 0x130) - *(float *)(iVar12 + 0x130)) *
                    *(float *)(iVar10 + 0xe4) +
                    (*(float *)(iVar14 + 0x134) - *(float *)(iVar12 + 0x134)) *
                    *(float *)(iVar10 + 0xe8);
          if (fVar9 <= 1.4) {
            if (-1.4 <= fVar9) {
              sVar13 = sVar13 + 1;
            }
            else {
              sVar11 = sVar11 + 1;
            }
          }
          else {
            sVar4 = sVar4 + 1;
          }
        }
      }
      if (*(char *)(iVar12 + 0x363) != '\0') {
        if ((sVar11 == 0) && (sVar13 < sVar4)) {
          bVar15 = *(char *)(iVar12 + 0x363) == '\0';
          goto LAB_00424523;
        }
        goto LAB_004243e9;
      }
      if ((sVar4 != 0) || (sVar11 <= sVar13)) goto LAB_004243e9;
    }
    else {
      *(short *)(iVar12 + 0x366) = *(short *)(iVar12 + 0x366) + -1;
LAB_004243e9:
      *(short *)(iVar12 + 0x364) = *(short *)(iVar12 + 0x364) + -1;
      if (*(short *)(iVar12 + 0x364) != 0) goto LAB_00424575;
    }
    bVar15 = *(char *)(iVar12 + 0x363) == '\0';
  }
LAB_00424523:
  *(bool *)(iVar12 + 0x363) = bVar15;
  if (bVar15) {
    fVar9 = *(float *)(pbVar2 + 0x58);
    min = *(float *)(pbVar2 + 0x54);
  }
  else {
    fVar9 = *(float *)(pbVar2 + 0x60);
    min = *(float *)(pbVar2 + 0x5c);
  }
  random_real_range(min,fVar9);
  uVar5 = __ftol();
  <ticks> written to unknown_364; unknown_366=0x1e.
  return unknown_363;
}
#endif
