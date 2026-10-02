// ai_communication_rate_player_proximity  (Ghidra: ai_communication_rate_player_proximity; named for this rewrite)
// address 0x4303f0, size 965 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: the function iterates player_data (0x0087a480, the array every other file in
// this repo calls `player_data`) and scores each live player's unit by distance from the
// object passed in EBX, so the phase-4 one-line summary ("desirability/weight based on
// nearby group members' exposure") is wrong about *whose* exposure: it is the human
// players'. Its three callers are all in the AI communication/conversation cluster
// (ai_communication_broadcast 0x42d340, ai_communication_rate_speaker 0x42fb90 and
// ai_conversation_resolve_participant 0x431680), which is exactly where "is a player close
// enough, and looking, for this line to be worth playing" belongs.
// register convention: EBX -> object_index (recovered from `push ebx` at 0x430407, which
// supplies the first argument of object_get_node_local_transform); param_1..param_3 are
// genuine stack arguments.
// blam-cc: EBX -> object_index, stack -> require_line_of_sight, out_player_object_index,
//          out_distance
//
// UNSURE: every magic number below (900.0 = 30 world units squared, the 15/3 unit distance
// ramp, the 0.70710677 = cos 45 degrees facing gate, the 0.7/3.4142134/0.35 facing bonus)
// is transcribed from the original; none of them is named anywhere in the image.
// UNSURE: the chain walk on object+0x11c (parent_object) that resolves both cluster indices
// climbs to the LAST object whose parent is -1, i.e. the root; reproduced literally.
// reconciled: R16 the local 0x10-byte iterator shadow struct is now types/memory.h data_iterator (same layout)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"
#include <stdint.h>
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
// REWRITTEN 2026-09-28 against objdump 0x4303f0..0x4307b4: when either root cluster is -1 the original
//   skips only the PVS test and still traces the segment (the draft dropped the player entirely);
//   distance and facing sums follow the original z, y, x order. Everything else matched.


extern data_array *player_data;    // 0x0087a480, stride 0x200 (no types/players.h yet)
extern data_array *object_data;    // 0x008603b0
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, the current structure BSP tag data
extern char ai_marker_name_a[];    // 0x0066bfa0

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                               object_marker *marker, uint32_t flags); // 0x4f6080
// 0x505880, not this module: the generic collision/trace request. Established by
// src/ai/actor_evaluate_engagement_reachability.c and src/ai/actor_grenade_parabolic_path_clear.c.
extern int8_t collision_test_movement_segment(uint32_t flags, real_point3d *origin, real_vector3d *delta,
                                    uint32_t exclude_object, void *scratch); // 0x505880

