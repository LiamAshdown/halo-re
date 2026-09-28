// ai_unit_set_squad_reference  (Ghidra: ai_unit_set_squad_reference; named for this rewrite)
// address 0x435750, size 431 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: phase-4 summary ("records which design squad-member slot a unit's actor
//   corresponds to, and notifies other actors that were targeting it"). It resolves a packed
//   ai reference (the same 2-bit tag in bits 30..31 and int8 sub-index in byte 2 that
//   ai_reference_parse @0x432320 decodes) against ScenarioEncounter.squads and stores the
//   resulting (encounter_index, squad_index) pair in object + 0x334 / + 0x336, then tells
//   every actor whose active_unit_index is this object about the change.
// register convention: plain __cdecl, two stack arguments.
//   // blam-cc: stack -> (object_index, packed_reference)
//
// UNSURE (high):
//  - object + 0x334 / + 0x336 are not named in types/objects.h; 0x334 takes the encounter
//    index (or -1) and 0x336 the squad index within it.
//  - Ghidra's rendering of the local initialisation is heavily aliased (local_1c / local_14
//    are assigned -1 four times through different sub-widths). It collapses to "both outputs
//    default to -1".
//  - The reference tag test is `packed_reference >> 0x1e`: 1 selects "match squads by their
//    ScenarioSquad.platoon field", 2 selects "the sub-index IS the squad index". Any other
//    tag falls through with squad index 0.
//  - ai_reference_actor_iterator_init_cursor (0x4369f0) is the cursor-seeding half of the
//    iterator pair; Ghidra shows it argument-less and leaves local_4 (the cursor) live.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"

extern data_array *object_data;    // 0x008603b0
extern Scenario *global_scenario;  // 0x00746f8c
extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *actor_data;     // 0x00880360

extern void actor_reset_squad_link_for_type_change(datum_index actor_index, datum_index encounter_index,
    int16_t squad_index); // 0x4290f0, EAX, EBX, stack
extern void ai_reference_actor_iterator_init_cursor(int32_t encounter_index, datum_index *cursor); // 0x4369f0, EAX, ECX

// blam-cc: stack -> (object_index, packed_reference)
void ai_unit_set_squad_reference(datum_index object_index, uint32_t packed_reference)
{
    object *obj;
    ScenarioEncounter *definition;
    int16_t encounter_index;
    int16_t out_encounter;
    uint32_t out_squad;
    uint32_t squad_index;
    int32_t squad_count;
    int32_t i;
    datum_index actor_index;
    actor *a;

    if (object_index == (datum_index)k_datum_index_none) {
        return;
    }
    obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    out_encounter = -1;
    out_squad = 0xffff;

    encounter_index = (int16_t)packed_reference;
    if (packed_reference == 0xffffffff || encounter_index < 0 ||
        global_scenario->encounters.count <= (int32_t)encounter_index) {
        goto store;
    }

    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)[encounter_index];
    squad_index = 0;

    if (packed_reference >> 0x1e == 1) {
        squad_count = definition->squads.count;
        squad_index = 0;
        if (0 < squad_count) {
            i = 0;
            do {
                if (((ScenarioSquad *)definition->squads.pointer)[i].platoon ==
                    (uint16_t)(uint8_t)(packed_reference >> 0x10)) {
                    break;
                }
                squad_index = squad_index + 1;
                i = (int32_t)(int16_t)squad_index;
            } while (i < squad_count);
        }
        if ((int16_t)squad_index >= squad_count) {
            squad_index = 0;
        } else if ((int16_t)squad_index < 0) {
            goto store;
        }
    } else if (packed_reference >> 0x1e == 2) {
        squad_index = (uint32_t)(uint8_t)(packed_reference >> 0x10);
        if ((int16_t)squad_index < 0) {
            goto store;
        }
    }

    if ((int32_t)(int16_t)squad_index < definition->squads.count) {
        out_squad = squad_index;
        out_encounter = encounter_index;
        if (encounter_index != -1 && (int16_t)squad_index != -1 &&
            *(int16_t *)((uint8_t *)obj + 0x334) != -1) {
            // FIXED (objdump 0x435814..0x435888): walk the members of the unit's OLD encounter (+0x334) and move
            //   the ones driving this unit to (EBX = the new encounter, stack = the squad). The draft passed no
            //   cursor and left the new encounter out of the move.
            datum_index cursor[3];

            ai_reference_actor_iterator_init_cursor((int32_t)*(int16_t *)((uint8_t *)obj + 0x334), cursor);
            actor_index = cursor[2];
            while (ai_globals_ptr->actors_valid != 0 &&
                   actor_index != (datum_index)k_datum_index_none) {
                datum_index current = actor_index;
                a = &((actor *)actor_data->data)[current & 0xffff];
                actor_index = a->next_in_encounter;
                if (a->active_unit_index == object_index) {
                    actor_reset_squad_link_for_type_change(current, (datum_index)(int32_t)encounter_index,
                        (int16_t)squad_index);
                }
            }
        }
    }

