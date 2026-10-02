// actor_score_firing_positions_by_history  (Ghidra: actor_score_firing_positions_by_history, renamed)
// address 0x411ee0, size 508 bytes
// name confidence: 0.45  rewrite confidence: 0.6
// evidence: row 2 of the scoring table at 0x006555c0, kinds mask 0x4d (goal kinds 0, 2, 3
//   and 6). It has no caller in the export, because only the table reaches it and Ghidra
//   never followed the table; the row was read out of bin/halo.exe. The two tag fields it
//   reads are ActorVariant.maximum_firing_distance (0x74) and the query travel bound at
//   0x18; the second half scores candidates against the kind 0 and kind 1 hazards.
// register convention: the four arguments are the Ghidra-recognized stack parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void * actor_get_actor_definition(datum_index actor_index); // 0x40fa70, this module
extern int16_t actor_evaluate_flank_offset(real_vector3d *cover_direction, real_vector3d *out_offset,
    real_point3d *threat_position, real_point3d *candidate_position); // 0x420b10, ECX, EBX, ESI, EDI

// blam-cc: stack -> actor_index, query, count, candidates
// Two desirability terms. The first rewards staying close: candidates nearer the actor score
// higher, with the whole effect faded out as the threat gets past the maximum firing
// distance. The second asks the per-hazard rater about every kind 0 and kind 1 hazard the
// query collected; a strong kind 1 rating scores 0 and a complete absence of hazards scores
// the full 10, with kind 1 taking priority over kind 0.
void actor_score_firing_positions_by_history(datum_index actor_index,
                                             actor_firing_position_query *query,
                                             uint16_t count,
                                             actor_firing_position_candidate *candidates)
{
    ActorVariant *variant;
    actor_firing_position_candidate *c;
    float travel_weight;
    float fraction;
    float bonus;
    int16_t best_kind_0;
    int16_t best_kind_1;
    int16_t rating;
    int32_t i;
    int32_t j;

    variant = (ActorVariant *)actor_get_actor_definition(actor_index);

    travel_weight = 8.0f;
    if (query->have_target == 0 ||
        (query->target_distance <= variant->maximum_firing_distance &&
         (travel_weight = (1.0f - query->target_distance / variant->maximum_firing_distance) * 8.0f,
          travel_weight > 0.0f))) {
        for (i = 0; i < (int16_t)count; i++) {
            c = &candidates[i];
            if (c->valid != 0 && c->distance_from_actor < query->maximum_distance) {
                fraction = 1.0f - c->distance_from_actor / query->maximum_distance;
                if (fraction < 0.0f) {
                    fraction = 0.0f;
                }
                c->score = fraction * travel_weight + c->score;
            }
        }
    }

    if (query->hazard_count_kind_01 <= 0) {
        return;
    }

    for (i = 0; i < (int16_t)count; i++) {
        c = &candidates[i];
        if (c->valid == 0) {
            continue;
        }

        best_kind_0 = 0;
        best_kind_1 = 0;

        if (query->hazard_count < 1) {
            bonus = 10.0f;
        } else {
            for (j = 0; j < query->hazard_count; j++) {
                int16_t kind = query->hazards[j].kind;
                if (kind != 0 && kind != 1) {
                    continue;
                }
                // FIXED (objdump 0x41200c..0x412021): ECX = &hazard.direction, EBX = 0, ESI = the candidate firing
                //   position (c->position), EDI = &hazard.position. The draft passed no arguments.
                rating = actor_evaluate_flank_offset(&query->hazards[j].direction, 0,
                    (real_point3d *)c->position, &query->hazards[j].position);
                kind = query->hazards[j].kind;
                if (kind == 0) {
                    if (rating > best_kind_0) {
                        best_kind_0 = rating;
                    }
                } else if (kind == 1 && best_kind_1 <= rating) {
                    best_kind_1 = rating;
                }
            }

            if (best_kind_1 >= 2) {
                bonus = 0.0f;
            } else if (best_kind_1 >= 1) {
                bonus = 1.5f;
            } else if (best_kind_0 >= 2) {
                bonus = 6.0f;
            } else if (best_kind_0 >= 1) {
                bonus = 8.5f;
            } else {
                bonus = 10.0f;
            }
        }

        c->score = bonus + c->score;
    }
}

#if 0
Original Ghidra decompilation (0x411ee0):

void FUN_00411ee0(undefined4 param_1,int param_2,ushort param_3,int param_4)

{
  short sVar1;
  float fVar2;
  short sVar3;
  short sVar4;
  short sVar5;
  short sVar6;
  short sVar7;
  int iVar8;
  uint uVar9;
  float *pfVar10;
  uint local_4;

  iVar8 = actor_get_actor_definition();
  param_1 = 8.0;
  if (((*(char *)(param_2 + 0x5fc) == '\0') ||
      ((*(float *)(param_2 + 0x600) <= *(float *)(iVar8 + 0x74) &&
       (param_1 = (1.0 - *(float *)(param_2 + 0x600) / *(float *)(iVar8 + 0x74)) * 8.0,
       0.0 < param_1)))) && (0 < (short)param_3)) {
    pfVar10 = (float *)(param_4 + 8);
    uVar9 = (uint)param_3;
    do {
      if ((*(char *)(pfVar10 + 10) != '\0') && (*pfVar10 < *(float *)(param_2 + 0x18))) {
        fVar2 = 1.0 - *pfVar10 / *(float *)(param_2 + 0x18);
        if (fVar2 < 0.0) {
          fVar2 = 0.0;
        }
        pfVar10[0xc] = fVar2 * param_1 + pfVar10[0xc];
      }
      pfVar10 = pfVar10 + 0xf;
      uVar9 = uVar9 - 1;
    } while (uVar9 != 0);
  }
  if ((0 < *(short *)(param_2 + 0x256)) && (0 < (short)param_3)) {
    local_4 = (uint)param_3;
    pfVar10 = (float *)(param_4 + 0x38);
    do {
      sVar3 = 0;
      sVar5 = 0;
      if (*(char *)(pfVar10 + -2) != '\0') {
        sVar7 = 0;
        if (*(short *)(param_2 + 0x254) < 1) {
LAB_004120b8:
          fVar2 = 10.0;
        }
        else {
          do {
            sVar1 = *(short *)(sVar7 * 0x1c + 0x25c + param_2);
            sVar4 = sVar5;
            if ((sVar1 == 0) || (sVar1 == 1)) {
              sVar6 = FUN_00420b10();
              sVar1 = *(short *)(sVar7 * 0x1c + param_2 + 0x25c);
              if (sVar1 == 0) {
                sVar4 = sVar6;
                if (sVar6 < sVar5) {
                  sVar4 = sVar5;
                }
              }
              else if ((sVar1 == 1) && (sVar3 <= sVar6)) {
                sVar3 = sVar6;
              }
            }
            sVar7 = sVar7 + 1;
            sVar5 = sVar4;
          } while (sVar7 < *(short *)(param_2 + 0x254));
          if (sVar3 < 2) {
            if (sVar3 < 1) {
              if (sVar4 < 2) {
                if (sVar4 < 1) goto LAB_004120b8;
                fVar2 = 8.5;
              }
              else {
                fVar2 = 6.0;
              }
            }
            else {
              fVar2 = 1.5;
            }
          }
          else {
            fVar2 = 0.0;
          }
        }
        *pfVar10 = fVar2 + *pfVar10;
      }
      pfVar10 = pfVar10 + 0xf;
      local_4 = local_4 - 1;
    } while (local_4 != 0);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
