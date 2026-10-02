// actor_score_firing_positions_by_range  (Ghidra: actor_score_firing_positions_by_range, renamed)
// address 0x411bf0, size 737 bytes
// name confidence: 0.6   rewrite confidence: 0.55
// evidence: row 1 of the scoring table at 0x006555c0, kinds mask 0x9 (goal kinds 0 and 3).
//   Every tag field it reads is a named range on the ActorVariant that
//   actor_get_actor_definition @0x40fa70 returns: maximum_firing_distance (0x74),
//   desired_combat_range[1] (0xa0) and berserk_firing_ranges[1] (0x16c). The kind 2 hazards
//   it projects candidates onto are the avoidance planes in
//   actor_firing_position_query.hazards.
// register convention: the four arguments are the Ghidra-recognized stack parameters; the
//   candidate array arrives in a float slot.
//
// This function is also the evidence that actor.unknown_378 is the berserk flag and not a
// stance selector: it is what chooses berserk_firing_ranges over desired_combat_range.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data; // 0x00880360

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this module
extern void *actor_get_threat_weapon_definition(int32_t actor_index);                    // 0x40f970, this module

// blam-cc: stack -> actor_index, query, count, candidates
// Rates candidates on distance to the threat and on clearance from the nearest avoidance
// plane. Scores are desirability, so a bigger term is a better candidate: the first term
// peaks inside 80 percent of the maximum firing distance, the second peaks at the preferred
// combat range while respecting the threat weapon own minimum range, and the third peaks
// once the candidate is 3.5 units clear of the nearest kind 2 hazard plane.
void actor_score_firing_positions_by_range(datum_index actor_index,
                                           actor_firing_position_query *query,
                                           uint16_t count,
                                           actor_firing_position_candidate *candidates)
{
    actor *self;
    ActorVariant *variant;
    actor_firing_position_candidate *c;
    void *weapon_definition;
    real_point3d *p;
    float distance;
    float threshold;
    float preferred_range;
    float minimum_range;
    float margin;
    float bonus;
    float nearest_plane;
    float dot;
    float ax, ay, az;
    int32_t i;
    int32_t j;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)actor_get_actor_definition(actor_index);

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        if (query->have_target != 0) {
            distance = (float)sqrt((double)c->distance_squared_to_target);

            if (variant->maximum_firing_distance > 0.0f) {
                threshold = variant->maximum_firing_distance * 0.8f;
                bonus = (threshold <= distance) ? (threshold / distance) * 10.0f : 10.0f;
                c->score = bonus + c->score;
            }

            if (variant->desired_combat_range[1] > 0.0f &&
                distance < variant->desired_combat_range[1]) {
                preferred_range = (self->berserking != 0) ? variant->berserk_firing_ranges[1]
                                                           : variant->desired_combat_range[1];
                weapon_definition = actor_get_threat_weapon_definition(actor_index);
                // UNSURE: the original starts this from whatever was left in ST0 by the
                // call. Zero is the only reading that makes the two guards below behave.
                minimum_range = 0.0f;
                if (weapon_definition != (void *)0 &&
                    *(float *)((uint8_t *)weapon_definition + 0x40c) > 0.0f &&
                    minimum_range <= *(float *)((uint8_t *)weapon_definition + 0x40c)) {
                    minimum_range = *(float *)((uint8_t *)weapon_definition + 0x40c);
                }
                margin = preferred_range - distance;
                if (minimum_range > 0.0f && distance - minimum_range < margin) {
                    margin = distance - minimum_range;
                }
                if (margin <= 2.0f) {
                    bonus = (margin > 0.0f) ? margin * 0.5f * 20.0f : 0.0f;
                } else {
                    bonus = 20.0f;
                }
                c->score = bonus + c->score;
            }
        }

        if (query->hazard_count_kind_2 > 0) {
            nearest_plane = 3.4028235e+38f;
            for (j = 0; j < query->hazard_count; j++) {
                actor_firing_position_hazard *h = &query->hazards[j];
                if (h->kind != 2) {
                    continue;
                }
                p = (real_point3d *)c->position;
                dot = (p->x - h->position.x) * h->direction.i +
                      (p->z - h->position.z) * h->direction.k +
                      (p->y - h->position.y) * h->direction.j;
                if (dot <= 0.0f) {
                    continue;
                }
                dot = -dot;
                ax = dot * h->direction.i + (p->x - h->position.x);
                ay = dot * h->direction.j + (p->y - h->position.y);
                az = dot * h->direction.k + (p->z - h->position.z);
                dot = ax * ax + ay * ay + az * az;
                if (dot < nearest_plane) {
                    nearest_plane = dot;
                }
            }
            bonus = (nearest_plane < 12.25f) ? (float)sqrt((double)nearest_plane) * 0.25f : 6.0f;
            c->score = bonus + c->score;
        }
    }
}

