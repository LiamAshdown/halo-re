// actor_evaluate_custom_charge_trigger  (Ghidra: actor_evaluate_custom_charge_trigger, renamed)
// address 0x424090, size 1267 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (REWRITTEN from the disassembly (see note above the function))
// evidence: phase-4 summary "Evaluates the 'custom' charge-trigger condition for melee/charge
// behavior, combining distance thresholds, aggression flags, and a randomized roll weighted
// by nearby allies and enemies." types/ai.h actor.awareness_level(0x6a)/combat_status/
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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data;      // 0x00880360
extern data_array *object_data;     // 0x008603b0
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0

extern real random_real_range(real min, real max); // 0x401050
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, UNSURE signature
extern uint8_t actor_has_unshielded_threat_weapon(datum_index actor_index); // 0x428370
extern uint8_t unit_is_in_busy_animation_state(uint32_t unit_index); // 0x569c90, ECX unit
extern void actor_prop_iterator_init(datum_index actor_index, actor_prop_iterator *out_iterator);
                                    // 0x43ecd0, already rewritten as
                                    // actor_index, stack -> iterator. It seeds only the
                                    // iterator's SECOND dword (.next) from actor.first_prop,
                                    // which is the field the loop below reads.

// REWRITTEN (0x424090..0x424582, raw offsets throughout):
//   gates: unit busy or actor+0x4a8 -> awareness +0x6a < 3 -> true; +0x6e < 5 -> false; unit
//   +0x2a3 == 0x17 without +0x378 -> true; target distance (prop +0x11c) > def+0x74, or with
//   +0x378 > def+0x16c -> false; unit +0x106 sign bit -> true; +0x378 -> false; vehicle mode
//   (+0x6c == 10) with +0xa0 in {2, 3} -> false; no threat weapon or +0x15d -> false; variant
//   +0x4c: 0 -> false, 1 -> true; target nearer than def+0xa0 -> true.
//   +0x362 set (a decision is live): +0x366 cooldown counts down; otherwise, with variant flag
//   0x08 and +0x245 > 0, the charging allies within 15 are split by their offset along the
//   target prop's +0xe0 axis (> 1.4 ahead, >= -1.4 level, else behind) and may flip +0x363
//   early; else +0x364 counts down and flips +0x363 at zero.
//   +0x362 clear: roll against variant+0x50, adjusted by the same-type allies (+0x6e >= 5)
//   split on their +0x358: c - ((1 - c) * with - c * without) * 0.5 when +0x200 > 0.
//   A new decision stores +0x363, +0x364 = max(random[variant+0x54/0x58 or 0x5c/0x60] * 30, 31)
//   truncated, +0x366 = 30.
//   Fixes vs the old C: the timer was __ftol(0.0) (random result dropped, no 31 floor), and the
//   ally split read +0x378 where the binary reads +0x358.
// blam-cc: stack -> actor_index
uint8_t actor_evaluate_custom_charge_trigger(datum_index actor_index)
{
    uint8_t *self = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    const uint8_t *variant = (const uint8_t *)tag_instances[*(uint32_t *)&((actor *)self)->actor_variant_tag & 0xffff].data;
    const uint8_t *def = (const uint8_t *)actor_get_actor_definition(actor_index);
    uint32_t unit_index = *(uint32_t *)&((actor *)self)->unit_index;
    const uint8_t *unit;
    const uint8_t *target = 0;
    uint8_t decision;
    int16_t variant_mode;

    if (!unit_is_in_busy_animation_state(unit_index) && self[0x4a8] == 0) {
        goto return_true;
    }
    if (((actor *)self)->awareness_level < 3) {
        goto return_true;
    }
    if (((struct actor *)self)->combat_status < 5) {
        goto return_false;
    }
    unit = (const uint8_t *)((object_header *)object_data->data)[unit_index & 0xffff].data;
    if (*(uint32_t *)&((actor *)self)->target_unit_index != 0xffffffff) {
        target = (const uint8_t *)prop_data->data + (*(uint32_t *)&((actor *)self)->target_unit_index & 0xffff) * 0x138;
    }
    if (unit[0x2a3] == 0x17 && self[0x378] == 0) {
        goto return_true;
    }
    if (target != 0 && *(const float *)(target + 0x11c) > *(const float *)(def + 0x74)) {
        goto return_false;
    }
    if (self[0x378] != 0 && target != 0 &&
        *(const float *)(target + 0x11c) > *(const float *)(def + 0x16c)) {
        goto return_false;
    }
    if ((int8_t)unit[0x106] < 0) {
        goto return_true;
    }
    if (self[0x378] != 0) {
        goto return_false;
    }
    if (((actor *)self)->mode == 10 &&
        (*(int16_t *)(self + 0xa0) == 2 || *(int16_t *)(self + 0xa0) == 3)) {
        goto return_false;
    }
    if (!actor_has_unshielded_threat_weapon(actor_index)) {
        goto return_false;
    }
    if (self[0x15d] != 0) {
        goto return_false;
    }
    variant_mode = *(int16_t *)&((ActorVariant *)variant)->movement_type;
    if (variant_mode == 0) {
        goto return_false;
    }
    if (variant_mode == 1) {
        goto return_true;
    }
    if (target != 0 && !(*(const float *)(target + 0x11c) >= *(const float *)(def + 0xa0))) {
        goto return_true;
    }

    if (self[0x362] != 0) {
        if (*(int16_t *)(self + 0x366) > 0) {
            *(int16_t *)(self + 0x366) -= 1;
        } else if ((variant[0] & 8) != 0 && (int8_t)self[0x245] > 0) {
            // 0x424278: no -1 check on +0x270 here; the binary indexes the prop array directly
            const uint8_t *axis_prop = (const uint8_t *)prop_data->data +
                (*(uint32_t *)&((actor *)self)->target_unit_index & 0xffff) * 0x138;
            actor_prop_iterator iterator;
            uint32_t cursor;
            int32_t ahead = 0, level = 0, behind = 0;

            actor_prop_iterator_init(actor_index, &iterator);
            cursor = iterator.next;
            while (cursor != 0xffffffff) {
                const uint8_t *p = (const uint8_t *)prop_data->data + (cursor & 0xffff) * 0x138;
                int16_t kind = *(const int16_t *)(p + 0x24);
                uint32_t owner;
                const uint8_t *other;
                float dot;

                cursor = *(const uint32_t *)(p + 0x8);
                if (kind < 2 || kind > 3 || p[0x60] != 0 || p[0x127] != 0) {
                    continue;
                }
                if (*(const float *)(p + 0x11c) >= 15.0f) {
                    continue;
                }
                owner = *(const uint32_t *)(p + 0x1c);
                if (owner == 0xffffffff) {
                    continue;
                }
                other = (const uint8_t *)actor_data->data + (owner & 0xffff) * 0x724;
                if (other[0x362] == 0) {
                    continue;
                }
                dot = (*(const float *)(other + 0x134) - ((actor *)self)->body_position.z) * *(const float *)(axis_prop + 0xe8) +
                      (*(const float *)(other + 0x130) - ((actor *)self)->body_position.y) * *(const float *)(axis_prop + 0xe4) +
                      (*(const float *)(other + 0x12c) - ((actor *)self)->body_position.x) * *(const float *)(axis_prop + 0xe0);
                if (dot > 1.4f) {
                    ahead++;
                } else if (dot >= -1.4f) {
                    level++;
                } else {
                    behind++;
                }
            }

            if (self[0x363] != 0) {
                if ((int16_t)behind == 0 && (int16_t)ahead > (int16_t)level) {
                    decision = 0; // 0x4243c7: !+0x363 with +0x363 set
                    goto apply_decision;
                }
            } else if ((int16_t)ahead == 0 && (int16_t)behind > (int16_t)level) {
                goto flip_decision;
            }
        }
        *(int16_t *)(self + 0x364) -= 1;
        if (*(int16_t *)(self + 0x364) != 0) {
            return self[0x363];
        }
    flip_decision:
        decision = (self[0x363] == 0);
    } else {
        float chance = ((ActorVariant *)variant)->initial_crouch_chance;
        float roll;

        if ((int8_t)self[0x200] > 0) {
            uint32_t cursor = *(uint32_t *)&((actor *)self)->first_prop;
            int16_t without = 0, with = 0;

            while (cursor != 0xffffffff) {
                const uint8_t *p = (const uint8_t *)prop_data->data + (cursor & 0xffff) * 0x138;
                int16_t kind = *(const int16_t *)(p + 0x24);
                uint32_t owner;
                const uint8_t *other;

                cursor = *(const uint32_t *)(p + 0x8);
                if (kind < 2 || kind > 3 || p[0x60] != 0 || p[0x127] != 0) {
                    continue;
                }
                owner = *(const uint32_t *)(p + 0x1c);
                if (owner == 0xffffffff) {
                    continue;
                }
                other = (const uint8_t *)actor_data->data + (owner & 0xffff) * 0x724;
                if (*(const int16_t *)(other + 0x4) != ((actor *)self)->type ||
                    *(const int16_t *)(other + 0x6e) < 5) {
                    continue;
                }
                if (other[0x358] != 0) {
                    with++;
                } else {
                    without++;
                }
            }
            chance = chance - ((1.0f - chance) * (float)(int32_t)with +
                               (-chance) * (float)(int32_t)without) * 0.5f;
        }

        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        self[0x362] = 1;
        roll = (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f;
        decision = (roll >= chance) ? 0 : 1;
    }

apply_decision:
    {
        float ticks;

        self[0x363] = decision;
        if (decision != 0) {
            ticks = random_real_range(*(const float *)(variant + 0x54), *(const float *)(variant + 0x58));
        } else {
            ticks = random_real_range(*(const float *)(variant + 0x5c), *(const float *)(variant + 0x60));
        }
        ticks = ticks * 30.0f;
        if (!(ticks > 31.0f)) {
            ticks = 31.0f;
        }
        *(int16_t *)(self + 0x364) = (int16_t)(int32_t)ticks;
        *(int16_t *)(self + 0x366) = 0x1e;
    }
    return self[0x363];

return_true:
    self[0x362] = 0;
    return 1;

return_false:
    self[0x362] = 0;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
