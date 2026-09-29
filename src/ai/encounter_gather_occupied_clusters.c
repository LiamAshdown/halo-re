// encounter_gather_occupied_clusters  (Ghidra: encounter_gather_occupied_clusters; named for this rewrite)
// address 0x436190, size 1147 bytes
// name confidence: 0.45   rewrite confidence: 0.45
// evidence: phase-4 summary ("computes (and optionally records per-actor) the set of BSP
//   zones/clusters that a squad's living actors currently occupy, expanded through
//   door/portal adjacency tables"). The bitmap it fills is one bit per structure BSP cluster
//   (the count comes from global_structure_bsp + 0x134), the same bitmap encounters_update_activation
//   @0x437e20 intersects against the per-player visible-cluster mask at
//   local_player_globals + 0x18. Each member's cluster comes from object + 0x9c after
//   following object + 0x11c to the root of the parent chain.
// register convention: EAX -> encounter_index, EBX -> out_clusters, stack ->
//   (record_per_actor, other_clusters).
//   // blam-cc: EAX -> encounter_index, EBX -> out_clusters,
//   //   stack -> (record_per_actor, other_clusters)
//
// UNSURE:
//  - Swarm members (actor.swarm set) contribute one cluster per unit on the
//    actor.cluster_unit_index chain (object + 0x1fc), not one per actor.
//  - The two accumulator masks are per-squad (local_10, one bit per squad index) and a
//    32-bit "zone" mask (local_14) built from the three ScenarioSquad fields at +0x6c and
//    +0x54 + 4 * (2 or 5); after the walk they are expanded through
//    ScenarioEncounter.firing_positions (stride 0x18, group at +0x0c, cluster at +0x0e) and
//    through each squad's move_positions (0xc4 count / 0xc8 address, stride 0x50, cluster at
//    +0x28). The exact meaning of the two is not established.
//  - Ghidra's rendering reuses iVar7 as both the firing-position loop index and the squad
//    pointer, which makes the outer loops look like they terminate early. Preserved with
//    separate variables here, which is the only reading that type-checks.
//  - actor_get_firing_position_group_mask(0) is an actor-side helper that returns a further zone mask.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "structures.h"
#include "ai.h"

extern data_array *encounter_data;                    // 0x008802c8
extern Scenario *global_scenario;                     // 0x00746f8c
extern ScenarioStructureBSP *global_structure_bsp;           // 0x00746f9c
extern data_array *actor_data;                        // 0x00880360
extern data_array *object_data;                       // 0x008603b0
extern data_array *prop_data;                         // 0x008802c0
extern encounter_squad_state *encounter_squad_states; // 0x008802cc

extern uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind,
    int16_t search_override); // 0x412880, EAX, SI, stack