#if 0
Original Ghidra decompilation (0x411bf0):

void FUN_00411bf0(uint param_1,int param_2,ushort param_3,float param_4)

{
  int iVar1;
  float *pfVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  int iVar6;
  int iVar7;
  float *pfVar8;
  uint uVar9;
  float *pfVar10;
  float10 extraout_ST0;
  float10 fVar11;
  float local_18;
  float local_14;
  uint local_10;

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar6 = actor_get_actor_definition();
  if (0 < (short)param_3) {
    local_10 = (uint)param_3;
    pfVar10 = (float *)((int)param_4 + 0x38);
    do {
      if (*(char *)(pfVar10 + -2) != '\0') {
        if (*(char *)(param_2 + 0x5fc) != '\0') {
          fVar3 = SQRT(pfVar10[-3]);
          if (0.0 < *(float *)(iVar6 + 0x74)) {
            fVar4 = *(float *)(iVar6 + 0x74) * 0.8;
            if (fVar4 <= fVar3) {
              fVar4 = (fVar4 / fVar3) * 10.0;
            }
            else {
              fVar4 = 10.0;
            }
            *pfVar10 = fVar4 + *pfVar10;
          }
          if ((0.0 < *(float *)(iVar6 + 0xa0)) && (fVar3 < *(float *)(iVar6 + 0xa0))) {
            if (*(char *)((param_1 & 0xffff) * 0x724 + iVar1 + 0x378) == '\0') {
              local_14 = *(float *)(iVar6 + 0xa0);
            }
            else {
              local_14 = *(float *)(iVar6 + 0x16c);
            }
            iVar7 = FUN_0040f970();
            fVar11 = extraout_ST0;
            if (((iVar7 != 0) && (0.0 < *(float *)(iVar7 + 0x40c))) &&
               (extraout_ST0 <= (float10)*(float *)(iVar7 + 0x40c))) {
              fVar11 = (float10)*(float *)(iVar7 + 0x40c);
            }
            param_4 = local_14 - fVar3;
            if (((float10)0.0 < fVar11) && ((float10)fVar3 - fVar11 < (float10)param_4)) {
              param_4 = (float)((float10)fVar3 - fVar11);
            }
            if (param_4 <= 2.0) {
              fVar3 = 0.0;
              if (0.0 < param_4) {
                fVar3 = param_4 * 0.5 * 20.0;
              }
            }
            else {
              fVar3 = 20.0;
            }
            *pfVar10 = fVar3 + *pfVar10;
          }
        }
        if (0 < *(short *)(param_2 + 600)) {
          local_18 = 3.4028235e+38;
          if (0 < (short)*(ushort *)(param_2 + 0x254)) {
            pfVar8 = (float *)(param_2 + 0x270);
            uVar9 = (uint)*(ushort *)(param_2 + 0x254);
            do {
              fVar3 = local_18;
              if (*(short *)(pfVar8 + -5) == 2) {
                pfVar2 = (float *)pfVar10[-0xe];
                fVar3 = (*pfVar2 - pfVar8[-4]) * pfVar8[-1] +
                        (pfVar2[2] - pfVar8[-2]) * pfVar8[1] + (pfVar2[1] - pfVar8[-3]) * *pfVar8;
                if (0.0 < fVar3) {
                  fVar3 = -fVar3;
                  fVar4 = fVar3 * pfVar8[-1] + (*pfVar2 - pfVar8[-4]);
                  fVar5 = fVar3 * *pfVar8 + (pfVar2[1] - pfVar8[-3]);
                  fVar3 = fVar3 * pfVar8[1] + (pfVar2[2] - pfVar8[-2]);
                  fVar3 = fVar4 * fVar4 + fVar5 * fVar5 + fVar3 * fVar3;
                  if (fVar3 < local_18) goto LAB_00411e87;
                }
                fVar3 = local_18;
              }
LAB_00411e87:
              local_18 = fVar3;
              pfVar8 = pfVar8 + 7;
              uVar9 = uVar9 - 1;
            } while (uVar9 != 0);
          }
          fVar3 = 6.0;
          if (local_18 < 12.25) {
            fVar3 = SQRT(local_18) * 0.25;
          }
          *pfVar10 = fVar3 + *pfVar10;
        }
      }
      pfVar10 = pfVar10 + 0xf;
      local_10 = local_10 - 1;
    } while (local_10 != 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
