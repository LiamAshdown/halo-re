// encounter_new  (Ghidra: squad_create; renamed for this rewrite)
// address 0x437060, size 435 bytes
// name confidence: 0.7   rewrite confidence: 0.8
// evidence: out/phase4/ai_types_notes.md, "encounter_new @0x437060 creates an *encounter*
//   datum (one per ScenarioEncounter), not a squad. encounters_reset @0x435cb0 calls it once
//   per Scenario.encounters entry." It allocates out of encounter_data (stride 0x6c) and
//   hands out the encounter runs of encounter_squad_state and encounter_platoon_state from
//   two caller-owned running cursors, which is exactly what types/ai.h documents.
// register convention: recovered from the disassembly
//   (objdump -d -M intel --start-address=0x437060 --stop-address=0x437215 bin/halo.exe):
//   EAX -> squad_cursor (mov edi,eax then mov dx,[edi]), EBX -> the ScenarioEncounter,
//   first stack argument -> platoon_cursor ([esp+0x14] after three pushes = the argument
//   slot). datum_new takes encounter_data in EDX.
//   // blam-cc: EAX -> squad_cursor, EBX -> definition, stack -> platoon_cursor
//
// The multiplier Ghidra hides behind its bare __ftol() call is ticks_per_second: the real
// instructions are `fld [edi+0x50]` (ScenarioSquad.squad_delay_time), `fmul ds:0x672ac8`
// (30.0), `call 0x6391b4`, so squad_delay_ticks is the delay expressed in ticks.
//
// UNSURE: encounter.unknown_20 is the only field zeroed here that types/ai.h has no reader
// for, and the three ScenarioEncounter.flags bits (1, 2, 3) copied to encounter+0x3c,
// +0x40 and +0x41 are not named in types/tags.h either.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *encounter_data;                        // 0x008802c8
extern encounter_squad_state *encounter_squad_states;     // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4
extern float ticks_per_second;                            // 0x00672ac8, 30.0

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern int32_t __ftol(double value);             // 0x006391b4, MSVC 7.1 CRT truncation
extern void encounter_squad_reset_starting_location_mask(datum_index encounter_index,
    int16_t squad_index); // 0x436f90, blam-cc: EAX -> encounter_index, ECX -> squad_index

// blam-cc: EAX -> squad_cursor, EBX -> definition, stack -> platoon_cursor
// Builds the runtime encounter datum for one ScenarioEncounter: it takes a run of
// squad_count encounter_squad_state records and a run of platoon_count
// encounter_platoon_state records out of the two running cursors the caller threads through
// every encounter, then seeds each squad record (respawn budget, squad delay, the two flag
// bits and its starting-location mask) and each platoon record flag bit from the tag.
void encounter_new(int16_t *squad_cursor, ScenarioEncounter *definition,
                   int16_t *platoon_cursor)
{
    datum_index encounter_index;
    encounter *enc;
    encounter_squad_state *squad_state;
    ScenarioSquad *squad_definition;
    int16_t squad_index;
    int16_t platoon_index;
    int16_t respawn_budget;
    int16_t count;

    encounter_index = datum_new(encounter_data);
    if (encounter_index == (datum_index)k_datum_index_none) {
        return;
    }
    enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];

    enc->team = definition->team_index;
    enc->first_actor = (datum_index)k_datum_index_none;
    enc->first_pursuit = (datum_index)k_datum_index_none;
    enc->blind = (uint8_t)((definition->flags >> 2) & 1);
    enc->deaf = (uint8_t)((definition->flags >> 3) & 1);
    enc->respawn_enabled = (uint8_t)((definition->flags >> 1) & 1);
    enc->respawn_delay_ticks = 0;
    enc->unknown_46 = 0;
    enc->engaged = 0;
    enc->ticks_since_engaged = (datum_index)k_datum_index_none;
    enc->has_live_target = 0;
    enc->ticks_since_live_target = (datum_index)k_datum_index_none;
    enc->last_idle_time = -1;
    enc->stood_down = 1;
    enc->last_grenade_time = (datum_index)k_datum_index_none;
    enc->activation_link_count = 0;
    enc->activation_tick = -1;

    count = (int16_t)definition->squads.count;
    enc->squad_count = count;
    enc->first_squad = *squad_cursor;
    *squad_cursor = *squad_cursor + count;

    squad_index = 0;
    if (0 < enc->squad_count) {
        do {
            squad_state = &encounter_squad_states[(int16_t)(enc->first_squad + squad_index)];
            squad_definition = &((ScenarioSquad *)definition->squads.pointer)[squad_index];

            squad_state->timer_started = 0;
            if ((squad_definition->flags & 8) == 0) {
                squad_state->squad_delay_ticks = (int16_t)__ftol(
                    (double)(squad_definition->squad_delay_time * ticks_per_second));
            } else {
                squad_state->squad_delay_ticks = 999;
            }
            squad_state->automatic_migration = (uint8_t)((squad_definition->flags >> 5) & 1);

            encounter_squad_reset_starting_location_mask(encounter_index, squad_index);

            if (0 < squad_definition->respawn_max_actors ||
                0 < squad_definition->respawn_min_actors) {
                respawn_budget = 999;
                if (squad_definition->respawn_total != 0) {
                    respawn_budget = squad_definition->respawn_total;
                }
                squad_state->respawn_budget = respawn_budget;
            }
            squad_index = squad_index + 1;
        } while (squad_index < enc->squad_count);
    }

    count = (int16_t)definition->platoons.count;
    enc->platoon_count = count;
    enc->first_platoon = *platoon_cursor;
    *platoon_cursor = *platoon_cursor + count;

    platoon_index = 0;
    if (0 < enc->platoon_count) {
        do {
            encounter_platoon_states[(int16_t)(enc->first_platoon + platoon_index)].defending =
                (uint8_t)((((ScenarioPlatoon *)definition->platoons.pointer)[platoon_index].flags
                    >> 2) & 1);
            platoon_index = platoon_index + 1;
        } while (platoon_index < enc->platoon_count);
    }
}

