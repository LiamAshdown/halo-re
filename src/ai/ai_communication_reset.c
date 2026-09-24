// ai_communication_reset  (Ghidra: ai_communication_reset; named for this rewrite)
// address 0x42d230, size 221 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/ai.h's own header note credits this address with zeroing ai_globals+0x10,
// +0x14..0x28, +0x2c/+0x2e and the 0x40-dword conversation_events ring at +0x30. The trailing
// data_array reset (clearing next_index/last_index/actual_count, reseeding next_identifier
// from the array's own name bytes with the high salt bit forced, marking it valid, then
// zeroing every element's datum_header) is manual open-coded data_new-style bookkeeping over
// the ai_conversation data_array types/ai.h already documents.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern int16_t communication_line_count; // 0x006f0c98
extern int32_t communication_line_base;  // 0x006f0c9c
extern int16_t conversation_line_count;  // 0x006f0ca0
extern int32_t conversation_line_base;   // 0x006f0ca4
extern data_array *ai_conversation_data; // 0x008802d4

extern char *_strncpy(char *dest, const char *src, uint32_t count); // 00623a90

// blam-cc: (no arguments)
// Resets the AI communication timers and the ai_conversation data_array to a clean state,
// used during AI subsystem (re)initialization: marks communication valid, clears the six
// per-index handle fields and the conversation-event ring, invalidates every entry in both
// communication-line timestamp tables, and manually re-homes the ai_conversation data_array
// (resetting its counts, reseeding its salt counter from its own name, marking it valid, and
// clearing every element's datum identifier).
void ai_communication_reset(void)
{
    int32_t i;
    int32_t entry_count;
    int32_t *entries;
    uint8_t *element;

    ai_globals_ptr->communication_valid = 1;
    ai_globals_ptr->unknown_14 = (datum_index)0;
    ai_globals_ptr->unknown_18 = (datum_index)0;
    ai_globals_ptr->unknown_1c = (datum_index)0;
    ai_globals_ptr->unknown_20 = (datum_index)0;
    ai_globals_ptr->unknown_24 = (datum_index)0;
    ai_globals_ptr->unknown_28 = (datum_index)0;

    entries = (int32_t *)communication_line_base;
    entry_count = communication_line_count * 2;
    for (i = 0; i < entry_count; i++) {
        entries[i * 2] = -1;
        entries[i * 2 + 1] = -1;
    }

    entries = (int32_t *)conversation_line_base;
    entry_count = conversation_line_count * 2;
    for (i = 0; i < entry_count; i++) {
        entries[i * 2] = -1;
        entries[i * 2 + 1] = -1;
    }

    ai_globals_ptr->conversation_event_count = 0;
    ai_globals_ptr->conversation_event_cursor = 0;
    {
        uint8_t *ring = (uint8_t *)ai_globals_ptr->conversation_events;
        for (i = 0; i < 0x100; i++) {
            ring[i] = 0;
        }
    }

    ai_conversation_data->next_index = 0;
    ai_conversation_data->last_index = 0;
    ai_conversation_data->actual_count = 0;
    _strncpy((char *)&ai_conversation_data->next_identifier, ai_conversation_data->name, 2);
    ai_conversation_data->next_identifier |= 0x8000;
    ai_conversation_data->valid = 1;

    for (i = 0; i < ai_conversation_data->maximum_count; i++) {
        element = (uint8_t *)ai_conversation_data->data + (int32_t)ai_conversation_data->size * i;
        *(int16_t *)element = 0;
    }
}

#if 0
Original Ghidra decompilation (0x42d230):

void FUN_0042d230(void)

{
  ushort *_Dest;
  char *_Source;
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  short sVar5;
  undefined4 *puVar6;

  iVar1 = DAT_00880354;
  *(undefined1 *)(DAT_00880354 + 0x10) = 1;
  iVar2 = 0;
  *(undefined4 *)(iVar1 + 0x14) = 0;
  *(undefined4 *)(iVar1 + 0x18) = 0;
  *(undefined4 *)(iVar1 + 0x1c) = 0;
  *(undefined4 *)(iVar1 + 0x20) = 0;
  *(undefined4 *)(iVar1 + 0x24) = 0;
  *(undefined4 *)(iVar1 + 0x28) = 0;
  iVar4 = DAT_006f0c9c;
  iVar3 = (int)DAT_006f0c98;
  sVar5 = 0;
  if (0 < iVar3 * 2) {
    do {
      sVar5 = sVar5 + 1;
      *(undefined4 *)(iVar4 + 4 + iVar2 * 8) = 0xffffffff;
      *(undefined4 *)(iVar4 + iVar2 * 8) = 0xffffffff;
      iVar2 = (int)sVar5;
    } while (iVar2 < iVar3 * 2);
  }
  iVar4 = DAT_006f0ca4;
  iVar2 = (int)DAT_006f0ca0;
  sVar5 = 0;
  if (0 < iVar2 * 2) {
    iVar3 = 0;
    do {
      sVar5 = sVar5 + 1;
      *(undefined4 *)(iVar4 + 4 + iVar3 * 8) = 0xffffffff;
      *(undefined4 *)(iVar4 + iVar3 * 8) = 0xffffffff;
      iVar3 = (int)sVar5;
    } while (iVar3 < iVar2 * 2);
  }
  *(undefined2 *)(iVar1 + 0x2c) = 0;
  *(undefined2 *)(iVar1 + 0x2e) = 0;
  puVar6 = (undefined4 *)(iVar1 + 0x30);
  for (iVar4 = 0x40; _Source = DAT_008802d4, iVar4 != 0; iVar4 = iVar4 + -1) {
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  }
  _Dest = (ushort *)(DAT_008802d4 + 0x32);
  DAT_008802d4[0x24] = '\x01';
  _Source[0x2e] = '\0';
  _Source[0x2f] = '\0';
  _Source[0x30] = '\0';
  _Source[0x31] = '\0';
  _Source[0x2c] = '\0';
  _Source[0x2d] = '\0';
  _strncpy((char *)_Dest,_Source,2);
  *_Dest = *_Dest | 0x8000;
  sVar5 = 0;
  if (0 < *(short *)(_Source + 0x20)) {
    do {
      iVar4 = (int)sVar5;
      sVar5 = sVar5 + 1;
      *(undefined2 *)(*(short *)(_Source + 0x22) * iVar4 + *(int *)(_Source + 0x34)) = 0;
    } while (sVar5 < *(short *)(_Source + 0x20));
  }
  return;
}
#endif