// blam-cc: EAX -> encounter_index, EBX -> out_clusters, stack -> (record_per_actor, other_clusters)
// Fills out_clusters with one bit per BSP cluster that a live member of the encounter
// occupies, expanded through the encounter's firing positions and its squads' move
// positions. When record_per_actor is set, each member's actor + 0x12 is set to "this member
// is not in a cluster the caller's other_clusters mask already covers".
void encounter_gather_occupied_clusters(datum_index encounter_index, uint32_t *out_clusters,
                                        uint8_t record_per_actor, uint32_t *other_clusters)
{
    ScenarioEncounter *definition;
    encounter *enc;
    actor *a;
    object *obj;
    uint32_t *fill;
    uint32_t dword_count;
    uint32_t squad_mask;
    uint32_t zone_mask;
    uint32_t extra;
    datum_index actor_index;
    datum_index current;
    datum_index object_index;
    datum_index root_index;
    datum_index child_index;
    int16_t cluster;
    uint32_t bit;
    uint8_t visible;
    int16_t i;
    int32_t j;
    ScenarioSquad *squad;
    ScenarioFiringPosition *positions;
    int16_t position_cluster;

    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];
    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];

    zone_mask = 0;
    squad_mask = 0;

    fill = out_clusters;
    // ScenarioStructureBSP.clusters.count lives at +0x134; types/tags.h has no offset
    // comments for that struct, so it is read raw here.
    for (dword_count = (uint32_t)(((*(int32_t *)((uint8_t *)global_structure_bsp + 0x134)) + 0x1f) >> 5);
         dword_count != 0; dword_count = dword_count - 1) {
        *fill = 0;
        fill = fill + 1;
    }

    actor_index = enc->first_actor;
    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    do {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];
        visible = 1;

        if (a->swarm == 0) {
            root_index = (datum_index)k_datum_index_none;
            if (a->unit_index != (datum_index)k_datum_index_none) {
                object_index = a->unit_index;
                do {
                    root_index = object_index;
                    obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
                    object_index = ((object *)obj)->parent_object;
                } while (object_index != (datum_index)k_datum_index_none);
            }
            obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
            cluster = obj->location_cluster_index;
            if (cluster != -1) {
                bit = 1 << (cluster & 0x1f);
                out_clusters[(int32_t)cluster >> 5] =
                    out_clusters[(int32_t)cluster >> 5] | bit;
                if (other_clusters != 0 &&
                    (other_clusters[(int32_t)cluster >> 5] & bit) != 0) {
                    visible = 0;
                }
            }

            if (enc->units_active != 0) {
                if (a->awareness_level == 3) {
                    if (1 < a->alert_level) {
                        extra = 0;
                        if (a->encounter_index != (datum_index)k_datum_index_none) {
                            squad = &((ScenarioSquad *)
                                ((ScenarioEncounter *)global_scenario->encounters.pointer)
                                    [a->encounter_index & 0xffff].squads.pointer)[a->squad_index];
                            extra = *(uint32_t *)&((struct ScenarioSquad *)squad)->pursuing;
                        }
                        zone_mask = zone_mask | extra;
                    }
                    if (a->mode == 6 || a->mode == 4) {
                        extra = 0;
                        if (a->encounter_index != (datum_index)k_datum_index_none) {
                            squad = &((ScenarioSquad *)
                                ((ScenarioEncounter *)global_scenario->encounters.pointer)
                                    [a->encounter_index & 0xffff].squads.pointer)[a->squad_index];
                            extra = *(uint32_t *)((uint8_t *)squad + 0x54 +
                                (int16_t)((-(uint16_t)(a->platoon_defending != 0) & 3) + 2) * 4);
                        }
                        zone_mask = zone_mask | extra;
                    } else if (a->mode == 3 || a->mode == 5) {
                        // FIXED (0x4363de..0x4363e2): EAX = this actor, SI = 0, stack = 0
                        zone_mask = zone_mask | actor_get_firing_position_group_mask(current, 0, 0);
                    }
                } else if (a->awareness_level == 2 && *(int16_t *)(a->mode_data.raw + 0) != 0) {
                    squad_mask = squad_mask | (1 << (a->squad_index & 0x1f));
                }

                if (a->target_unit_index != (datum_index)k_datum_index_none) {
                    cluster = ((prop *)prop_data->data)
                        [a->target_unit_index & 0xffff].cluster_index;
                    if (cluster != -1) {
                        out_clusters[(int32_t)cluster >> 5] =
                            out_clusters[(int32_t)cluster >> 5] | (1 << (cluster & 0x1f));
                    }
                }
            }
        } else {
            child_index = a->cluster_unit_index;
            while (child_index != (datum_index)k_datum_index_none) {
                object *child = ((object_header *)object_data->data)[child_index & 0xffff].data;
                root_index = (datum_index)k_datum_index_none;
                object_index = child_index;
                for (; object_index != (datum_index)k_datum_index_none;
                     object_index = *(datum_index *)((uint8_t *)((object_header *)
                         object_data->data)[object_index & 0xffff].data + 0x11c)) {
                    root_index = object_index;
                }
                obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
                cluster = obj->location_cluster_index;
                if (cluster != -1) {
                    bit = 1 << (cluster & 0x1f);
                    out_clusters[(int32_t)cluster >> 5] =
                        out_clusters[(int32_t)cluster >> 5] | bit;
                    if (other_clusters != 0 &&
                        (other_clusters[(int32_t)cluster >> 5] & bit) != 0) {
                        visible = 0;
                    }
                }
                child_index = *(datum_index *)((uint8_t *)child + 0x1fc);
            }
        }

        if (record_per_actor != 0) {
            if (encounter_squad_states[(int16_t)(a->squad_index + enc->first_squad)].dormant_disallowed
                != 0) {
                visible = 0;
            }
            a->unknown_12 = visible;
        }
        actor_index = a->next_in_encounter;
    } while (actor_index != (datum_index)k_datum_index_none);

    if (zone_mask != 0 && 0 < definition->firing_positions.count) {
        positions = (ScenarioFiringPosition *)definition->firing_positions.pointer;
        i = 0;
        j = 0;
        do {
            if ((int16_t)positions[j].cluster_index != -1 &&
                (zone_mask & (1 << (positions[j].group_index & 0x1f))) != 0) {
                position_cluster = (int16_t)positions[j].cluster_index;
                out_clusters[(int32_t)position_cluster >> 5] =
                    out_clusters[(int32_t)position_cluster >> 5] |
                    (1 << (position_cluster & 0x1f));
            }
            i = i + 1;
            j = (int32_t)i;
        } while (j < definition->firing_positions.count);
    }

    if (squad_mask != 0 && 0 < definition->squads.count) {
        i = 0;
        j = 0;
        do {
            if ((squad_mask & (1 << (j & 0x1f))) != 0) {
                squad = &((ScenarioSquad *)definition->squads.pointer)[j];
                if (0 < squad->move_positions.count) {
                    int16_t k = 0;
                    int32_t m = 0;
                    do {
                        position_cluster = *(int16_t *)((uint8_t *)squad->move_positions.pointer +
                            m * 0x50 + 0x28);
                        if (position_cluster != -1) {
                            out_clusters[(int32_t)position_cluster >> 5] =
                                out_clusters[(int32_t)position_cluster >> 5] |
                                (1 << (position_cluster & 0x1f));
                        }
                        k = k + 1;
                        m = (int32_t)k;
                    } while (m < squad->move_positions.count);
                }
            }
            i = i + 1;
            j = (int32_t)i;
        } while (j < definition->squads.count);
    }
}

