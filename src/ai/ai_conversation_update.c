// ai_conversation_update  (Ghidra: ai_conversation_update; renamed per ai_types_notes.md)
// address 0x430a70, size 490 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/ai_types_notes.md's misattribution table: "the per-tick conversation
// update", not "per-tick update of every squad instance". squad_despawn renamed to
// ai_conversation_stop, and ai_conversation_activate_next_participant/ai_conversation_current_line_is_ready keep their corrected participant-
// activation/readiness roles from the same table (both outside this batch's address range).
// register convention: plain __cdecl, no parameters (drains the ai_conversation data_array).
// blam-cc: (no arguments)
//
// UNSURE, substantially: the instance handle Ghidra's own decompile calls "squad_instance_index"
// is initialized to -1 and never visibly reassigned before use, which would make every call
// into it a no-op; the data_iterator's own `index` field (the handle of the instance the loop
// is currently on) is used here instead, matching the same pattern already resolved in
// ai_conversation_stop_all.c. The per-participant leader/reference resolution in the middle
// of the loop (actor.conversation_participant selection against the line's two indices and
// its flag bits) is reproduced structurally but not independently verified.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#include "ai.h"

extern game_time_globals *game_time; // 0x006f1d6c
extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;    // 0x00746f8c
extern data_array *actor_data;       // 0x00880360

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0, this batch
extern int8_t ai_conversation_resolve_participants(datum_index instance_index, uint8_t *out_flag); // 0x430fc0, deferred in this batch; UNSURE signature
extern uint8_t ai_conversation_activate_next_participant(void); // 0x431d10, outside this batch; UNSURE signature (activates next participant)
extern uint8_t ai_conversation_current_line_is_ready(void); // 0x431e70, outside this batch; UNSURE signature (participant ready to speak)

// blam-cc: (no arguments)
// Per-tick update of every live ai_conversation instance: every 30 ticks, re-checks whether
// a not-yet-fully-primed instance should keep running (stopping it if not); once primed,
// resolves each bit-flagged participant's actor to a conversation_participant reference
// (from the matching ScenarioAIConversationLine's leader/alternate indices), or, once fully
// resolved, advances through the conversation's lines one activation at a time until either
// a line is not ready or the line list is exhausted, then stops the instance.
void ai_conversation_update(void)
{
    int32_t current_tick;
    data_iterator iterator;
    ai_conversation *instance;
    datum_index handle;
    ScenarioAIConversation *definition;
    char ready;
    int32_t i;
    int32_t participant_count;

    current_tick = game_time->game_time;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;

    instance = data_iterator_next(&iterator);
    while (instance != 0) {
        handle = iterator.index;
        definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];

        if (instance->unknown_06 == 0) {
            ready = 1;
            if ((current_tick - instance->start_tick) % 0x1e == 0) {
                ai_conversation_resolve_participants(handle, &ready);
            }
            if (instance->unknown_06 != 0) {
                goto primed;
            }
            if (ready == 0) {
                ai_conversation_stop(handle, 1, 0);
            }
            if (instance->unknown_06 != 0) {
                goto primed;
            }
            goto resolve_participants;
        }

primed:
        if (instance->unknown_07[0] == 0) {
            char can_advance;
            if (instance->unknown_48 < 0 || definition->lines.count <= instance->unknown_48) {
                can_advance = 0;
            } else {
                can_advance = 1;
            }
            while (1) {
                if (can_advance != 0) {
                    can_advance = ai_conversation_current_line_is_ready();
                    if (can_advance == 0) {
                        goto resolve_participants;
                    }
                }
                instance->unknown_48 = instance->unknown_48 + 1;
                if (definition->lines.count <= instance->unknown_48) {
                    break;
                }
                can_advance = ai_conversation_activate_next_participant();
            }
            instance->unknown_07[0] = 1;
            goto resolve_participants;
        } else {
            ai_conversation_stop(handle, 0, 1);
        }
        instance = data_iterator_next(&iterator);
        continue;

resolve_participants:
        if (instance->unknown_07[0] != 0) {
            participant_count = definition->participants.count;
            for (i = 0; i < participant_count; i++) {
                if ((instance->participant_mask & (1u << (i & 0x1f))) != 0 &&
                    instance->participant_actor[i] != (datum_index)k_datum_index_none) {
                    actor *a = &((actor *)actor_data->data)[instance->participant_actor[i] & 0xffff];
                    datum_index unit_index = a->unit_index;
                    a->conversation_index = handle;
                    a->conversation_participant = (datum_index)k_datum_index_none;
                    if (unit_index == (datum_index)instance->unknown_54) {
                        a->conversation_participant = instance->unknown_58;
                    } else if (unit_index == (datum_index)instance->unknown_58 &&
                               (instance->unknown_4e & 1) != 0) {
                        a->conversation_participant = instance->unknown_54;
                    } else if ((instance->unknown_4e & 2) == 0) {
                        if ((instance->unknown_4e & 4) != 0) {
                            a->conversation_participant = instance->unknown_58;
                        }
                    } else {
                        a->conversation_participant = instance->unknown_54;
                    }
                }
            }
        }
        instance = data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x430a70):

