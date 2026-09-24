// encounter_spawn_squads  (Ghidra: encounter_spawn_squads; named for this rewrite)
// address 0x437510, size 468 bytes
// name confidence: 0.45   rewrite confidence: 0.5
// evidence: phase-4 summary ("spawns the actors for a squad's starting locations, scaling
//   counts by difficulty and rolling per-location spawn chances for certain zone activity
//   types"). It walks ScenarioEncounter.squads, picks the per-squad actor count from
//   ScenarioSquad.normal_diff_count (+0x7c) / insane_diff_count (+0x7e) according to the
//   difficulty byte pair at 0x006b0b80 + 0x0e, and calls encounter_squad_spawn_actor
//   (0x438e20) that many times. ai_reference_activate_squads @0x432b80 (already rewritten)
//   is its other caller and passes the unpacked (encounter, platoon, squad) triple, which is
//   what fixes the filter argument order.
// register convention: plain __cdecl, three stack arguments.
//   // blam-cc: stack -> (encounter_index, platoon_filter, squad_filter)
//
// UNSURE:
//  - ai_squad_resolve_actor_type (0x4374a0) is compared against 7 in every branch below; 7
//    is not a named ActorType in types/ai.h. Whatever it is, the leader-chance percentage is
//    only ever non-zero for that type.
//  - ScenarioSquad.unique_leader_type (+0x2c) selects the leader rule: case 0 tests the
//    encounter's live_count / member_count against 4 or 10 depending on live_count, case 2
//    rolls a coin, cases 3 and 4 are a flat 100 / 101.
//  - Only the FIRST spawned actor of a squad gets the leader chance; the rest are spawned
//    with 0. That matches the original (iVar9 is zeroed inside the loop).
//  - Ghidra shows encounter_squad_spawn_actor taking four arguments here but only two at its
//    other call site; the declaration below matches this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_global_data;   // 0x00880354
extern Scenario *global_scenario;    // 0x00746f8c
extern uint8_t *cache_file_slot_table; // 0x006b0b80, same global the rest of src/ai declares under
                                       //   this name; the int16 at +0x0e is the difficulty level (0..3)
extern data_array *encounter_data;   // 0x008802c8
extern uint32_t random_seed_global;  // 0x00719cd0

extern int16_t ai_squad_resolve_actor_type(ScenarioSquad *squad); // 0x4374a0, blam-cc: ECX -> squad
extern void encounter_recompute_morale(datum_index encounter_index); // 0x437940
extern void encounters_update_activation(void);                      // 0x437e20
extern void encounter_squad_spawn_actor(datum_index encounter_index, int32_t squad_index,
    int32_t leader_chance, int32_t unit_type_index); // 0x438e20, this call site shows four arguments

// blam-cc: stack -> (encounter_index, platoon_filter, squad_filter)
// Spawns the configured number of actors for every squad of an encounter that passes the
// platoon / squad filter (both -1 means "all squads"), then refreshes the encounter's morale
// and the global activation pass.
void encounter_spawn_squads(datum_index encounter_index, int16_t platoon_filter,
                            int16_t squad_filter)
{
    ScenarioEncounter *definition;
    ScenarioSquad *squad;
    encounter *enc;
    int32_t squad_index;
    int32_t spawn_count;
    int16_t actor_type;
    int16_t remaining;
    int32_t leader_chance;
    int32_t margin;
    uint8_t all_squads;

    if (ai_global_data->actors_valid == 0) {
        return;
    }
    definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
        [encounter_index & 0xffff];

    all_squads = (uint8_t)(platoon_filter == -1 && squad_filter == -1);

    squad_index = 0;
    if (0 < definition->squads.count) {
        do {
            squad = &((ScenarioSquad *)definition->squads.pointer)[squad_index];
            if (all_squads != 0 || (int16_t)squad_index == squad_filter ||
                ((int16_t)squad->platoon != -1 && (int16_t)squad->platoon == platoon_filter)) {

                leader_chance = 0;
                spawn_count = 0;
                switch (*(int16_t *)(cache_file_slot_table + 0x0e)) {
                case 0:
                case 1:
                    spawn_count = (int32_t)(uint16_t)squad->normal_diff_count;
                    break;
                case 2:
                    spawn_count = ((int32_t)squad->insane_diff_count +
                                   (int32_t)squad->normal_diff_count) / 2;
                    break;
                case 3:
                    spawn_count = (int32_t)(uint16_t)squad->insane_diff_count;
                    break;
                }

                actor_type = ai_squad_resolve_actor_type(squad);
                remaining = (int16_t)spawn_count;

                switch (squad->unique_leader_type) {
                case 0:
                    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
                    if (actor_type == 7) {
                        if (enc->live_count == 0) {
                            margin = (int32_t)enc->member_count + (int32_t)remaining - 4;
                        } else if (enc->live_count == 1) {
                            margin = (int32_t)enc->member_count + (int32_t)remaining - 10;
                        } else {
                            break;
                        }
                        if (margin >= 0) {
                            goto leader_coin_flip;
                        }
                    }
                    break;
                case 2:
leader_coin_flip:
                    if (actor_type == 7) {
                        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
                        leader_chance = 100 - (int32_t)(random_seed_global >> 0x1f);
                    }
                    break;
                case 3:
                    if (actor_type == 7) {
                        leader_chance = 100;
                    }
                    break;
                case 4:
                    if (actor_type == 7) {
                        leader_chance = 0x65;
                    }
                    break;
                }

                if (0 < remaining) {
                    uint32_t left = (uint32_t)spawn_count & 0xffff;
                    do {
                        encounter_squad_spawn_actor(encounter_index, squad_index,
                                                    leader_chance, 0);
                        leader_chance = 0;
                        left = left - 1;
                    } while (left != 0);
                }
            }
            squad_index = (int32_t)(int16_t)(squad_index + 1);
        } while (squad_index < definition->squads.count);
    }

    encounter_recompute_morale(encounter_index);
    encounters_update_activation();
}

