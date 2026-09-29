// ai_conversation_clear_object_references  (Ghidra: squad_clear_unit_references, renamed
// per ai_types_notes.md)
// address 0x430d30, size 354 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/ai_types_notes.md's misattribution table names this address
// ai_conversation_clear_object_references. squad_despawn is likewise renamed to
// ai_conversation_stop per the same table.
// register convention: plain __cdecl, matching Ghidra's own recognized signature exactly.
// blam-cc: stack -> object_index, force_full_scan
//
// UNSURE: actor+0xa8 (read/cleared here only when force_full_scan and actor.mode == 12,
// _actor_mode_conversation) falls inside types/ai.h's actor.mode_data.raw union at offset 0x0c;
// not independently named there.
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "ai.h"
#include "fn_ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern Scenario *global_scenario;        // 0x00746f8c
extern data_array *actor_data;           // 0x00880360

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0


// blam-cc: stack -> object_index, force_full_scan
// Clears every ai_conversation instance's direct references to object_index (its two
// speaker-selection handles at unknown_54/unknown_58, and unknown_10). When
// force_full_scan is set, or the conversation's own definition requires it (flags bit 0),
// also scans every flagged participant's unit for a match, clearing a conversation-mode
// callback slot (actor mode_data+0x0c) and conversation_participant when found. Stops the
// instance if object_index was referenced anywhere in it.
void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan)
{
    data_iterator iterator;
    ai_conversation *instance;
    ScenarioAIConversation *definition;
    uint8_t referenced;
    int16_t i;
    int32_t participant_count;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = data_iterator_next(&iterator);
    while (instance != 0) {
        definition = &((ScenarioAIConversation *)global_scenario->ai_conversations.pointer)[instance->definition_index];
        referenced = 0;

        if (instance->speaker_unit == (int32_t)object_index) {
            instance->unknown_63 = 1;
            instance->speaker_unit = -1;
            referenced = 1;
        }
        if (instance->addressee_unit == (int32_t)object_index) {
            instance->addressee_unit = -1;
            referenced = 1;
        }
        if (instance->unknown_10 == object_index) {
            instance->unknown_10 = (datum_index)k_datum_index_none;
            referenced = 1;
        }

        if (force_full_scan != 0 || (definition->flags & 1) != 0) {
            participant_count = definition->participants.count;
            for (i = 0; i < participant_count; i++) {
                if ((instance->participant_mask & (1u << (i & 0x1f))) != 0 &&
                    instance->participant_actor[i] != (datum_index)k_datum_index_none) {
                    actor *a = &((actor *)actor_data->data)[instance->participant_actor[i] & 0xffff];
                    if (a->unit_index == object_index) {
                        referenced = 1;
                    }
                    if (force_full_scan != 0) {
                        if (a->mode == 12 && *(int32_t *)(a->mode_data.raw + 0x0c) == (int32_t)object_index) {
                            *(int32_t *)(a->mode_data.raw + 0x0c) = -1;
                        }
                        if (a->conversation_participant == object_index) {
                            a->conversation_participant = (datum_index)k_datum_index_none;
                        }
                    }
                }
            }
            if (referenced) {
                ai_conversation_stop(iterator.index, 0, 0);
                return;
            }
        }
        instance = data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x430d30):

void __cdecl squad_clear_unit_references(int object_index,char force_full_scan)

{
  uint uVar1;
  int iVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  bool bVar8;
  bool bVar9;

  iVar2 = data_iterator_next();
  if (iVar2 == 0) {
    return;
  }
  do {
    iVar6 = *(short *)(iVar2 + 2) * 0x74 + *(int *)(global_scenario + 0x46c);
    bVar7 = *(int *)(iVar2 + 0x54) == object_index;
    if (bVar7) {
      *(undefined1 *)(iVar2 + 99) = 1;
      *(undefined4 *)(iVar2 + 0x54) = 0xffffffff;
    }
    bVar8 = *(int *)(iVar2 + 0x58) == object_index;
    if (bVar8) {
      *(undefined4 *)(iVar2 + 0x58) = 0xffffffff;
    }
    bVar9 = *(int *)(iVar2 + 0x10) == object_index;
    if (bVar9) {
      *(undefined4 *)(iVar2 + 0x10) = 0xffffffff;
    }
    bVar9 = bVar9 || (bVar8 || bVar7);
    if ((force_full_scan != '\0') || ((*(byte *)(iVar6 + 0x20) & 1) != 0)) {
      iVar4 = 0;
      sVar3 = 0;
      if (0 < *(int *)(iVar6 + 0x50)) {
        do {
          if (((*(uint *)(iVar2 + 0x14) & 1 << ((byte)iVar4 & 0x1f)) != 0) &&
             (uVar1 = *(uint *)(iVar2 + 0x28 + iVar4 * 4), uVar1 != 0xffffffff)) {
            iVar4 = (uVar1 & 0xffff) * 0x724;
            iVar5 = iVar4 + *(int *)(DAT_00880360 + 0x34);
            if (*(int *)(iVar4 + 0x18 + *(int *)(DAT_00880360 + 0x34)) == object_index) {
              bVar9 = true;
            }
            if (force_full_scan != '\0') {
              if ((*(short *)(iVar5 + 0x6c) == 0xc) && (*(int *)(iVar5 + 0xa8) == object_index)) {
                *(undefined4 *)(iVar5 + 0xa8) = 0xffffffff;
              }
              if (*(int *)(iVar5 + 0x1e0) == object_index) {
                *(undefined4 *)(iVar5 + 0x1e0) = 0xffffffff;
              }
            }
          }
          sVar3 = sVar3 + 1;
          iVar4 = (int)sVar3;
        } while (iVar4 < *(int *)(iVar6 + 0x50));
      }
      if (bVar9) {
        squad_despawn(0xffffffff,'\0','\0');
        return;
      }
    }
    iVar2 = data_iterator_next();
    if (iVar2 == 0) {
      return;
    }
  } while( true );
}
#endif
