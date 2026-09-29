// object_list_get_first  (Ghidra: FUN_0048b2f0; named per out/phase4/hs_types_notes.md:
// "Iterator state written by object_list_get_first @0x48b2f0")
// address 0x48b2f0, size 70 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/hs.h object_list_header (first_reference 0x08), object_list_reference
//   (object_index 0x04, next 0x08), object_list_iterator (a bare datum_index holding the next
//   reference node).
// register convention: list header index in ECX (in_ECX); output iterator pointer in EDX
//   (in_EDX).
//   // blam-cc: ECX -> header_index, EDX -> iterator_out

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

// Returns the object_index of the first element in the list headed by `header_index` (or
// k_datum_index_none if the list is empty or invalid), and writes the iterator state to continue
// from there into `*iterator_out`.
datum_index object_list_get_first(datum_index header_index, object_list_iterator *iterator_out)
{
    object_list_header *header;
    datum_index first;
    object_list_reference *node;

    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        first = header->first_reference;
        *iterator_out = first;
        if (first != k_datum_index_none) {
            node = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (first & 0xffff) * 0x0c);
            *iterator_out = node->next;
            return node->object_index;
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x48b2f0):

undefined4 FUN_0048b2f0(void)

{
  int iVar1;
  uint uVar2;
  uint in_ECX;
  uint *in_EDX;

  if (in_ECX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
    *in_EDX = uVar2;
    if (uVar2 != 0xffffffff) {
      iVar1 = *(int *)(DAT_0087a468 + 0x34) + (uVar2 & 0xffff) * 0xc;
      *in_EDX = *(uint *)(iVar1 + 8);
      return *(undefined4 *)(iVar1 + 4);
    }
  }
  return 0xffffffff;
}
#endif