#if 0
Original Ghidra decompilation (0x437060):

void squad_create(short *param_1)

{
  undefined2 uVar1;
  short sVar2;
  short *in_EAX;
  short sVar3;
  int unaff_EBX;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined8 uVar7;

  uVar7 = datum_new();
  if ((uint)uVar7 != 0xffffffff) {
    iVar5 = ((uint)uVar7 & 0xffff) * 0x6c + *(int *)((int)((ulonglong)uVar7 >> 0x20) + 0x34);
    *(undefined2 *)(iVar5 + 2) = *(undefined2 *)(unaff_EBX + 0x24);
    *(undefined4 *)(iVar5 + 0x14) = 0xffffffff;
    *(undefined4 *)(iVar5 + 0x38) = 0xffffffff;
    *(byte *)(iVar5 + 0x40) = (byte)(*(uint *)(unaff_EBX + 0x20) >> 2) & 1;
    *(byte *)(iVar5 + 0x41) = (byte)(*(uint *)(unaff_EBX + 0x20) >> 3) & 1;
    *(byte *)(iVar5 + 0x3c) = (byte)(*(uint *)(unaff_EBX + 0x20) >> 1) & 1;
    *(undefined2 *)(iVar5 + 0x3e) = 0;
    *(undefined1 *)(iVar5 + 0x46) = 0;
    *(undefined1 *)(iVar5 + 0x45) = 0;
    *(undefined4 *)(iVar5 + 0x50) = 0xffffffff;
    *(undefined1 *)(iVar5 + 0x44) = 0;
    *(undefined4 *)(iVar5 + 0x54) = 0xffffffff;
    *(undefined4 *)(iVar5 + 0x58) = 0xffffffff;
    *(undefined1 *)(iVar5 + 0x42) = 1;
    *(undefined4 *)(iVar5 + 0x5c) = 0xffffffff;
    *(undefined2 *)(iVar5 + 0x20) = 0;
    *(undefined4 *)(iVar5 + 0x10) = 0xffffffff;
    sVar2 = *(short *)(unaff_EBX + 0x80);
    *(short *)(iVar5 + 6) = sVar2;
    *(short *)(iVar5 + 4) = *in_EAX;
    *in_EAX = *in_EAX + sVar2;
    sVar2 = 0;
    if (0 < *(short *)(iVar5 + 6)) {
      do {
        iVar4 = (short)(*(short *)(iVar5 + 4) + sVar2) * 0x20 + DAT_008802cc;
        iVar6 = sVar2 * 0xe8 + *(int *)(unaff_EBX + 0x84);
        *(undefined1 *)(iVar4 + 0x11) = 0;
        if ((*(byte *)(iVar6 + 0x28) & 8) == 0) {
          uVar1 = __ftol();
          *(undefined2 *)(iVar4 + 0x12) = uVar1;
        }
        else {
          *(undefined2 *)(iVar4 + 0x12) = 999;
        }
        *(byte *)(iVar4 + 0x10) = (byte)(*(uint *)(iVar6 + 0x28) >> 5) & 1;
        FUN_00436f90();
        if ((0 < *(short *)(iVar6 + 0x86)) || (0 < *(short *)(iVar6 + 0x84))) {
          sVar3 = 999;
          if (*(short *)(iVar6 + 0x88) != 0) {
            sVar3 = *(short *)(iVar6 + 0x88);
          }
          *(short *)(iVar4 + 0xc) = sVar3;
        }
        sVar2 = sVar2 + 1;
      } while (sVar2 < *(short *)(iVar5 + 6));
    }
    sVar2 = *(short *)(unaff_EBX + 0x8c);
    *(short *)(iVar5 + 10) = sVar2;
    *(short *)(iVar5 + 8) = *param_1;
    *param_1 = *param_1 + sVar2;
    iVar4 = DAT_008802c4;
    sVar2 = 0;
    if (0 < *(short *)(iVar5 + 10)) {
      do {
        iVar6 = (int)sVar2;
        sVar3 = *(short *)(iVar5 + 8) + sVar2;
        sVar2 = sVar2 + 1;
        *(byte *)(sVar3 * 0x10 + iVar4) =
             (byte)(*(uint *)(iVar6 * 0xac + 0x20 + *(int *)(unaff_EBX + 0x90)) >> 2) & 1;
      } while (sVar2 < *(short *)(iVar5 + 10));
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
