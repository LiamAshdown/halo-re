// ai_object_list_spawn_members  (Ghidra: ai_object_list_spawn_members; named for this rewrite)
// address 0x432a40, size 143 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: walks an object_list (types/hs.h) exactly like ai_object_list_respawn_members
// (0x432e80, this batch), calling ai_reference_spawn_starting_location_object (0x4328c0,
// this batch) for each object with a packed ai reference objdump recovers as a second,
// caller-inherited register argument.
// register convention: confirmed by objdump (bin/halo.exe 0x432a40..0x432acf): EAX ->
// object_list_header, EDI -> packed_reference (inherited unchanged; `push edi; push eax;
// call 0x4328c0` puts EAX -> unit_index and EDI -> packed_reference, matching that
// function's own parameter order).
//   // blam-cc: EAX -> object_list_header, EDI -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern void ai_reference_spawn_starting_location_object(datum_index unit_index, uint32_t packed_reference); // 0x4328c0, this batch

// blam-cc: EAX -> object_list_header, EDI -> packed_reference
void ai_object_list_spawn_members(datum_index object_list_header_handle, uint32_t packed_reference)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data + (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        } else {
            object_index = (datum_index)k_datum_index_none;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        ai_reference_spawn_starting_location_object(object_index, packed_reference);

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data + (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x432a40):

void FUN_00432a40(void)

{
  uint in_EAX;
  uint in_ECX;
  int iVar1;
  uint uVar2;

  iVar1 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar1 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  while (iVar1 != -1) {
    FUN_004328c0(iVar1);
    if (in_ECX == 0xffffffff) {
      iVar1 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar1 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  return;
}
#endif
