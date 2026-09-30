// hs_object_list_new_singleton  (Ghidra: FUN_0048ac10)
// address 0x48ac10, size 75 bytes
// name confidence: 0.35 (out/phase4/hs_functions.md: "Allocates and appends a new reference-list
//   node for a given object index, if valid" -- more precisely, allocates a brand new list
//   containing exactly that one object)
// rewrite confidence: 0.6
// evidence: types/hs.h object_list_header (count 0x06, first_reference 0x08); this module's
//   object_list_reference_add (0x48b2a0), whose (header_index, object_index) shape matches this
//   call exactly once the new header's index is threaded through.
// register convention: none (void); object index is the recognized stack parameter (param_1).

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"
#include "fn_memory.h"


    // this module, 0x48b2a0

extern data_array *object_list_header_data; // 0x0087a464

// Allocates a new, empty object_list and, if `object_index` is valid, adds it as the list's only
// element. Returns the new list's handle (k_datum_index_none if the header itself could not be
// allocated).
datum_index hs_object_list_new_singleton(datum_index object_index)
{
    datum_index header_index;
    object_list_header *header;

    header_index = k_datum_index_none;
    if (object_index != k_datum_index_none) {
        header_index = datum_new(object_list_header_data);
        if (header_index != k_datum_index_none) {
            header = (object_list_header *)((uint8_t *)object_list_header_data->data +
                (header_index & 0xffff) * 0x0c);
            header->count = 0;
            header->first_reference = k_datum_index_none;
        }
        object_list_reference_add(header_index, object_index);
    }
    return header_index;
}

#if 0
Original Ghidra decompilation (0x48ac10):

uint FUN_0048ac10(int param_1)

{
  int iVar1;
  uint uVar2;
  undefined8 uVar3;

  uVar2 = 0xffffffff;
  if (param_1 != -1) {
    uVar3 = datum_new();
    uVar2 = (uint)uVar3;
    if (uVar2 != 0xffffffff) {
      iVar1 = *(int *)((int)((ulonglong)uVar3 >> 0x20) + 0x34) + (uVar2 & 0xffff) * 0xc;
      *(undefined2 *)(iVar1 + 6) = 0;
      *(undefined4 *)(iVar1 + 8) = 0xffffffff;
    }
    object_list_reference_add(param_1);
  }
  return uVar2;
}
#endif