// blam-cc: EBX -> object_index, stack -> require_line_of_sight, out_player_object_index,
//          out_distance
// Scores how "interesting" object_index is to the human players: 1.0 when no player is
// within 30 world units (and also when there are no players at all), rising towards ~2.5 as
// the nearest player closes to 3 units, with a +0.5 bonus when line of sight is clear (only
// evaluated when require_line_of_sight is set) and a further bonus when that player is
// actually aiming at the object. Optionally reports the winning player's unit object index
// and its distance.
float ai_communication_rate_player_proximity(uint8_t require_line_of_sight,
                                             datum_index *out_player_object_index,
                                             float *out_distance,
                                             datum_index object_index /* EBX */)
{
    object_marker self_marker;
    object_marker player_marker;
    real_point3d self_position;
    real_point3d player_position;
    real_vector3d to_self;
    // The original builds the iterator inline: [data, int16 next_index, index, signature],
    // the types/memory.h data_iterator (next_index is written as a WORD, pad_06 untouched).
    data_iterator iterator;
    void *player;
    datum_index best_object_index;
    float best_score;
    float best_distance;
    uint8_t saw_any_player;
    uint8_t line_of_sight_clear;
    float dx, dy, dz, distance_squared, distance, score, facing;
    uint32_t walk, previous;
    int16_t self_cluster, player_cluster;
    int32_t bitmap_row_dwords;
    object *player_object;
    uint8_t trace_scratch[96];

    best_object_index = (datum_index)k_datum_index_none;
    best_score = 0.0f;
    best_distance = 3.4028235e+38f;
    saw_any_player = 0;

    object_get_node_local_transform(object_index, ai_marker_name_a, &self_marker, 1);
    self_position = self_marker.node_transform.position;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    player = data_iterator_next(&iterator);
    if (player != 0) {
        do {
            // player+0x34 is the player's controlled unit object index.
            if (((struct player *)player)->unit != (datum_index)k_datum_index_none) {
                datum_index player_unit = ((struct player *)player)->unit;

                saw_any_player = 1;
                object_get_node_local_transform(player_unit, ai_marker_name_a, &player_marker, 1);
                player_position = player_marker.node_transform.position;
                dx = self_position.x - player_position.x;
                dy = self_position.y - player_position.y;
                dz = self_position.z - player_position.z;
                distance_squared = dz * dz + dy * dy + dx * dx; // 0x430506: summed z, y, x
                if (distance_squared < 900.0f) {
                    line_of_sight_clear = 0;
                    if (require_line_of_sight != 0) {
                        // Resolve both ends to their root object, then to that root's
                        // location_cluster_index (+0x9c), and reject the pair outright when
                        // the BSP cluster PVS says the two clusters cannot see each other.
                        previous = (uint32_t)k_datum_index_none;
                        if (object_index != (datum_index)k_datum_index_none) {
                            walk = (uint32_t)object_index;
                            do {
                                previous = walk;
                                walk = (uint32_t)((object_header *)object_data->data)
                                           [previous & 0xffff].data->parent_object;
                            } while (walk != (uint32_t)k_datum_index_none);
                        }
                        self_cluster = *(int16_t *)((uint8_t *)((object_header *)object_data->data)
                                                        [previous & 0xffff].data + 0x9c);
                        walk = (uint32_t)player_unit;
                        previous = (uint32_t)k_datum_index_none;
                        while (walk != (uint32_t)k_datum_index_none) {
                            previous = walk;
                            walk = (uint32_t)((object_header *)object_data->data)
                                       [previous & 0xffff].data->parent_object;
                        }
                        player_cluster = *(int16_t *)((uint8_t *)((object_header *)object_data->data)
                                                          [previous & 0xffff].data + 0x9c);
                        // 0x4305bf: an unknown cluster on either end skips only the PVS test (je 0x430613),
                        // not the player -- the segment trace below still decides line of sight.
                        if (self_cluster != -1 && player_cluster != -1) {
                        // ScenarioStructureBSP.clusters.count (+0x134) and
                        // .cluster_data.pointer (+0x14c, the row-major cluster visibility
                        // bitmap); the row stride is ceil(count/32) dwords. Same access as
                        // src/ai/actor_target_scan_potential_targets.c.
                        bitmap_row_dwords = (int32_t)(global_structure_bsp->clusters.count + 0x1f) >> 5;
                        if ((((uint32_t *)(uintptr_t)global_structure_bsp->cluster_data.pointer)
                                 [bitmap_row_dwords * (int32_t)self_cluster +
                                  ((int32_t)player_cluster >> 5)] &
                             (1u << ((uint8_t)player_cluster & 0x1f))) == 0) {
                            goto advance;
                        }
                        }
                        to_self.i = dx;
                        to_self.j = dy;
                        to_self.k = dz;
                        // 0x27 is this module's standard "line of sight" trace mask.
                        line_of_sight_clear =
                            (distance_squared < 9.0f ||
                             collision_test_movement_segment(0x27, &player_position, &to_self,
                                                   (uint32_t)k_datum_index_none, trace_scratch) == 0)
                                ? 1 : 0;
                    }

                    distance = (float)sqrt((double)distance_squared);
                    score = 1.0f;
                    if (distance < 15.0f) {
                        if (3.0f <= distance) {
                            score = (15.0f - distance) * 0.083333336f + 1.0f;
                        } else {
                            score = 2.0f;
                        }
                        if (line_of_sight_clear) {
                            score = score + 0.5f;
                        }
                        if (0.0001f < distance) {
                            player_object = ((object_header *)object_data->data)
                                                [player_unit & 0xffff].data;
                            facing = (((unit_data *)((uint8_t *)player_object + k_unit_data_offset))->aiming_vector.k * dz +
                                      ((unit_data *)((uint8_t *)player_object + k_unit_data_offset))->aiming_vector.j * dy +
                                      ((unit_data *)((uint8_t *)player_object + k_unit_data_offset))->aiming_vector.i * dx) / distance; // 0x4306f3
                            if (0.70710677f < facing) {
                                score = (0.7f - (1.0f - facing) * 3.4142134f * 0.35f) + score;
                            }
                        }
                    }
                    if (best_score < score) {
                        best_object_index = player_unit;
                        best_score = score;
                        best_distance = distance;
                    }
                }
            }
advance:
            player = data_iterator_next(&iterator);
        } while (player != 0);
        if (saw_any_player) {
            goto done;
        }
    }
    best_score = 1.0f;
done:
    if (out_distance != 0) {
        *out_distance = best_distance;
    }
    if (out_player_object_index != 0) {
        *out_player_object_index = best_object_index;
    }
    return best_score;
}

#if 0
Original Ghidra decompilation (0x4303f0):

float10 FUN_004303f0(char param_1,undefined4 *param_2,float *param_3)

