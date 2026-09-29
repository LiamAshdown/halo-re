// encounters_update_activation  (Ghidra: encounters_update_activation; named for this rewrite)
// address 0x437e20, size 1163 bytes (0x437e20..0x4382aa)
// name confidence: 0.5   rewrite confidence: 0.9 (verified branch by branch against objdump 0x437e20..0x4382aa)
// evidence: phase-4 summary ("master per-tick pass that updates every actor's
//   cluster-visibility/active state and decides which squads should be activated or
//   deactivated based on player-relevant BSP clusters and dependent squads"). The mask it
//   tests against is local_player_globals + 0x18 (the per-player visible-cluster bitmap),
//   and the per-encounter bitmap comes from encounter_gather_occupied_clusters @0x436190,
//   whose register convention is confirmed here (objdump -d -M intel
//   --start-address=0x4381b3 --stop-address=0x4381c5 bin/halo.exe: push [esp+0x18] /
//   push 1 / lea ebx,[esp+0x40] / mov eax,ebp / call 0x436190).
// register convention: no arguments.
//
// UNSURE:
//  - Ghidra prints the ScenarioEncounter lookup in the encounter loop as
//    `*(short *)(encounters.pointer + 0xafffce)`; 0xafffce is 0xb0 * 0xffff + 0x7e, i.e. the
//    decompiler constant-folded `(index & 0xffff) * 0xb0 + 0x7e` with index left at its -1
//    initial value. The disassembly at 0x438183 shows the real index being loaded from the
//    iterator, so it is `encounters[iterator.index].precomputed_bsp_index`.
//  - encounter + 0x20 is a count and encounter + 0x22 an array of uint16 encounter indices
//    the encounter depends on; the encounter struct has room for at most three of them
//    (0x22, 0x24, 0x26), which types/ai.h lists as three separate unknown int16s.
//  - The iterator here is the plain 0x10-byte data_iterator + signature form (no active-only
//    filter byte), so only the first four fields of encounter_iterator are used.
//  - actor_create_swarm / actor_delete_swarm / actor_set_units_active / actor_clear_perceived_props are all
//    argument-less in Ghidra and are written as taking the actor they act on.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#include "ai.h"
#include "fn_ai.h"

extern ScenarioStructureBSP *global_structure_bsp;      // 0x00746f9c
extern ai_globals *ai_globals_ptr;               // 0x00880354
extern player_globals *local_player_globals;     // 0x0087a478
extern data_array *actor_data;                   // 0x00880360
extern data_array *object_data;                  // 0x008603b0
extern data_array *swarm_data;                   // 0x0088035c
extern game_time_globals *game_time;             // 0x006f1d6c
extern data_array *encounter_data;               // 0x008802c8
extern Scenario *global_scenario;                // 0x00746f8c
extern int16_t global_structure_bsp_index;                // 0x0069e8d8


extern void actor_create_swarm(datum_index actor_index);     // 0x427f40, not yet rewritten


extern uint8_t encounter_activate(datum_index encounter_index);   // 0x437710, blam-cc: ECX -> encounter_index

extern void *data_iterator_next(data_iterator *iterator);         // 0x4d05d0, blam-cc: EDI -> iterator

