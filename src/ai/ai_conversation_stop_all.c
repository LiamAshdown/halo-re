// ai_conversation_stop_all  (Ghidra: ai_conversation_stop_all; renamed per ai_types_notes.md)
// address 0x4309c0, size 90 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: out/phase4/ai_types_notes.md's misattribution table: "stops every instance of one
// conversation", not "destroys every live squad instance". squad_despawn (0x430ea0) is
// renamed to ai_conversation_stop per the same table.
// register convention: SI -> conversation_definition_index (unaff_SI, the only register
// Ghidra's own decompile shows).
// blam-cc: SI -> conversation_definition_index
//
// UNSURE: Ghidra's own decompile passes ai_conversation_stop the literal -1 as the instance
// handle at every call, which would make it a no-op; the data_iterator's own `index` field
// (the handle of the instance the loop is currently looking at) is the only value that makes
// the surrounding loop meaningful, so it is used here instead.
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
extern void ai_conversation_stop(datum_index instance_handle, uint8_t reason_a, uint8_t reason_b); // 0x430ea0, this batch

// blam-cc: SI -> conversation_definition_index
void ai_conversation_stop_all(int16_t conversation_definition_index)
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
            ai_conversation_stop(iterator.index, 0, 0);
        }
        instance = (ai_conversation *)data_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4309c0):

void FUN_004309c0(void)

{
  int iVar1;
  short unaff_SI;

  iVar1 = data_iterator_next();
  while (iVar1 != 0) {
    if (*(short *)(iVar1 + 2) == unaff_SI) {
      squad_despawn(0xffffffff,'\0','\0');
    }
    iVar1 = data_iterator_next();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
