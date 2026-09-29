// ai_object_list_max_flee_grade  (Ghidra: ai_object_list_max_flee_grade; named for this rewrite)
// address 0x434f20, size 666 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: phase-4 summary ("computes the highest morale/danger grade across all actors
//   belonging to a squad group, used to drive flee/retreat decisions"). It walks an
//   object_list (types/hs.h, the 0x0087a464 / 0x0087a468 pair every ai_object_list_* function
//   in this directory uses) and for each biped or vehicle reduces the controlling actor to a
//   small integer grade, returning the maximum. It shares its swarm branch with
//   ai_actor_type_get_morale_grade @0x434ed0 (already rewritten), which is where the name
//   comes from. _actor_mode_flee (11) is the only mode that produces a grade above the
//   "recently hurt" fallback.
// register convention: EAX -> object_list_header_handle handle; no stack arguments.
//   // blam-cc: EAX -> object_list_header_handle
//
// UNSURE:
//  - actor + 0x9c and + 0xa4 / + 0xa8 are inside actor.mode_data.raw, so the command-list lookup
//    below only makes sense while the actor is in the flee mode whose mode_data holds a
//    command-list index at +0x00 and a command cursor at +0x08.
//  - The grade produced by the command-list branch is ((~flags & 0x10) | 0x20) >> 4, i.e. 3
//    when bit 4 of actor.mode_data.raw[0x0c] is clear and 2 when it is set.
//  - actor.last_obey_tick is a tick stamp; a member hurt within the last 150 ticks grades 1.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"
#include "hs.h"
#include "ai.h"

extern game_time_globals *game_time;           // 0x006f1d6c
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_data;                // 0x008603b0
extern data_array *actor_data;                 // 0x00880360
extern Scenario *global_scenario;              // 0x00746f8c
extern data_array *swarm_data;                 // 0x0088035c
extern data_array *swarm_component_data;       // 0x00880358

extern uint32_t ai_actor_type_get_morale_grade(int16_t actor_type_index, uint8_t *command_reference); // 0x434ed0, AX, EDX

