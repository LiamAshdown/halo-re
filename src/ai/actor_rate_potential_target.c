// actor_rate_potential_target  (Ghidra: actor_rate_potential_target, already named)
// address 0x41fd50, size 888 bytes
// name confidence: 0.6   rewrite confidence: 0.85 (REWRITTEN/verified end to end against 0x41fd50: fixed the far-melee case (2, not unchanged) and the NULL threat-weapon case (falls to the team check); score summed in the binary order)
// evidence: out/phase2/results/ai_02.json -- given an actor and a candidate prop (target-data
//   record), combines several bonus categories into a float score with a 1/(dist*0.1+1)
//   falloff term; callers (actor_choose_best_target, actor_consider_target_candidate) store the
//   result into prop.desirability. Confirmed by this batch's actor_consider_target_candidate.c
//   call site to take (actor_index, target_prop_index) and return float. The tag-data float
//   fields at ActorVariant+0x74/0xa0/0x160/0x170 resolve to maximum_firing_distance,
//   desired_combat_range[1], melee_range and berserk_melee_range in types/tags.h.
// register convention: Ghidra shows this as (float param_1, uint param_2), but param_1's bit
//   pattern is used directly as an actor_index (`(uint)param_1 & 0xffff`) -- it is not really a
//   float. Modeled here as (datum_index actor_index, datum_index target_prop_index).
//
// UNSURE, substantially: two call sites (actor_has_unshielded_threat_weapon and the actor_get_threat_weapon_definition/
// actor_get_actor_definition pair) are each followed by an `extraout_ST0` float that
// contributes to the final score with no traced source -- the x87-return-plus-EAX-bool pattern
// seen elsewhere in this module (see actor_target_hearing_check.c). Modeled with the same small
// result-struct trick; the actual source of these floats is unresolved.
// UNSURE: Actor tag +0x38c (self.actor_definition_tag's tag data) has no name in the portion of
// types/tags.h traced for this batch; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14

extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70

// UNSURE: see file header -- the (AL bool, ST0 float) dual return is modelled by
// types/ai.h bool_float_return, shared with actor_target_hearing_check.c.
// FIXED: 0x428370 returns only AL (it never touches the FPU); a struct return made MSVC pass a hidden result
// pointer in the actor_index slot. The float the old model read is the 0.0 loaded at 0x41fde0, i.e. `extra`.
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370, EAX actor
extern uint8_t *actor_get_threat_weapon_definition(datum_index actor_index); // 0x40f970, UNSURE signature: return type
                                                        // is a tag data pointer of unresolved type