store:
    *(int16_t *)((uint8_t *)obj + 0x334) = out_encounter;
    *(int16_t *)((uint8_t *)obj + 0x336) = (int16_t)out_squad;
}

#if 0
Original Ghidra decompilation (0x435750):

void FUN_00435750(uint param_1,uint param_2)

{
  int iVar1;
  int iVar2;
  short sVar3;
  short sVar4;
  undefined2 uVar5;
  int iVar6;
  int iVar7;
  short sVar8;
  uint uVar9;
  short local_1c;
  uint local_14;
  uint local_4;

  if (param_1 == 0xffffffff) {
    return;
  }
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_1c = -1;
  sVar4 = local_1c;
  local_1c = -1;
  sVar3 = local_1c;
  local_1c = -1;
  local_14 = 0xffffffff;
  local_14._0_2_ = 0xffff;
  uVar5 = (undefined2)local_14;
  local_14._0_2_ = 0xffff;
  if (((param_2 == 0xffffffff) || (sVar8 = (short)param_2, sVar8 < 0)) ||
     (local_1c = sVar3, local_14._0_2_ = uVar5, *(int *)(global_scenario + 0x42c) <= (int)sVar8))
  goto LAB_004358de;
  iVar7 = (param_2 & 0xffff) * 0xb0 + *(int *)(global_scenario + 0x430);
  uVar9 = 0;
  local_1c = sVar4;
  if (param_2 >> 0x1e == 1) {
    iVar2 = *(int *)(iVar7 + 0x80);
    uVar9 = 0;
    if (0 < iVar2) {
      iVar6 = 0;
      do {
        if (*(ushort *)(iVar6 * 0xe8 + 0x22 + *(int *)(iVar7 + 0x84)) == (ushort)param_2._2_1_)
        break;
        uVar9 = uVar9 + 1;
        iVar6 = (int)(short)uVar9;
      } while (iVar6 < iVar2);
    }
    if ((short)uVar9 < iVar2) goto LAB_004357de;
    uVar9 = 0;
LAB_004357e7:
    if ((((int)(short)uVar9 < *(int *)(iVar7 + 0x80)) &&
        (local_14 = uVar9, local_1c = sVar8, sVar8 != -1)) &&
       (((short)uVar9 != -1 && (*(short *)(iVar1 + 0x334) != -1)))) {
      FUN_004369f0();
      while ((*(char *)(DAT_00880354 + 1) != '\0' && (local_4 != 0xffffffff))) {
        iVar7 = (local_4 & 0xffff) * 0x724;
        local_4 = *(uint *)(iVar7 + *(int *)(DAT_00880360 + 0x34) + 0x2c);
        if (*(uint *)(iVar7 + 0x158 + *(int *)(DAT_00880360 + 0x34)) == param_1) {
          FUN_004290f0(uVar9);
        }
      }
    }
  }
  else {
    if (param_2 >> 0x1e != 2) goto LAB_004357e7;
    uVar9 = (uint)param_2._2_1_;
LAB_004357de:
    if (-1 < (short)uVar9) goto LAB_004357e7;
  }
LAB_004358de:
  *(short *)(iVar1 + 0x334) = local_1c;
  *(undefined2 *)(iVar1 + 0x336) = (undefined2)local_14;
  return;
}
#endif