// blam-cc: EAX -> object_list_header_handle
int16_t ai_object_list_max_flee_grade(datum_index object_list_header_handle)
{
    int32_t tick;
    object_list_header *header;
    object_list_reference *node;
    object_header *entry;
    object *obj;
    unit_data *unit;
    actor *a;
    swarm *sw;
    ScenarioCommandList *command_list;
    datum_index node_index;
    datum_index object_index;
    uint32_t grade;
    uint32_t best;
    int16_t component_count;
    int16_t component_index;
    int32_t command_index;

    tick = game_time->game_time;
    object_index = (datum_index)k_datum_index_none;
    node_index = (datum_index)k_datum_index_none;
    best = 0;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }
    if (object_index == (datum_index)k_datum_index_none) {
        return 0;
    }

    do {
        entry = 0;
        if (object_index != (datum_index)k_datum_index_none &&
            0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)
                [(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 ||
                 candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            obj = entry->data;
            unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            grade = 0;

            if (unit->actor_index == (datum_index)k_datum_index_none) {
                if (unit->swarm_actor_index != (datum_index)k_datum_index_none) {
                    a = &((actor *)actor_data->data)[unit->swarm_actor_index & 0xffff];
                    if (a->mode == _actor_mode_flee &&
                        a->swarm_index != (datum_index)k_datum_index_none) {
                        sw = &((swarm *)swarm_data->data)[a->swarm_index & 0xffff];
                        component_count = sw->component_count;
                        component_index = 0;
                        if (0 < component_count) {
                            do {
                                if (sw->unit_index[component_index] == object_index) {
                                    break;
                                }
                                component_index = component_index + 1;
                            } while (component_index < component_count);
                        }
                        if (component_index < component_count &&
                            (((swarm_component *)swarm_component_data->data)
                                 [sw->component_index[component_index] & 0xffff].flags & 8) != 0) {
                            // FIXED (objdump 0x43512c..0x435138): AX = the actor's mode-data word (+0x9c), EDX = the
                            //   swarm component + 0x1c. The draft passed nothing.
                            grade = (uint32_t)(uint16_t)ai_actor_type_get_morale_grade(*(int16_t *)&((struct actor *)a)->mode_data,
                                (uint8_t *)&((swarm_component *)swarm_component_data->data)
                                    [sw->component_index[component_index] & 0xffff] + 0x1c);
                            goto have_grade;
                        }
                    }
                    goto recently_hurt;
                }
            } else {
                a = &((actor *)actor_data->data)[unit->actor_index & 0xffff];
                if (a->mode == _actor_mode_flee) {
                    command_list = &((ScenarioCommandList *)global_scenario->command_lists.pointer)
                        [*(int16_t *)(a->mode_data.raw + 0x00)];
                    command_index = (int32_t)(uint32_t)a->mode_data.raw[8];
                    if (command_index < command_list->commands.count &&
                        (uint8_t *)command_list->commands.pointer + command_index * 0x20 != 0) {
                        grade = (uint32_t)((((uint8_t)~a->mode_data.raw[0x0c] & 0x10) | 0x20) >> 4);
                    } else {
                        grade = 1;
                    }
have_grade:
                    if ((int16_t)grade != 0) {
                        goto keep_best;
                    }
                }
recently_hurt:
                if (a->last_obey_tick != -1 && tick <= a->last_obey_tick + 0x96) {
                    grade = 1;
                }
            }
keep_best:
            if ((int16_t)best <= (int16_t)grade) {
                best = grade;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    } while (object_index != (datum_index)k_datum_index_none);

    return (int16_t)best;
}

#if 0
Original Ghidra decompilation (0x434f20):

uint FUN_00434f20(void)

{
  int iVar1;
  uint in_EAX;
  uint uVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  short sVar6;
  short sVar7;
  int iVar8;
  uint local_10;
  uint local_c;

  iVar1 = *(int *)(DAT_006f1d6c + 0xc);
  iVar5 = -1;
  local_c = 0;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar5 = -1;
      local_10 = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      local_10 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  uVar2 = local_10;
  if (iVar5 == -1) {
    return 0;
  }
  do {
    iVar8 = 0;
    if (((iVar5 != -1) && (sVar7 = (short)iVar5, -1 < sVar7)) &&
       (sVar7 < *(short *)(DAT_008603b0 + 0x20))) {
      iVar3 = (int)*(short *)(DAT_008603b0 + 0x22) * (int)sVar7;
      sVar7 = *(short *)(iVar3 + *(int *)(DAT_008603b0 + 0x34));
      if ((sVar7 != 0) && ((sVar6 = (short)((uint)iVar5 >> 0x10), sVar6 == 0 || (sVar7 == sVar6))))
      {
        iVar8 = iVar3 + *(int *)(DAT_008603b0 + 0x34);
      }
    }
    if (((iVar8 != 0) && ((1 << (*(byte *)(iVar8 + 3) & 0x1f) & 3U) != 0)) &&
       (iVar8 = *(int *)(iVar8 + 8), iVar8 != 0)) {
      uVar4 = 0;
      if (*(uint *)(iVar8 + 500) == 0xffffffff) {
        if (*(uint *)(iVar8 + 0x1f8) != 0xffffffff) {
          iVar8 = (*(uint *)(iVar8 + 0x1f8) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
          if ((*(short *)(iVar8 + 0x6c) == 0xb) && (*(uint *)(iVar8 + 0x28) != 0xffffffff)) {
            iVar3 = (*(uint *)(iVar8 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34);
            sVar7 = *(short *)(iVar3 + 2);
            sVar6 = 0;
            if (0 < sVar7) {
              do {
                uVar2 = local_10;
                if (*(int *)(iVar3 + 0x18 + sVar6 * 4) == iVar5) break;
                sVar6 = sVar6 + 1;
              } while (sVar6 < sVar7);
            }
            if ((sVar6 < sVar7) &&
               ((*(byte *)((*(uint *)(iVar3 + 0x58 + sVar6 * 4) & 0xffff) * 0x40 + 2 +
                          *(int *)(DAT_00880358 + 0x34)) & 8) != 0)) {
              uVar4 = FUN_00434ed0();
              goto LAB_0043513d;
            }
          }
          goto LAB_00435142;
        }
      }
      else {
        iVar8 = (*(uint *)(iVar8 + 500) & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
        if (*(short *)(iVar8 + 0x6c) == 0xb) {
          iVar5 = *(short *)(iVar8 + 0x9c) * 0x60;
          if (((int)(uint)*(byte *)(iVar8 + 0xa4) <
               *(int *)(iVar5 + 0x30 + *(int *)(global_scenario + 0x43c))) &&
             ((uint)*(byte *)(iVar8 + 0xa4) * 0x20 +
              *(int *)(iVar5 + *(int *)(global_scenario + 0x43c) + 0x34) != 0)) {
            uVar4 = ((byte)~*(byte *)(iVar8 + 0xa8) & 0x10 | 0x20) >> 4;
          }
          else {
            uVar4 = 1;
          }
LAB_0043513d:
          if ((short)uVar4 != 0) goto LAB_00435160;
        }
LAB_00435142:
        if ((*(int *)(iVar8 + 0x94) != -1) && (iVar1 <= *(int *)(iVar8 + 0x94) + 0x96)) {
          uVar4 = 1;
        }
      }
LAB_00435160:
      if ((short)local_c <= (short)uVar4) {
        local_c = uVar4;
      }
    }
    if (uVar2 == 0xffffffff) {
      iVar5 = -1;
    }
    else {
      uVar4 = uVar2 & 0xffff;
      uVar2 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar4 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar4 * 0xc + 4);
      local_10 = uVar2;
    }
    if (iVar5 == -1) {
      return local_c;
    }
  } while( true );
}
#endif