// Computes a floating-point desirability score for a candidate prop (target-data record) as
// the given actor's next combat target, combining visibility, distance, alertness and
// prior-target continuity bonuses. Returns 0.0 for a disqualified candidate.
float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index)
{
    actor *self;
    prop *target;
    Actor *actor_def;
    ActorVariant *variant_def;
    uint8_t *override_tag; // UNSURE: unresolved tag type, see actor_get_threat_weapon_definition
    int8_t bonus_a; // cVar4
    int8_t bonus_b; // cVar2
    int8_t bonus_c; // cVar3
    int8_t bonus_d; // cVar6
    float extra;    // fVar10
    float threshold;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    target = (prop *)((uint8_t *)prop_data->data + (target_prop_index & 0xffff) * sizeof(prop));

    if (target->unknown_133 != 0 || target->is_unit == 0) {
        return 0.0f;
    }

    bonus_a = 0;
    if ((-1 < target->kind && target->kind < 2) ||
        ((target->is_vault != 0 && 0x95 < target->unknown_76) || target->actor_type == 0xf)) {
        return 0.0f;
    }

    extra = 0.0f;
    actor_def = (Actor *)tag_instances[self->actor_definition_tag & 0xffff].data;
    variant_def = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;
    bonus_b = 0;
    bonus_c = 0;

    if (self->swarm == 0 && target->unknown_9c < 1) {
        bonus_a = (int8_t)actor_has_unshielded_threat_weapon(actor_index);

        if (bonus_a == 0) {
            // FIXED: the two ranges were swapped. Ghidra reads ActorVariant+0x170 when
            // actor.unknown_378 is clear and ActorVariant+0x160 when it is set, and
            // types/tags.h puts melee_range at 0x160 and berserk_melee_range at 0x170.
            threshold = (self->unknown_378 == 0) ? variant_def->berserk_melee_range
                                                 : variant_def->melee_range;

            if (2.0f <= target->distance || (bonus_a = 5, target->kind == 5)) {
                if (target->relationship_object_index == -1) {
                    if (target->unknown_130 == 0 || ((struct Actor *)actor_def)->melee_leap_velocity != 0.0f) {
                        if (target->unknown_118 == self->unknown_15d) {
                            // FIXED (0x41fec2): beyond the melee threshold -> 2 (0x41ff0a),
                            //   within it -> 3; the old C left 0 / 5 in place for the far case
                            bonus_a = (target->distance >= threshold) ? 2 : 3;
                        } else {
                            bonus_a = 1;
                        }
                    } else {
                        bonus_a = 0;
                    }
                } else {
                    bonus_a = 0;
                }
            }
        } else {
            override_tag = actor_get_threat_weapon_definition(actor_index);
            extra = 0.0f; // UNSURE: extraout_ST0_00, source unresolved -- see file header
            variant_def = (ActorVariant *)actor_get_actor_definition(actor_index);

            // FIXED (0x41fef1): a NULL threat weapon definition skips the range test and falls
            //   through to the team check; the old C returned 2 for it
            if (override_tag != (uint8_t *)0 && target->distance >= *(float *)(override_tag + 0x40c)) {
                bonus_a = 2;
            } else if (target->unknown_118 == self->unknown_15d) {
                if (2.0f <= target->distance || (bonus_a = 5, target->kind == 5)) {
                    // 0x41ff44 / 0x41ff5e: definition +0xa0 and +0x74
                    if (target->distance >= *(const float *)((const uint8_t *)variant_def + 0xa0)) {
                        bonus_a = 2;
                        if (target->distance >= ((struct ActorVariant *)variant_def)->maximum_firing_distance) {
                            bonus_a = 1;
                        }
                    } else {
                        bonus_a = 3;
                    }
                }
            } else {
                bonus_a = 2;
            }
        }
    }

    if (target->is_vault == 0) {
        if (self->swarm == 0 && target->seen != 0 && target->unknown_9c == 0) {
            bonus_d = 6;
        } else if (target->kind < 2 || 3 < target->kind) {
            bonus_d = (target->unknown_b8 == 0) ? (int8_t)(target->kind == 4) + 1 : 3;
        } else if (self->swarm != 0) {
            bonus_d = 4;
        } else if (0 < target->unknown_9c) {
            bonus_d = 3;
        } else if (target->unknown_38 != 0 && target->unknown_38 != 1) {
            bonus_d = 3;
        } else if (target->unknown_12f != 0 && (int8_t)target->unknown_122 < 2) {
            // FIXED: Ghidra compares prop+0x122 as a signed char.
            bonus_d = 5;
        } else {
            bonus_d = 4;
        }
    } else {
        bonus_d = 1;
    }

    if (self->target_unit_index == k_datum_index_none) {
        if (target->is_parented != 0 || target_prop_index == self->unknown_54) {
            extra = 3.0f;
        }
    } else if (target_prop_index == self->target_unit_index && 2 < self->combat_status) {
        bonus_b = 1;
    }

    if (target->unknown_134 != 0) {
        bonus_c = 2;
    }

    // 0x420074: (extra + 5 / (distance * 0.1 + 1)) + (int)(c + b + a + d) * 10, in that order
    return extra + 5.0f / (target->distance * 0.1f + 1.0f) +
           (float)((int32_t)bonus_c + (int32_t)bonus_b + (int32_t)bonus_a + (int32_t)bonus_d) * 10.0f;
}

#if 0
Original Ghidra decompilation (0x41fd50):

float10 actor_rate_potential_target(float param_1,uint param_2)