void FUN_00430a70(void)

{
  uint uVar1;
  int iVar2;
  uint squad_instance_index;
  char cVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  char local_19;
  int local_18;
  int local_14;
  uint local_10;
  undefined2 local_c;
  uint local_8;
  uint local_4;

  local_14 = *(int *)(DAT_006f1d6c + 0xc);
  local_10 = DAT_008802d4;
  local_4 = DAT_008802d4 ^ 0x69746572;
  local_c = 0;
  local_8 = 0xffffffff;
  iVar4 = data_iterator_next();
  squad_instance_index = local_8;
  do {
    if (iVar4 == 0) {
      return;
    }
    iVar7 = *(short *)(iVar4 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
    local_8 = squad_instance_index;
    if (*(char *)(iVar4 + 6) == '\0') {
      local_19 = '\x01';
      if ((local_14 - *(int *)(iVar4 + 0xc)) % 0x1e == 0) {
        FUN_00430fc0(squad_instance_index,&local_19);
      }
      if (*(char *)(iVar4 + 6) != '\0') goto LAB_00430b2b;
      if (local_19 == '\0') {
        squad_despawn(squad_instance_index,'\x01','\0');
      }
      if (*(char *)(iVar4 + 6) != '\0') goto LAB_00430b2b;
LAB_00430b79:
      if (*(char *)(iVar4 + 7) != '\0') goto LAB_00430b80;
      if (*(char *)(iVar4 + 6) != '\0') {
        iVar6 = 0;
        local_18 = 0;
        if (0 < *(int *)(iVar7 + 0x50)) {
          do {
            if (((*(uint *)(iVar4 + 0x14) & 1 << ((byte)iVar6 & 0x1f)) != 0) &&
               (uVar1 = *(uint *)(iVar4 + 0x28 + iVar6 * 4), uVar1 != 0xffffffff)) {
              iVar5 = (uVar1 & 0xffff) * 0x724;
              iVar6 = *(int *)(iVar5 + 0x18 + *(int *)(DAT_00880360 + 0x34));
              iVar5 = iVar5 + *(int *)(DAT_00880360 + 0x34);
              *(uint *)(iVar5 + 0x1dc) = squad_instance_index;
              *(undefined4 *)(iVar5 + 0x1e0) = 0xffffffff;
              iVar2 = *(int *)(iVar4 + 0x54);
              if (iVar6 == iVar2) {
                *(undefined4 *)(iVar5 + 0x1e0) = *(undefined4 *)(iVar4 + 0x58);
              }
              else if ((iVar6 == *(int *)(iVar4 + 0x58)) && ((*(byte *)(iVar4 + 0x4e) & 1) != 0)) {
                *(int *)(iVar5 + 0x1e0) = iVar2;
              }
              else if ((*(ushort *)(iVar4 + 0x4e) & 2) == 0) {
                if ((*(ushort *)(iVar4 + 0x4e) & 4) != 0) {
                  *(int *)(iVar5 + 0x1e0) = *(int *)(iVar4 + 0x58);
                }
              }
              else {
                *(int *)(iVar5 + 0x1e0) = iVar2;
              }
            }
            local_18 = local_18 + 1;
            iVar6 = (int)(short)local_18;
          } while (iVar6 < *(int *)(iVar7 + 0x50));
        }
      }
    }
    else {
LAB_00430b2b:
      if (*(char *)(iVar4 + 7) == '\0') {
        if ((*(short *)(iVar4 + 0x48) < 0) ||
           (*(int *)(iVar7 + 0x5c) <= (int)*(short *)(iVar4 + 0x48))) {
          cVar3 = '\0';
        }
        else {
          cVar3 = '\x01';
        }
        while( true ) {
          if ((cVar3 != '\0') && (cVar3 = FUN_00431e70(), cVar3 == '\0')) goto LAB_00430b79;
          *(short *)(iVar4 + 0x48) = *(short *)(iVar4 + 0x48) + 1;
          if (*(int *)(iVar7 + 0x5c) <= (int)*(short *)(iVar4 + 0x48)) break;
          cVar3 = FUN_00431d10();
        }
        *(undefined1 *)(iVar4 + 7) = 1;
        goto LAB_00430b79;
      }
LAB_00430b80:
      squad_despawn(squad_instance_index,'\0','\x01');
    }
    iVar4 = data_iterator_next();
    squad_instance_index = local_8;
  } while( true );
}
#endif
