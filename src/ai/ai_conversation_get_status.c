// ai_conversation_get_status  (Ghidra: ai_conversation_get_status; renamed per ai_types_notes.md)
// address 0x430830, size 282 bytes
// name confidence: 0.5   rewrite confidence: 0.55
// evidence: out/phase4/ai_types_notes.md's misattribution table: this address returns "the
// status of the conversation instances of one ScenarioAIConversation", not a squad status.
// Iterates ai_conversation_data (types/ai.h) via the plain data_iterator, then falls back to
// ai_globals's conversation_events ring for a recently-stopped instance.
// register convention: plain __cdecl, one stack argument.
// blam-cc: stack -> conversation_definition_index
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern data_array *ai_conversation_data; // 0x008802d4
extern ai_globals *ai_globals_ptr;       // 0x00880354

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: stack -> conversation_definition_index
// Returns the highest status among all live ai_conversation instances of
// conversation_definition_index (1 = not yet started, 2 = started but not primed, 3/4 =
// running, plain/flagged). If none are live, checks the recent-stop event ring for the most
// recently stopped instance of this definition and reports its outcome as 5 (aborted) or
// 6/7 (finished, flagged/plain). Returns 0 if nothing at all is found.
int32_t ai_conversation_get_status(int16_t conversation_definition_index)
{
    data_iterator iterator;
    ai_conversation *instance;
    int32_t best;
    int32_t status;
    int16_t i;
    int16_t best_index;
    int32_t best_tick;
    ai_conversation_event *event;

    best = 0;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;
    instance = data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            if (instance->weighted_actor_count == 0) {
                status = 1;
            } else if (instance->unknown_05 == 0) {
                status = 2;
            } else {
                status = (instance->unknown_07[1] != 0) + 3;
            }
            if ((uint16_t)best <= (uint16_t)status) {
                best = status;
            }
        }
        instance = data_iterator_next(&iterator);
    }
    if ((int16_t)best != 0) {
        return best;
    }

    best_tick = -1;
    best_index = -1;
    if (0 < ai_globals_ptr->conversation_event_count) {
        for (i = 0; i < ai_globals_ptr->conversation_event_count; i++) {
            event = &ai_globals_ptr->conversation_events[i];
            if (event->definition_index == conversation_definition_index && best_tick < event->tick) {
                best_tick = event->tick;
                best_index = i;
            }
        }
        if (best_index != -1) {
            event = &ai_globals_ptr->conversation_events[best_index];
            if (event->reason_a == 0) {
                return 7 - (event->reason_b != 0);
            }
            return 5;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x430830):

int FUN_00430830(short param_1)

{
  int iVar1;
  int iVar2;
  int *piVar3;
  int iVar4;
  int local_14;

  iVar4 = 0;
  local_14 = 0;
  local_14._0_2_ = 0;
  iVar1 = data_iterator_next();
  if (iVar1 != 0) {
    do {
      if (*(short *)(iVar1 + 2) == param_1) {
        if (*(char *)(iVar1 + 6) == '\0') {
          iVar1 = 1;
        }
        else if (*(char *)(iVar1 + 5) == '\0') {
          iVar1 = 2;
        }
        else {
          iVar1 = (*(char *)(iVar1 + 8) != '\0') + 3;
        }
        local_14 = iVar4;
        if ((ushort)iVar4 <= (ushort)iVar1) {
          iVar4 = iVar1;
          local_14 = iVar1;
        }
      }
      iVar1 = data_iterator_next();
    } while (iVar1 != 0);
    if ((short)iVar4 != 0) {
      return iVar4;
    }
  }
  iVar4 = -1;
  iVar1 = -1;
  iVar2 = 0;
  if (0 < *(short *)(DAT_00880354 + 0x2c)) {
    piVar3 = (int *)(DAT_00880354 + 0x34);
    do {
      if (((short)piVar3[-1] == param_1) && (iVar4 < *piVar3)) {
        iVar4 = *piVar3;
        iVar1 = iVar2;
      }
      iVar2 = iVar2 + 1;
      piVar3 = piVar3 + 4;
    } while ((short)iVar2 < *(short *)(DAT_00880354 + 0x2c));
    if ((short)iVar1 != -1) {
      iVar4 = ((short)iVar1 + 3) * 0x10;
      if (*(char *)(iVar4 + 2 + DAT_00880354) == '\0') {
        return 7 - (uint)(*(char *)(iVar4 + DAT_00880354 + 3) != '\0');
      }
      return 5;
    }
  }
  return CONCAT22((short)((uint)iVar2 >> 0x10),(undefined2)local_14);
}
#endif
