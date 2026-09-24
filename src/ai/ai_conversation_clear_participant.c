// ai_conversation_clear_participant  (Ghidra: ai_conversation_clear_participant; renamed per ai_types_notes.md)
// address 0x430c70, size 185 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/ai_types_notes.md's misattribution table: "clears a participant
// reference", not "removes an object index from any squad member slot". squad_despawn
// renamed to ai_conversation_stop per the same table.
// register convention: plain __cdecl, one stack argument.
// blam-cc: stack -> actor_index
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;        // 0x00746f8c

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0, this batch

// blam-cc: stack -> actor_index
// Removes actor_index from every ai_conversation instance's participant list: if the
// conversation definition disallows continuing without this participant (flags bit 0), stops
// the whole instance; otherwise just clears its slot, and if it was the leader participant
// (unknown_4a), marks the instance's unknown_63 flag.
void ai_conversation_clear_participant(datum_index actor_index)
{
    data_iterator iterator;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    int16_t i;
    int32_t participant_count;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = data_iterator_next(&iterator);
    while (instance != 0) {
        definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
        participant_count = definition->participants.count;

        for (i = 0; i < participant_count; i++) {
            if (instance->participant_actor[i] == actor_index) {
                if ((definition->flags & 1) != 0) {
                    ai_conversation_stop(iterator.index, 0, 0);
                    break;
                }
                instance->participant_mask &= ~(1u << (i & 0x1f));
                instance->participant_actor[i] = (datum_index)k_datum_index_none;
                if (instance->unknown_4a == i) {
                    instance->unknown_63 = 1;
                }
            }
        }
        instance = data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x430c70):

void FUN_00430c70(int param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;

  iVar1 = data_iterator_next();
  do {
    if (iVar1 == 0) {
      return;
    }
    iVar2 = *(short *)(iVar1 + 2) * 0x74;
    iVar3 = iVar2 + *(int *)(global_scenario + 0x46c);
    sVar4 = 0;
    if (0 < *(int *)(iVar2 + 0x50 + *(int *)(global_scenario + 0x46c))) {
      iVar2 = 0;
      do {
        if (*(int *)(iVar1 + 0x28 + iVar2 * 4) == param_1) {
          if ((*(byte *)(iVar3 + 0x20) & 1) != 0) {
            squad_despawn(0xffffffff,'\0','\0');
            break;
          }
          *(uint *)(iVar1 + 0x14) = *(uint *)(iVar1 + 0x14) & ~(1 << ((byte)iVar2 & 0x1f));
          *(undefined4 *)(iVar1 + 0x28 + iVar2 * 4) = 0xffffffff;
          if (*(short *)(iVar1 + 0x4a) == sVar4) {
            *(undefined1 *)(iVar1 + 99) = 1;
          }
        }
        sVar4 = sVar4 + 1;
        iVar2 = (int)sVar4;
      } while (iVar2 < *(int *)(iVar3 + 0x50));
    }
    iVar1 = data_iterator_next();
  } while( true );
}
#endif
