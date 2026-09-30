// ai_mark_recognized_objects_for_reaction  (Ghidra: ai_mark_recognized_objects_for_reaction; named for this rewrite)
// address 0x42ba80, size 287 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: phase-4 summary ("for actors belonging to either of two given teams, marks all
// of their recognized objects belonging to the matching team with reaction flags"). Nearly
// identical in shape to ai_notify_actors_of_encounter_state_change.c (0x42b940), which
// already recovered actor_iterator_next's dropped iterator-state argument from disassembly;
// reused here rather than re-deriving it.
// register convention: plain __cdecl, all three arguments on the stack.
// blam-cc: stack -> team_a, team_b, status
//
// UNSURE: actor_target_update_active_flag and team_pair_override_clear_flag are both called with no argument Ghidra could trace;
// see ai_notify_actors_of_encounter_state_change.c for the same actor_target_update_active_flag case.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, EAX, EDI
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50
extern void team_pair_override_clear_flag(int16_t index_b, int16_t index_a); // 0x45c0f0, EBX -> index_b, EDI -> index_a

// blam-cc: stack -> team_a, team_b, status
// For every actor whose team matches team_a or team_b, walks its prop list and, for each
// prop belonging to the OTHER of the two teams, unconditionally marks it for a reaction
// (unknown_61 set, unknown_62 cleared), stamps its status byte, and restamps its engaged
// flag and desirability score. Finishes with a pair-relationship cleanup call.
void ai_mark_recognized_objects_for_reaction(int16_t team_a, int16_t team_b, uint8_t status)
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    int16_t other_team;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.next_index = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
        iterator.encounterless_done = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.next_actor_index = -1;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        other_team = team_b;
        if (a->team != team_a) {
            other_team = team_a;
        }
        if ((a->team == team_a || a->team == team_b) && other_team != (int16_t)-1) {
            actor_index = iterator.actor_index /* the full handle, salt included */;

            prop_cursor = a->first_prop;
            while (prop_cursor != (datum_index)k_datum_index_none) {
                current_prop_index = prop_cursor;
                p = &((prop *)prop_data->data)[current_prop_index & 0xffff];
                prop_cursor = p->next_in_actor;
                if (p->team == other_team) {
                    p->allegiance = 1;
                    p->team_pair_status = 0;
                    p->enemy = status;
                    p->engaged = actor_target_update_active_flag(actor_index, current_prop_index); // FIXED: EDI = the prop (0x42bb29)
                    p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
                }
            }
        }
        a = actor_iterator_next(&iterator);
    }
    team_pair_override_clear_flag(team_b, team_a); // FIXED: arguments from the binary call site (the draft passed none) (0x42bb92: EBX = arg 2, EDI = arg 1)
}

#if 0
Original Ghidra decompilation (0x42ba80):

void FUN_0042ba80(short param_1,short param_2,undefined1 param_3)

{
  uint uVar1;
  undefined1 uVar2;
  int iVar3;
  short sVar4;
  uint uVar5;
  float10 fVar6;
  uint local_8;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_8 = 0xffffffff;
  }
  iVar3 = actor_iterator_next();
  while (iVar3 != 0) {
    sVar4 = param_2;
    if (((*(short *)(iVar3 + 0x3e) == param_1) ||
        (sVar4 = param_1, *(short *)(iVar3 + 0x3e) == param_2)) && (sVar4 != -1)) {
      uVar1 = *(uint *)((local_8 & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
      while (uVar5 = uVar1, uVar5 != 0xffffffff) {
        iVar3 = (uVar5 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
        uVar1 = *(uint *)(iVar3 + 8);
        if (*(short *)(iVar3 + 0x12) == sVar4) {
          *(undefined1 *)(iVar3 + 0x61) = 1;
          *(undefined1 *)(iVar3 + 0x62) = 0;
          *(undefined1 *)(iVar3 + 0x60) = param_3;
          uVar2 = FUN_0041fc60();
          *(undefined1 *)(iVar3 + 0xa4) = uVar2;
          fVar6 = (float10)actor_rate_potential_target(local_8,uVar5);
          *(float *)(iVar3 + 0x50) = (float)fVar6;
        }
      }
    }
    iVar3 = actor_iterator_next();
  }
  FUN_0045c0f0();
  return;
}
#endif
