// ai_conversation_mark_all  (Ghidra: ai_conversation_mark_all; renamed per ai_types_notes.md)
// address 0x430a20, size 77 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/ai_types_notes.md's misattribution table: "marks every live instance
// of a given squad definition with a flag" really operates on conversation instances, not
// squads. Sets ai_conversation.unknown_07[2] (offset 0x09) for every instance matching
// conversation_definition_index.
// register convention: SI -> conversation_definition_index (unaff_SI, the only register
// Ghidra's own decompile shows).
// blam-cc: SI -> conversation_definition_index
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

// blam-cc: SI -> conversation_definition_index
void ai_conversation_mark_all(int16_t conversation_definition_index)
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
            instance->advance = 1;
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x430a20):

void FUN_00430a20(void)

{
  int iVar1;
  short unaff_SI;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if (*(short *)(iVar1 + 2) == unaff_SI) {
      *(undefined1 *)(iVar1 + 9) = 1;
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
