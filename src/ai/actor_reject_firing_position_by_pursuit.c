// actor_reject_firing_position_by_pursuit  (Ghidra: actor_reject_firing_position_by_pursuit, renamed)
// address 0x412350, size 365 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: the last row of the rejection table at 0x006555f8, kinds mask 0x20, i.e. goal
//   kind 5 -- which actor_get_firing_position_group_mask @0x412880 maps to
//   ScenarioSquad.pursuing. Its two callees are the "ai pursuit" ring accessors
//   out/phase4/ai_types_notes.md lists as squad_recent_object_* @0x436b10 and 0x436b90,
//   hanging off encounter.first_pursuit.
// register convention: actor_index, the query and the candidate are the three
//   Ghidra-recognized stack parameters; both ring accessors take the encounter handle plus
//   two out-parameters.
//
// UNSURE: this function has no caller in the export, because nothing but the rule table
// reaches it and Ghidra did not follow the table. It was located by reading the table out
// of bin/halo.exe.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;     // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c

extern uint8_t ai_pursuit_note_object(datum_index object_index, datum_index encounter_index, int16_t type,
    int32_t min_last_tick); // 0x436b10, EDX object, stack encounter, CX type, EAX min_last_tick
extern uint8_t ai_pursuit_check_object(datum_index object_index, datum_index encounter_index, int16_t type,
    int32_t min_last_tick, char create_if_missing, int16_t *out_count, uint32_t *out_last_tick);
    // 0x436b90: EBX object, EAX min_last_tick, CX type, stack (encounter, out_count, out_last_tick); the
    // create_if_missing slot is the constant 0 the binary passes on to squad_recent_object_get_or_create

// blam-cc: stack -> actor_index, query, candidate
// The pursuit rule. A candidate the actor can already reach in under six units and that the
// movement request liked is treated as freshly visited; anything further away is looked up
// in the encounter recently-seen ring. Depending on query.score_instead_of_reject a lookup
// miss either rejects the candidate or just costs it 15.0. Surviving candidates are then
// charged for staleness (up to 10.0 once the sighting is more than 300 ticks old) and for a
// low sighting count (5.0 per missing sighting below four). Scores are desirability.
uint8_t actor_reject_firing_position_by_pursuit(datum_index actor_index,
                                                actor_firing_position_query *query,
                                                actor_firing_position_candidate *candidate)
{
    actor *self;
    int32_t tick;
    int32_t last_tick;
    int32_t count_out;
    int16_t sighting_count;
    uint8_t missed;
    float bonus;

    tick = game_time->game_time;
    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    last_tick = -1;
    count_out = 0;

    if (candidate == (actor_firing_position_candidate *)0) {
        return 0;
    }

    if (candidate->request_result != 0 || candidate->distance_from_actor >= 6.0f) {
        // FIXED (objdump 0x4123d5..0x4123f0): EBX = the actor, EAX = query +0xc, CX = the firing position,
        //   stack = (encounter, &count, &last_tick). The draft passed three arguments in the wrong slots.
        int16_t count16 = 0;
        missed = (uint8_t)(ai_pursuit_check_object(actor_index, self->encounter_index, candidate->firing_position_index,
            *(int32_t *)((uint8_t *)query + 0xc), 0, &count16, (uint32_t *)&last_tick) == 0);
        sighting_count = count16;
    } else {
        // FIXED (objdump 0x4123b3..0x4123c2): EDX = the actor, EAX = query +0xc, CX = the firing position,
        //   stack = the encounter.
        ai_pursuit_note_object(actor_index, self->encounter_index, candidate->firing_position_index,
            *(int32_t *)((uint8_t *)query + 0xc));
        sighting_count = 7;
        missed = 0;
        last_tick = tick;
    }

    if (query->score_instead_of_reject == 0) {
        if (missed != 0) {
            candidate->rejected = 1;
            if (query->collect_all == 0) {
                candidate->valid = 0;
            }
        }
    } else if (missed != 0) {
        candidate->score = candidate->score + 15.0f;
    }

    if (candidate->valid != 0) {
        bonus = 0.0f;
        if (last_tick == -1 || last_tick + 300 < tick) {
            bonus = 10.0f;
        } else if (last_tick < tick) {
            bonus = (float)(tick - last_tick) * 0.033333335f;
        }
        candidate->score = candidate->score + bonus;

        bonus = 0.0f;
        if (sighting_count < 4) {
            bonus = (float)(4 - sighting_count) * 5.0f;
        }
        candidate->score = candidate->score + bonus;
    }

    return candidate->valid;
}

#if 0
Original Ghidra decompilation (0x412350):

/* WARNING: Type propagation algorithm not settling */

char FUN_00412350(uint param_1,int param_2,int param_3)

{
  int iVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  int iVar5;
  short sVar6;
  bool bVar7;
  int local_8 [2];

  iVar1 = *(int *)(DAT_006f1d6c + 0xc);
  iVar5 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  local_8[0] = -1;
  local_8[1] = 0;
  if (param_3 != 0) {
    if ((*(short *)(param_3 + 6) != 0) || (6.0 <= *(float *)(param_3 + 8))) {
      cVar4 = FUN_00436b90(*(undefined4 *)(iVar5 + 0x34),local_8 + 1,local_8);
      sVar6 = (short)local_8[1];
      bVar7 = cVar4 == '\0';
    }
    else {
      FUN_00436b10(*(undefined4 *)(iVar5 + 0x34));
      sVar6 = 7;
      bVar7 = false;
      local_8[0] = iVar1;
    }
    if (*(char *)(param_2 + 0x10) == '\0') {
      if ((!bVar7) && (*(undefined1 *)(param_3 + 0x31) = 1, *(char *)(param_2 + 0x14) == '\0')) {
        *(undefined1 *)(param_3 + 0x30) = 0;
      }
    }
    else if (bVar7) {
      *(float *)(param_3 + 0x38) = *(float *)(param_3 + 0x38) + 15.0;
    }
    if (*(char *)(param_3 + 0x30) != '\0') {
      fVar2 = 0.0;
      if ((local_8[0] == -1) || (local_8[0] + 300 < iVar1)) {
        fVar2 = 10.0;
      }
      else if (local_8[0] < iVar1) {
        fVar2 = (float)(iVar1 - local_8[0]) * 0.033333335;
      }
      fVar2 = fVar2 + *(float *)(param_3 + 0x38);
      *(float *)(param_3 + 0x38) = fVar2;
      fVar3 = 0.0;
      if (sVar6 < 4) {
        fVar3 = (float)(4 - sVar6) * 5.0;
      }
      *(float *)(param_3 + 0x38) = fVar2 + fVar3;
    }
    return *(char *)(param_3 + 0x30);
  }
  return '\0';
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