{
  short sVar1;
  char cVar2;
  char cVar3;
  char cVar4;
  int iVar5;
  char cVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  float10 fVar10;
  float10 extraout_ST0;
  float10 extraout_ST0_00;

  iVar7 = (param_2 & 0xffff) * 0x138;
  iVar9 = ((uint)param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  iVar8 = iVar7 + *(int *)(DAT_008802c0 + 0x34);
  if ((*(char *)(iVar7 + 0x133 + *(int *)(DAT_008802c0 + 0x34)) != '\0') ||
     (*(char *)(iVar8 + 0x60) == '\0')) {
LAB_004200bb:
    return (float10)0.0;
  }
  cVar4 = '\0';
  if (((-1 < *(short *)(iVar8 + 0x24)) && (*(short *)(iVar8 + 0x24) < 2)) ||
     (((*(char *)(iVar8 + 0x127) != '\0' && (0x95 < *(short *)(iVar8 + 0x76))) ||
      (*(short *)(iVar8 + 0x10) == 0xf))) goto LAB_004200bb;
  fVar10 = (float10)0.0;
  iVar7 = *(int *)((*(uint *)(iVar9 + 0x58) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar5 = *(int *)((*(uint *)(iVar9 + 0x5c) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  cVar2 = '\0';
  cVar3 = '\0';
  if ((*(char *)(iVar9 + 6) == '\0') && (*(short *)(iVar8 + 0x9c) < 1)) {
    cVar4 = FUN_00428370();
    if (cVar4 == '\0') {
      if (*(char *)(iVar9 + 0x378) == '\0') {
        param_1 = *(float *)(iVar5 + 0x170);
      }
      else {
        param_1 = *(float *)(iVar5 + 0x160);
      }
      fVar10 = extraout_ST0;
      if ((2.0 <= *(float *)(iVar8 + 0x11c)) || (cVar4 = '\x05', *(short *)(iVar8 + 0x24) == 5)) {
        if (*(int *)(iVar8 + 0x110) == -1) {
          if ((*(char *)(iVar8 + 0x130) == '\0') || (*(float *)(iVar7 + 0x38c) != 0.0)) {
            if (*(char *)(iVar8 + 0x118) == *(char *)(iVar9 + 0x15d)) {
              if (param_1 <= *(float *)(iVar8 + 0x11c)) goto LAB_0041ff0a;
              cVar4 = '\x03';
            }
            else {
LAB_0041ff73:
              cVar4 = '\x01';
            }
          }
          else {
            cVar4 = '\0';
          }
        }
        else {
          cVar4 = '\0';
        }
      }
    }
    else {
      iVar7 = FUN_0040f970();
      iVar5 = actor_get_actor_definition();
      fVar10 = extraout_ST0_00;
      if ((iVar7 == 0) || (*(float *)(iVar7 + 0x40c) <= *(float *)(iVar8 + 0x11c))) {
        if (*(char *)(iVar8 + 0x118) == *(char *)(iVar9 + 0x15d)) {
          if ((2.0 <= *(float *)(iVar8 + 0x11c)) || (cVar4 = '\x05', *(short *)(iVar8 + 0x24) == 5))
          {
            if (*(float *)(iVar5 + 0xa0) <= *(float *)(iVar8 + 0x11c)) {
              cVar4 = '\x02';
              if (*(float *)(iVar5 + 0x74) <= *(float *)(iVar8 + 0x11c)) goto LAB_0041ff73;
            }
            else {
              cVar4 = '\x03';
            }
          }
        }
        else {
          cVar4 = '\x02';
        }
      }
      else {
LAB_0041ff0a:
        cVar4 = '\x02';
      }
    }
  }
  if (*(char *)(iVar8 + 0x127) == '\0') {
    if (((*(char *)(iVar9 + 6) == '\0') && (*(char *)(iVar8 + 0x74) != '\0')) &&
       (*(short *)(iVar8 + 0x9c) == 0)) {
      cVar6 = '\x06';
    }
    else {
      sVar1 = *(short *)(iVar8 + 0x24);
      if ((sVar1 < 2) || (3 < sVar1)) {
        if (*(char *)(iVar8 + 0xb8) == '\0') {
          cVar6 = (sVar1 == 4) + '\x01';
        }
        else {
          cVar6 = '\x03';
        }
      }
      else {
        if (*(char *)(iVar9 + 6) == '\0') {
          if (0 < *(short *)(iVar8 + 0x9c)) {
            cVar6 = '\x03';
            goto LAB_00420025;
          }
          if ((*(short *)(iVar8 + 0x38) != 0) && (*(short *)(iVar8 + 0x38) != 1)) {
            cVar6 = '\x03';
            goto LAB_00420025;
          }
          if ((*(char *)(iVar8 + 0x12f) != '\0') && (*(char *)(iVar8 + 0x122) < '\x02')) {
            cVar6 = '\x05';
            goto LAB_00420025;
          }
        }
        cVar6 = '\x04';
      }
    }
  }
  else {
    cVar6 = '\x01';
  }
LAB_00420025:
  if (*(uint *)(iVar9 + 0x270) == 0xffffffff) {
    if ((*(char *)(iVar8 + 0x12e) != '\0') || (param_2 == *(uint *)(iVar9 + 0x54))) {
      fVar10 = (float10)3.0;
    }
  }
  else if ((param_2 == *(uint *)(iVar9 + 0x270)) && (2 < *(short *)(iVar9 + 0x6e))) {
    cVar2 = '\x01';
  }
  if (*(char *)(iVar8 + 0x134) != '\0') {
    cVar3 = '\x02';
  }
  return (float10)(byte)(cVar3 + cVar2 + cVar4 + cVar6) * (float10)10.0 +
         (float10)5.0 / ((float10)*(float *)(iVar8 + 0x11c) * (float10)0.1 + (float10)1.0) + fVar10;
}
#endif
