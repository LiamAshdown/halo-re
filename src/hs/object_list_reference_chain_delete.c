// object_list_reference_chain_delete  (Ghidra: object_list_reference_chain_delete, already named;
// matches out/phase4/hs_types_notes.md's own description of this exact function)
// address 0x48b220, size 40 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/hs.h object_list_reference (next at 0x08); this function's own raw bytes are
//   hs_types_notes.md's cited evidence for that field.
// register convention: reference-node data array in EAX (in_EAX); chain head index in EDX
//   (in_EDX).
//   // blam-cc: EAX -> reference_array, EDX -> chain_head

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern void datum_delete(data_array *array, datum_index handle); // blam-cc: EAX -> array,
    // EDX -> handle; memory module, 0x4d0510

// Deletes every node in the singly-linked reference chain starting at `chain_head`.
void object_list_reference_chain_delete(data_array *reference_array, datum_index chain_head)
{
    object_list_reference *node;
    datum_index next;

    while (chain_head != k_datum_index_none) {
        node = (object_list_reference *)((uint8_t *)reference_array->data +
            (chain_head & 0xffff) * 0x0c);
        next = node->next;
        datum_delete(reference_array, chain_head);
        chain_head = next;
    }
}

#if 0
Original Ghidra decompilation (0x48b220):

void object_list_reference_chain_delete(void)

{
  int iVar1;
  int in_EAX;
  uint in_EDX;

  for (; in_EDX != 0xffffffff; in_EDX = *(uint *)(iVar1 + (in_EDX & 0xffff) * 0xc + 8)) {
    iVar1 = *(int *)(in_EAX + 0x34);
    in_EAX = datum_delete();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