// Refreshes every unassigned actor's "the player cannot see me" flag, activating or
// deactivating each one on a 30-tick hysteresis, then does the same per encounter using the
// encounter's own occupied-cluster bitmap and its list of dependent encounters.
void encounters_update_activation(void)
{
    uint32_t *visible_clusters;
    datum_index actor_index;
    datum_index current;
    actor *a;
    object *obj;
    swarm *sw;
    datum_index object_index;
    datum_index root_index;
    datum_index child_index;
    int16_t cluster;
    int16_t component_count;
    int16_t component_index;
    encounter_iterator iterator;
    encounter *enc;
    ScenarioEncounter *definition;
    uint32_t encounter_clusters[16];
    uint32_t dword_count;
    uint8_t overlaps;
    uint8_t wants_active;
    uint8_t any_dependent_pending;
    int16_t i;
    uint16_t *dependents;

    visible_clusters = (uint32_t *)((uint8_t *)local_player_globals + 0x18);

    actor_index = ai_globals_ptr->unknown_08;
    while (actor_index != (datum_index)k_datum_index_none) {
        current = actor_index;
        a = &((actor *)actor_data->data)[current & 0xffff];

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
            if (cluster == -1) {
                a->unknown_12 = 1;
            } else {
                a->unknown_12 = (uint8_t)(1 - (((1 << (cluster & 0x1f)) &
                    visible_clusters[(int32_t)cluster >> 5]) != 0));
            }
        } else {
            a->unknown_12 = 1;
            if (a->swarm_index == (datum_index)k_datum_index_none) {
                child_index = a->cluster_unit_index;
                if (child_index != (datum_index)k_datum_index_none) {
                    do {
                        root_index = (datum_index)k_datum_index_none;
                        for (object_index = child_index;
                             object_index != (datum_index)k_datum_index_none;
                             object_index = *(datum_index *)((uint8_t *)((object_header *)
                                 object_data->data)[object_index & 0xffff].data + 0x11c)) {
                            root_index = object_index;
                        }
                        obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
                        cluster = obj->location_cluster_index;
                        if (cluster != -1 &&
                            (visible_clusters[(int32_t)cluster >> 5] &
                             (1 << (cluster & 0x1f))) != 0) {
                            a->unknown_12 = 0;
                            break;
                        }
                        obj = ((object_header *)object_data->data)[child_index & 0xffff].data;
                        child_index = *(datum_index *)((uint8_t *)obj + 0x1fc);
                    } while (child_index != (datum_index)k_datum_index_none);
                }
            } else {
                sw = &((swarm *)swarm_data->data)[a->swarm_index & 0xffff];
                component_count = sw->component_count;
                component_index = 0;
                if (0 < component_count) {
                    do {
                        root_index = (datum_index)k_datum_index_none;
                        for (object_index = sw->unit_index[component_index];
                             object_index != (datum_index)k_datum_index_none;
                             object_index = *(datum_index *)((uint8_t *)((object_header *)
                                 object_data->data)[object_index & 0xffff].data + 0x11c)) {
                            root_index = object_index;
                        }
                        obj = ((object_header *)object_data->data)[root_index & 0xffff].data;
                        cluster = obj->location_cluster_index;
                        if (cluster != -1 &&
                            (visible_clusters[(int32_t)cluster >> 5] &
                             (1 << (cluster & 0x1f))) != 0) {
                            a->unknown_12 = 0;
                            break;
                        }
                        component_index = component_index + 1;
                    } while (component_index < component_count);
                }
            }
        }

        if (a->unknown_12 == 0 || a->force_active != 0) {
            *(int16_t *)&a->unknown_10[0] = 0x5a;
            if (a->active != 1) {
                if (a->swarm == 0 || (actor_create_swarm(current),
                                      a->swarm_index != (datum_index)k_datum_index_none)) {
                    a->active = 1;
                    if (a->awareness_level == 0) {
                        actor_set_units_active(current, 0);
                    }
                } else {
                    a->swarm_pending = 1;
                }
            }
        } else if (*(int16_t *)&a->unknown_10[0] < 0x1f) {
            *(int16_t *)&a->unknown_10[0] = 0;
            if (a->active != 0) {
                actor_clear_perceived_props(current);
                actor_delete_swarm(current);
                actor_set_units_active(current, 1);
                a->active = 0;
                a->deactivation_tick = (datum_index)game_time->game_time;
            }
        } else {
            *(int16_t *)&a->unknown_10[0] = (int16_t)(*(int16_t *)&a->unknown_10[0] - 0x1e);
        }
        actor_index = a->next_in_encounter;
    }

    iterator.data = encounter_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)encounter_data ^ 0x69746572;

    enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
    while (enc != 0) {
        definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
            [iterator.index & 0xffff];
        wants_active = (uint8_t)(0 < enc->reinforcement_delay || enc->force_active != 0);

        if ((int16_t)definition->precomputed_bsp_index == -1 ||
            (int16_t)definition->precomputed_bsp_index == global_structure_bsp_index) {

            encounter_gather_occupied_clusters(iterator.index, encounter_clusters, 1,
                                               visible_clusters);

            overlaps = 0;
            dword_count = (uint32_t)(((*(int16_t *)((uint8_t *)global_structure_bsp + 0x134)) + 0x1f) >> 5);
            i = (int16_t)dword_count - 1;
            if (0 <= i) {
                uint32_t n = dword_count & 0xffff;
                uint32_t *cursor = encounter_clusters + i;
                do {
                    if ((visible_clusters[cursor - encounter_clusters] & *cursor) != 0) {
                        overlaps = 1;
                    }
                    cursor = cursor - 1;
                    n = n - 1;
                } while (n != 0);
            }

            if (wants_active != 0 || overlaps != 0) {
                enc->activation_delay = 0x96;
                encounter_activate(iterator.index);
                enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
                continue;
            }
        }

        if (enc->units_active == 0 || enc->activation_delay <= 0x1e) {
            any_dependent_pending = 0;
            if (0 < enc->unknown_20) {
                dependents = (uint16_t *)&enc->unknown_22;
                for (i = enc->unknown_20; i != 0; i = i - 1) {
                    if (0 < ((encounter *)encounter_data->data)[*dependents].activation_delay) {
                        any_dependent_pending = 1;
                    }
                    dependents = dependents + 1;
                }
            }
            enc->activation_delay = 0;
            if (any_dependent_pending != 0) {
                encounter_activate(iterator.index);
            } else {
                encounter_deactivate(iterator.index);
            }
        } else {
            enc->activation_delay = (int16_t)(enc->activation_delay - 0x1e);
        }
        enc = (encounter *)data_iterator_next((data_iterator *)&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x437e20):

void FUN_00437e20(void)

{
  char cVar1;
  short sVar2;
  short sVar3;
  uint uVar4;
  bool bVar5;
  uint uVar6;
  int iVar7;
  short sVar8;
  int iVar9;
  uint uVar10;
  ushort *puVar11;
  int iVar12;
  uint uVar13;
  uint *puVar14;
  int iVar15;
  int iVar16;
  int iVar17;
  uint local_40 [16];

  iVar7 = DAT_00746f9c;
  iVar12 = DAT_0087a478 + 0x18;
  uVar10 = *(uint *)(DAT_00880354 + 8);
  while (uVar10 != 0xffffffff) {
    iVar17 = (uVar10 & 0xffff) * 0x724;
    iVar15 = *(int *)(DAT_00880360 + 0x34) + iVar17;
    if (*(char *)(*(int *)(DAT_00880360 + 0x34) + 6 + iVar17) == '\0') {
      uVar13 = 0xffffffff;
      if (*(uint *)(iVar15 + 0x18) != 0xffffffff) {
        uVar4 = *(uint *)(iVar15 + 0x18);
        do {
          uVar13 = uVar4;
          uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc) +
                           0x11c);
        } while (uVar4 != 0xffffffff);
      }
      sVar2 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc) +
                        0x9c);
      if (sVar2 == -1) {
        *(undefined1 *)(iVar15 + 0x12) = 1;
      }
      else {
        *(char *)(iVar15 + 0x12) =
             '\x01' - ((1 << ((byte)sVar2 & 0x1f) & *(uint *)(iVar12 + ((int)sVar2 >> 5) * 4)) != 0)
        ;
      }
    }
    else {
      *(undefined1 *)(iVar15 + 0x12) = 1;
      if (*(uint *)(iVar15 + 0x28) == 0xffffffff) {
        uVar13 = *(uint *)(iVar15 + 0x24);
        if (uVar13 != 0xffffffff) {
          iVar9 = *(int *)(DAT_008603b0 + 0x34);
          do {
            uVar4 = 0xffffffff;
            for (uVar6 = uVar13; uVar6 != 0xffffffff;
                uVar6 = *(uint *)(*(int *)(iVar9 + 8 + (uVar6 & 0xffff) * 0xc) + 0x11c)) {
              uVar4 = uVar6;
            }
            iVar16 = *(int *)(iVar9 + 8 + (uVar4 & 0xffff) * 0xc);
            if ((*(short *)(iVar16 + 0x9c) != -1) &&
               (sVar2 = *(short *)(iVar16 + 0x9c),
               (*(uint *)(iVar12 + ((int)sVar2 >> 5) * 4) & 1 << ((byte)sVar2 & 0x1f)) != 0)) {
              *(undefined1 *)(iVar15 + 0x12) = 0;
              break;
            }
            uVar13 = *(uint *)(*(int *)(iVar9 + 8 + (uVar13 & 0xffff) * 0xc) + 0x1fc);
          } while (uVar13 != 0xffffffff);
        }
      }
      else {
        iVar9 = (*(uint *)(iVar15 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
        sVar2 = *(short *)(iVar9 + 2);
        sVar8 = 0;
        if (0 < sVar2) {
          do {
            uVar4 = *(uint *)(iVar9 + 0x18 + sVar8 * 4);
            uVar13 = 0xffffffff;
            while (uVar6 = uVar4, uVar6 != 0xffffffff) {
              uVar13 = uVar6;
              uVar4 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc)
                               + 0x11c);
            }
            iVar16 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar13 & 0xffff) * 0xc);
            if ((*(short *)(iVar16 + 0x9c) != -1) &&
               (sVar3 = *(short *)(iVar16 + 0x9c),
               (*(uint *)(iVar12 + ((int)sVar3 >> 5) * 4) & 1 << ((byte)sVar3 & 0x1f)) != 0)) {
              *(undefined1 *)(iVar15 + 0x12) = 0;
              break;
            }
            sVar8 = sVar8 + 1;
          } while (sVar8 < sVar2);
        }
      }
    }
    iVar9 = DAT_00880360;
    if (*(char *)(iVar15 + 0x12) == '\0' || *(char *)(iVar15 + 10) != '\0') {
      *(undefined2 *)(*(int *)(DAT_00880360 + 0x34) + 0x10 + iVar17) = 0x5a;
      iVar16 = *(int *)(iVar9 + 0x34) + iVar17;
      if (*(char *)(*(int *)(iVar9 + 0x34) + 8 + iVar17) != '\x01') {
        if ((*(char *)(iVar16 + 6) == '\0') || (actor_create_swarm(), *(int *)(iVar16 + 0x28) != -1)
           ) {
          *(undefined1 *)(iVar16 + 8) = 1;
          if (*(short *)(iVar16 + 0x6a) == 0) {
            actor_set_units_active();
          }
        }
        else {
          *(undefined1 *)(iVar16 + 0xb) = 1;
        }
      }
    }
    else if (*(short *)(iVar15 + 0x10) < 0x1f) {
      *(undefined2 *)(iVar15 + 0x10) = 0;
      *(undefined2 *)(*(int *)(iVar9 + 0x34) + 0x10 + iVar17) = 0;
      iVar16 = *(int *)(iVar9 + 0x34) + iVar17;
      if (*(char *)(*(int *)(iVar9 + 0x34) + 8 + iVar17) != '\0') {
        FUN_00427e00(uVar10);
        actor_delete_swarm();
        actor_set_units_active();
        iVar17 = DAT_006f1d6c;
        *(undefined1 *)(iVar16 + 8) = 0;
        *(undefined4 *)(iVar16 + 0xc) = *(undefined4 *)(iVar17 + 0xc);
      }
    }
    else {
      *(short *)(iVar15 + 0x10) = *(short *)(iVar15 + 0x10) + -0x1e;
    }
    uVar10 = *(uint *)(iVar15 + 0x2c);
  }
  iVar15 = data_iterator_next();
  do {
    if (iVar15 == 0) {
      return;
    }
    cVar1 = *(char *)(iVar15 + 0xc);
    sVar2 = *(short *)(iVar15 + 0x3e);
    sVar8 = *(short *)(*(int *)(global_scenario + 0x430) + 0xafffce);
    if ((sVar8 == -1) || (sVar8 == DAT_0069e8d8)) {
      FUN_00436190(1,iVar12);
      uVar10 = *(short *)(iVar7 + 0x134) + 0x1f >> 5;
      bVar5 = false;
      sVar8 = (short)uVar10 + -1;
      if (-1 < sVar8) {
        puVar14 = local_40 + sVar8;
        uVar10 = uVar10 & 0xffff;
        bVar5 = false;
        do {
          if ((*(uint *)((iVar12 - (int)local_40) + (int)puVar14) & *puVar14) != 0) {
            bVar5 = true;
          }
          puVar14 = puVar14 + -1;
          uVar10 = uVar10 - 1;
        } while (uVar10 != 0);
      }
      if ((sVar2 < 1 && cVar1 == '\0') && !bVar5) goto LAB_00438220;
      *(undefined2 *)(iVar15 + 0xe) = 0x96;
      squad_activate();
    }
    else {
LAB_00438220:
      if ((*(char *)(iVar15 + 0xd) == '\0') || (*(short *)(iVar15 + 0xe) < 0x1f)) {
        bVar5 = false;
        if (0 < *(short *)(iVar15 + 0x20)) {
          uVar10 = (uint)*(ushort *)(iVar15 + 0x20);
          puVar11 = (ushort *)(iVar15 + 0x22);
          bVar5 = false;
          do {
            if (0 < *(short *)((uint)*puVar11 * 0x6c + 0xe + *(int *)(DAT_008802c8 + 0x34))) {
              bVar5 = true;
            }
            puVar11 = puVar11 + 1;
            uVar10 = uVar10 - 1;
          } while (uVar10 != 0);
        }
        *(undefined2 *)(iVar15 + 0xe) = 0;
        if (bVar5) {
          squad_activate();
        }
        else {
          squad_deactivate();
        }
      }
      else {
        *(short *)(iVar15 + 0xe) = *(short *)(iVar15 + 0xe) + -0x1e;
      }
    }
    iVar15 = data_iterator_next();
  } while( true );
}
#endif