#if 0
Original Ghidra decompilation (0x437510):

void FUN_00437510(uint param_1,short param_2,short param_3)

{
  bool bVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  short sVar7;
  uint uVar8;
  int iVar9;
  uint uVar10;
  bool bVar11;

  if (*(char *)(DAT_00880354 + 1) == '\0') {
    return;
  }
  uVar4 = param_1 & 0xffff;
  iVar5 = uVar4 * 0xb0 + *(int *)(global_scenario + 0x430);
  if ((param_2 != -1) || (bVar1 = true, param_3 != -1)) {
    bVar1 = false;
  }
  iVar6 = 0;
  if (0 < *(int *)(iVar5 + 0x80)) {
    iVar3 = 0;
    uVar8 = uVar4;
    do {
      iVar3 = iVar3 * 0xe8 + *(int *)(iVar5 + 0x84);
      if (((bVar1) || ((short)iVar6 == param_3)) ||
         ((*(short *)(iVar3 + 0x22) != -1 && (*(short *)(iVar3 + 0x22) == param_2)))) {
        iVar9 = 0;
        switch(*(undefined2 *)(DAT_006b0b80 + 0xe)) {
        case 0:
        case 1:
          uVar8 = (uint)*(ushort *)(iVar3 + 0x7c);
          break;
        case 2:
          uVar8 = ((int)*(short *)(iVar3 + 0x7e) + (int)*(short *)(iVar3 + 0x7c)) / 2;
          break;
        case 3:
          uVar8 = (uint)*(ushort *)(iVar3 + 0x7e);
        }
        sVar2 = FUN_004374a0();
        sVar7 = (short)uVar8;
        switch(*(undefined2 *)(iVar3 + 0x2c)) {
        case 0:
          iVar3 = uVar4 * 0x6c + *(int *)(DAT_008802c8 + 0x34);
          if (sVar2 == 7) {
            if (*(short *)(iVar3 + 0x1c) == 0) {
              iVar3 = (int)*(short *)(iVar3 + 0x18) + (int)sVar7;
              bVar11 = SBORROW4(iVar3,4);
              iVar3 = iVar3 + -4;
            }
            else {
              if (*(short *)(iVar3 + 0x1c) != 1) break;
              iVar3 = (int)*(short *)(iVar3 + 0x18) + (int)sVar7;
              bVar11 = SBORROW4(iVar3,10);
              iVar3 = iVar3 + -10;
            }
            if (bVar11 == iVar3 < 0) goto switchD_00437600_caseD_2;
          }
          break;
        case 2:
switchD_00437600_caseD_2:
          if (sVar2 == 7) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            iVar9 = 100 - (random_seed_global >> 0x1f);
          }
          break;
        case 3:
          if (sVar2 == 7) {
            iVar9 = 100;
          }
          break;
        case 4:
          if (sVar2 == 7) {
            iVar9 = 0x65;
          }
        }
        if (0 < sVar7) {
          uVar10 = uVar8 & 0xffff;
          do {
            FUN_00438e20(param_1,iVar6,iVar9,0);
            iVar9 = 0;
            uVar10 = uVar10 - 1;
          } while (uVar10 != 0);
        }
      }
      iVar6 = iVar6 + 1;
      iVar3 = (int)(short)iVar6;
    } while (iVar3 < *(int *)(iVar5 + 0x80));
  }
  FUN_00437940(param_1);
  FUN_00437e20();
  return;
}
#endif