#if 0
Original Ghidra decompilation (0x436190):

void FUN_00436190(char param_1,int param_2)

{
  short sVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  uint uVar8;
  undefined4 *unaff_EBX;
  int iVar9;
  short sVar10;
  undefined4 *puVar11;
  undefined1 local_15;
  uint local_14;
  uint local_10;

  iVar3 = (in_EAX & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  local_10 = 0;
  local_14 = 0;
  iVar7 = (in_EAX & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
  puVar11 = unaff_EBX;
  for (uVar4 = *(int *)(DAT_00746f9c + 0x134) + 0x1f >> 5 & 0x3fffffff; uVar4 != 0;
      uVar4 = uVar4 - 1) {
    *puVar11 = 0;
    puVar11 = puVar11 + 1;
  }
  for (iVar5 = 0; iVar5 != 0; iVar5 = iVar5 + -1) {
    *(undefined1 *)puVar11 = 0;
    puVar11 = (undefined4 *)((int)puVar11 + 1);
  }
  uVar4 = *(uint *)(iVar7 + 0x14);
  if (uVar4 != 0xffffffff) {
    do {
      iVar5 = (uVar4 & 0xffff) * 0x724;
      iVar9 = *(int *)(DAT_00880360 + 0x34) + iVar5;
      local_15 = 1;
      if (*(char *)(*(int *)(DAT_00880360 + 0x34) + 6 + iVar5) == '\0') {
        uVar4 = 0xffffffff;
        if (*(uint *)(iVar9 + 0x18) != 0xffffffff) {
          uVar8 = *(uint *)(iVar9 + 0x18);
          do {
            uVar4 = uVar8;
            uVar8 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) +
                             0x11c);
          } while (uVar8 != 0xffffffff);
        }
        sVar6 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar4 & 0xffff) * 0xc) +
                          0x9c);
        if (sVar6 != -1) {
          uVar4 = 1 << ((byte)sVar6 & 0x1f);
          unaff_EBX[(int)sVar6 >> 5] = unaff_EBX[(int)sVar6 >> 5] | uVar4;
          if ((param_2 != 0) && ((*(uint *)(((int)sVar6 >> 5) * 4 + param_2) & uVar4) != 0)) {
            local_15 = 0;
          }
        }
        if (*(char *)(iVar7 + 0xd) != '\0') {
          if (*(short *)(iVar9 + 0x6a) == 3) {
            if (1 < *(short *)(iVar9 + 0x6e)) {
              uVar4 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x34 + iVar5);
              uVar8 = 0;
              if (uVar4 != 0xffffffff) {
                uVar8 = *(uint *)(*(short *)(*(int *)(DAT_00880360 + 0x34) + iVar5 + 0x3a) * 0xe8 +
                                  *(int *)((uVar4 & 0xffff) * 0xb0 + 0x84 +
                                          *(int *)(global_scenario + 0x430)) + 0x6c);
              }
              local_14 = local_14 | uVar8;
            }
            sVar6 = *(short *)(iVar9 + 0x6c);
            if ((sVar6 == 6) || (sVar6 == 4)) {
              uVar4 = *(uint *)(*(int *)(DAT_00880360 + 0x34) + 0x34 + iVar5);
              iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar5;
              uVar8 = 0;
              if (uVar4 != 0xffffffff) {
                uVar8 = *(uint *)(*(short *)(iVar5 + 0x3a) * 0xe8 +
                                  *(int *)((uVar4 & 0xffff) * 0xb0 + 0x84 +
                                          *(int *)(global_scenario + 0x430)) + 0x54 +
                                 (short)((-(ushort)(*(char *)(iVar5 + 0x374) != '\0') & 3) + 2) * 4)
                ;
              }
              local_14 = local_14 | uVar8;
            }
            else if ((sVar6 == 3) || (sVar6 == 5)) {
              uVar4 = FUN_00412880(0);
              local_14 = local_14 | uVar4;
            }
          }
          else if ((*(short *)(iVar9 + 0x6a) == 2) && (*(short *)(iVar9 + 0x9c) != 0)) {
            local_10 = local_10 | 1 << (*(byte *)(iVar9 + 0x3a) & 0x1f);
          }
          if ((*(uint *)(iVar9 + 0x270) != 0xffffffff) &&
             (sVar6 = *(short *)((*(uint *)(iVar9 + 0x270) & 0xffff) * 0x138 + 0x100 +
                                *(int *)(DAT_008802c0 + 0x34)), sVar6 != -1)) {
            unaff_EBX[(int)sVar6 >> 5] = unaff_EBX[(int)sVar6 >> 5] | 1 << ((byte)sVar6 & 0x1f);
          }
        }
      }
      else {
        uVar4 = *(uint *)(iVar9 + 0x24);
        while (uVar4 != 0xffffffff) {
          iVar5 = *(int *)(DAT_008603b0 + 0x34);
          iVar2 = *(int *)(iVar5 + 8 + (uVar4 & 0xffff) * 0xc);
          uVar8 = 0xffffffff;
          for (; uVar4 != 0xffffffff;
              uVar4 = *(uint *)(*(int *)(iVar5 + 8 + (uVar4 & 0xffff) * 0xc) + 0x11c)) {
            uVar8 = uVar4;
          }
          sVar6 = *(short *)(*(int *)(iVar5 + 8 + (uVar8 & 0xffff) * 0xc) + 0x9c);
          if (sVar6 != -1) {
            uVar4 = 1 << ((byte)sVar6 & 0x1f);
            unaff_EBX[(int)sVar6 >> 5] = unaff_EBX[(int)sVar6 >> 5] | uVar4;
            if ((param_2 != 0) && ((*(uint *)(((int)sVar6 >> 5) * 4 + param_2) & uVar4) != 0)) {
              local_15 = 0;
            }
          }
          uVar4 = *(uint *)(iVar2 + 0x1fc);
        }
      }
      if (param_1 != '\0') {
        if (*(char *)((short)(*(short *)(iVar9 + 0x3a) + *(short *)(iVar7 + 4)) * 0x20 + 0x14 +
                     DAT_008802cc) != '\0') {
          local_15 = 0;
        }
        *(undefined1 *)(iVar9 + 0x12) = local_15;
      }
      uVar4 = *(uint *)(iVar9 + 0x2c);
    } while (uVar4 != 0xffffffff);
    if ((local_14 != 0) && (sVar6 = 0, 0 < *(int *)(iVar3 + 0x98))) {
      iVar7 = 0;
      do {
        iVar5 = *(int *)(iVar3 + 0x9c) + iVar7 * 0x18;
        if ((*(short *)(*(int *)(iVar3 + 0x9c) + 0xe + iVar7 * 0x18) != -1) &&
           ((local_14 & 1 << (*(byte *)(iVar5 + 0xc) & 0x1f)) != 0)) {
          sVar10 = *(short *)(iVar5 + 0xe);
          iVar7 = (int)sVar10 >> 5;
          unaff_EBX[iVar7] = unaff_EBX[iVar7] | 1 << ((byte)sVar10 & 0x1f);
        }
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < *(int *)(iVar3 + 0x98));
    }
    if ((local_10 != 0) && (sVar6 = 0, 0 < *(int *)(iVar3 + 0x80))) {
      iVar7 = 0;
      do {
        if ((local_10 & 1 << ((byte)iVar7 & 0x1f)) != 0) {
          iVar7 = iVar7 * 0xe8 + *(int *)(iVar3 + 0x84);
          sVar10 = 0;
          if (0 < *(int *)(iVar7 + 0xc4)) {
            iVar5 = 0;
            do {
              iVar5 = iVar5 * 0x50 + *(int *)(iVar7 + 200);
              if (*(short *)(iVar5 + 0x28) != -1) {
                sVar1 = *(short *)(iVar5 + 0x28);
                iVar5 = (int)sVar1 >> 5;
                unaff_EBX[iVar5] = unaff_EBX[iVar5] | 1 << ((byte)sVar1 & 0x1f);
              }
              sVar10 = sVar10 + 1;
              iVar5 = (int)sVar10;
            } while (iVar5 < *(int *)(iVar7 + 0xc4));
          }
        }
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < *(int *)(iVar3 + 0x80));
    }
  }
  return;
}
#endif
