// ai_conversation_get_unknown_48  (Ghidra: ai_conversation_get_unknown_48; renamed per ai_types_notes.md)
// address 0x430960, size 94 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/ai_types_notes.md's misattribution table: "a field of the live
// conversation instance", not "the formation/actor-type index on a live squad instance".
// Returns ai_conversation.unknown_48 (types/ai.h: "ai_conversation_new sets 0xffff") for the
// live instance matching conversation_definition_index, or 999 if none is live.
// register convention: BX -> conversation_definition_index (unaff_BX, the only register
// Ghidra's own decompile shows).
// blam-cc: BX -> conversation_definition_index
// reconciled: R16 data_iterator is 0x10 bytes (int16 next_index, +0x0c signature = data ^ 'iter'); the inline constructor now stores the signature like the original

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *ai_conversation_data; // 0x008802d4

extern void * data_iterator_next(data_iterator *iterator); // 0x4d05d0

// blam-cc: BX -> conversation_definition_index
int16_t ai_conversation_get_unknown_48(int16_t conversation_definition_index)
{
    data_iterator iterator;
    ai_conversation *instance;

    iterator.data = ai_conversation_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)k_datum_index_none;
    iterator.signature = (uint32_t)(uintptr_t)iterator.data ^ k_data_iterator_signature;

    instance = (ai_conversation *)data_iterator_next(&iterator);
    while (instance != 0) {
        if (instance->definition_index == conversation_definition_index) {
            return instance->line_index;
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
    return 999;
}

#if 0
Original Ghidra decompilation (0x430960):

undefined2 FUN_00430960(void)

{
  int iVar1;
  short unaff_BX;

  iVar1 = data_iterator_next();
  while( true ) {
    if (iVar1 == 0) {
      return 999;
    }
    if (*(short *)(iVar1 + 2) == unaff_BX) break;
    iVar1 = data_iterator_next();
  }
  return *(undefined2 *)(iVar1 + 0x48);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
