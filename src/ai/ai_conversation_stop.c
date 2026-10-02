// ai_conversation_stop  (Ghidra: squad_despawn, renamed per ai_types_notes.md)
// address 0x430ea0, size 279 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/ai_types_notes.md's misattribution table names this address
// ai_conversation_stop; it is also "the writer of the ai_globals event ring". The ring
// cursor's mod-16 wrap is simplified from Ghidra's `(cursor+1) & 0x8000000f` (with a
// negative-number fixup) to the plain `& 0xf` types/ai.h itself already documents for this
// exact function ("ai_conversation_event_count = 16, squad_despawn masks the cursor with
// 0xf"); the underlying non-negative int16 counter can never take the fixup path.
// register convention: plain __cdecl, matching Ghidra's own recognized signature exactly.
// blam-cc: stack -> instance, reason_a, reason_b

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;        // 0x00746f8c
extern ai_globals *ai_globals_ptr;       // 0x00880354
extern game_time_globals *game_time;     // 0x006f1d6c
extern data_array *actor_data;           // 0x00880360

extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510

// blam-cc: stack -> instance, reason_a, reason_b
// Stops an ai_conversation instance: records it (definition, reason, current tick) into the
// 16-slot recent-stop ring (advancing and clamping the high-water mark), clears every
// flagged participant actor's conversation_index/conversation_participant (and, in
// conversation mode, its mode_data), then deletes the instance datum.
void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b)
{
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    ai_conversation_event *event;
    int16_t cursor;
    int16_t new_count;
    int16_t i;
    int32_t participant_count;

    if (instance_handle == (datum_index)k_datum_index_none) {
        return;
    }
    instance = &((ai_conversation *)ai_conversation_data->data)[instance_handle & 0xffff];
    definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];

    cursor = ai_globals_ptr->conversation_event_cursor;
    ai_globals_ptr->conversation_event_cursor = (cursor + 1) & 0xf;
    new_count = cursor + 1;
    if (new_count < ai_globals_ptr->conversation_event_count) {
        new_count = ai_globals_ptr->conversation_event_count;
    }
    ai_globals_ptr->conversation_event_count = new_count;

    event = &ai_globals_ptr->conversation_events[cursor];
    event->definition_index = instance->definition_index;
    event->reason_a = reason_a;
    event->reason_b = reason_b;
    event->tick = game_time->game_time;

    participant_count = definition->participants.count;
    for (i = 0; i < participant_count; i++) {
        if ((instance->participant_mask & (1u << (i & 0x1f))) != 0 &&
            instance->participant_actor[i] != (datum_index)k_datum_index_none) {
            actor *a = &((actor *)actor_data->data)[instance->participant_actor[i] & 0xffff];
            a->conversation_index = (datum_index)k_datum_index_none;
            a->conversation_participant = (datum_index)k_datum_index_none;
            if (a->mode == 12) {
                *(int32_t *)a->mode_data.raw = -1;
            }
        }
    }

    datum_delete(ai_conversation_data, instance_handle);
}

#if 0
Original Ghidra decompilation (0x430ea0):

void __cdecl squad_despawn(uint squad_instance_index,uchar reason_a,uchar reason_b)

{
  short *psVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  uint uVar6;
  int iVar7;
  int iVar8;

  iVar3 = DAT_00880354;
  if (squad_instance_index == 0xffffffff) {
    return;
  }
  iVar2 = (squad_instance_index & 0xffff) * 100 + *(int *)(DAT_008802d4 + 0x34);
  iVar8 = *(short *)(iVar2 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
  uVar6 = (int)(short)(*(short *)(DAT_00880354 + 0x2e) + 1) & 0x8000000f;
  if ((int)uVar6 < 0) {
    uVar6 = (uVar6 - 1 | 0xfffffff0) + 1;
  }
  psVar1 = (short *)(DAT_00880354 + 0x2c);
  iVar7 = (int)*(short *)(DAT_00880354 + 0x2e);
  *(short *)(DAT_00880354 + 0x2e) = (short)uVar6;
  iVar4 = iVar7 + 1;
  if (iVar7 + 1 < (int)*psVar1) {
    iVar4 = (int)*psVar1;
  }
  *(short *)(iVar3 + 0x2c) = (short)iVar4;
  *(undefined2 *)((iVar7 + 3) * 0x10 + iVar3) = *(undefined2 *)(iVar2 + 2);
  iVar3 = iVar3 + iVar7 * 0x10;
  *(uchar *)(iVar3 + 0x32) = reason_a;
  *(uchar *)(iVar3 + 0x33) = reason_b;
  *(undefined4 *)(iVar3 + 0x34) = *(undefined4 *)(DAT_006f1d6c + 0xc);
  iVar3 = DAT_00880360;
  sVar5 = 0;
  if (0 < *(int *)(iVar8 + 0x50)) {
    iVar4 = 0;
    do {
      if (((*(uint *)(iVar2 + 0x14) & 1 << ((byte)iVar4 & 0x1f)) != 0) &&
         (uVar6 = *(uint *)(iVar2 + 0x28 + iVar4 * 4), uVar6 != 0xffffffff)) {
        iVar4 = (uVar6 & 0xffff) * 0x724 + *(int *)(iVar3 + 0x34);
        *(undefined4 *)(iVar4 + 0x1dc) = 0xffffffff;
        *(undefined4 *)(iVar4 + 0x1e0) = 0xffffffff;
        if (*(short *)(iVar4 + 0x6c) == 0xc) {
          *(undefined4 *)(iVar4 + 0x9c) = 0xffffffff;
        }
      }
      sVar5 = sVar5 + 1;
      iVar4 = (int)sVar5;
    } while (iVar4 < *(int *)(iVar8 + 0x50));
  }
  datum_delete();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