{
  short sVar1;
  short sVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  bool bVar11;
  bool bVar12;
  uint uVar13;
  char cVar14;
  int iVar15;
  uint uVar16;
  uint unaff_EBX;
  float local_13c;
  undefined4 local_12c;
  float local_11c;
  float local_118;
  float local_114;
  float local_110;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_fc;
  float local_f8;
  uint local_f4;
  undefined2 local_f0;
  undefined4 local_ec;
  uint local_e8;
  float local_e4;
  float local_e0;
  float local_dc;
  undefined1 local_d8 [96];
  float local_78;
  float local_74;
  float local_70;
  undefined1 local_6c [96];
  float local_c;
  float local_8;
  float local_4;

  local_12c = 0xffffffff;
  local_13c = 0.0;
  local_11c = 3.4028235e+38;
  bVar12 = false;
  object_get_node_local_transform();
  local_104 = local_70;
  local_f4 = DAT_0087a480;
  local_e8 = DAT_0087a480 ^ 0x69746572;
  local_10c = local_78;
  local_108 = local_74;
  local_f0 = 0;
  local_ec = 0xffffffff;
  iVar15 = data_iterator_next();
  if (iVar15 != 0) {
    do {
      if (*(int *)(iVar15 + 0x34) != -1) {
        bVar12 = true;
        object_get_node_local_transform(*(int *)(iVar15 + 0x34),&DAT_0066bfa0,local_6c,1);
        iVar5 = DAT_008603b0;
        local_118 = local_c;
        fVar6 = local_10c - local_c;
        local_114 = local_8;
        local_110 = local_4;
        fVar7 = local_108 - local_8;
        fVar8 = local_104 - local_4;
        fVar9 = fVar6 * fVar6 + fVar7 * fVar7 + fVar8 * fVar8;
        if (fVar9 < 900.0) {
          bVar11 = false;
          if (param_1 != '\0') {
            uVar16 = 0xffffffff;
            if (unaff_EBX != 0xffffffff) {
              uVar3 = unaff_EBX;
              do {
                uVar16 = uVar3;
                uVar3 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                          (uVar16 & 0xffff) * 0xc) + 0x11c);
              } while (uVar3 != 0xffffffff);
            }
            iVar4 = *(int *)(DAT_008603b0 + 0x34);
            sVar1 = *(short *)(*(int *)(iVar4 + 8 + (uVar16 & 0xffff) * 0xc) + 0x9c);
            uVar3 = *(uint *)(iVar15 + 0x34);
            uVar16 = 0xffffffff;
            while (uVar13 = uVar3, uVar13 != 0xffffffff) {
              uVar16 = uVar13;
              uVar3 = *(uint *)(*(int *)(iVar4 + 8 + (uVar13 & 0xffff) * 0xc) + 0x11c);
            }
            sVar2 = *(short *)(*(int *)(iVar4 + 8 + (uVar16 & 0xffff) * 0xc) + 0x9c);
            if (((sVar1 != -1) && (sVar2 != -1)) &&
               ((*(uint *)(*(int *)(DAT_00746f9c + 0x14c) +
                          ((*(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5) * (int)sVar1 +
                          ((int)sVar2 >> 5)) * 4) & 1 << ((byte)sVar2 & 0x1f)) == 0))
            goto LAB_00430768;
            local_e4 = fVar6;
            local_e0 = fVar7;
            local_dc = fVar8;
            cVar14 = FUN_00505880(0x27,&local_118,&local_e4,0xffffffff,local_d8);
            if ((fVar9 < 9.0) || (cVar14 == '\0')) {
              bVar11 = true;
            }
            else {
              bVar11 = false;
            }
          }
          fVar9 = SQRT(fVar9);
          fVar10 = 1.0;
          if (fVar9 < 15.0) {
            if (3.0 <= fVar9) {
              fVar10 = (15.0 - fVar9) * 0.083333336 + 1.0;
            }
            else {
              fVar10 = 2.0;
            }
            if (bVar11) {
              fVar10 = fVar10 + 0.5;
            }
            if (0.0001 < fVar9) {
              iVar5 = *(int *)(*(int *)(iVar5 + 0x34) + 8 +
                              (*(uint *)(iVar15 + 0x34) & 0xffff) * 0xc);
              local_100 = *(float *)(iVar5 + 0x23c);
              local_fc = *(float *)(iVar5 + 0x240);
              local_f8 = *(float *)(iVar5 + 0x244);
              fVar6 = (local_100 * fVar6 + local_fc * fVar7 + local_f8 * fVar8) / fVar9;
              if (0.70710677 < fVar6) {
                fVar10 = (0.7 - (1.0 - fVar6) * 3.4142134 * 0.35) + fVar10;
              }
            }
          }
          if (local_13c < fVar10) {
            local_12c = *(undefined4 *)(iVar15 + 0x34);
            local_13c = fVar10;
            local_11c = fVar9;
          }
        }
      }
LAB_00430768:
      iVar15 = data_iterator_next();
    } while (iVar15 != 0);
    if (bVar12) goto LAB_0043078c;
  }
  local_13c = 1.0;
LAB_0043078c:
  if (param_3 != (float *)0x0) {
    *param_3 = local_11c;
  }
  if (param_2 != (undefined4 *)0x0) {
    *param_2 = local_12c;
  }
  return (float10)local_13c;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
