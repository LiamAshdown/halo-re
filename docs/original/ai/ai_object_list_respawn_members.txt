// ai_object_list_respawn_members  (Ghidra: ai_object_list_respawn_members; named for this rewrite)
// address 0x432e80, size 142 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: walks an object_list (types/hs.h, same header/reference data_arrays as
// ai_reference_build_object_list, this batch) exactly like ai_reference_notify_squad_actions
// (0x432a40, this batch) does, and calls ai_reference_respawn_member (0x432df0, this batch)
// for each object with a packed ai reference recovered by objdump.
// register convention: confirmed by objdump (bin/halo.exe 0x432e80..0x432f0d): EAX ->
// object_list_header, EBX -> packed_reference (inherited unchanged, forwarded to
// ai_reference_respawn_member's own EAX).
//   // blam-cc: EAX -> object_list_header_handle, EBX -> packed_reference

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern void ai_reference_respawn_member(uint32_t packed_reference, datum_index unit_index); // 0x432df0, this batch

// blam-cc: EAX -> object_list_header_handle, EBX -> packed_reference
void ai_object_list_respawn_members(datum_index object_list_header_handle, uint32_t packed_reference)
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
        ai_reference_respawn_member(packed_reference, object_index);

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
Original Ghidra decompilation (0x432e80):

void FUN_00432e80(void)

{
  uint in_EAX;
  uint in_ECX;
  uint uVar1;
  int iVar2;

  iVar2 = -1;
  if (in_EAX != 0xffffffff) {
    uVar1 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar1 == 0xffffffff) {
      iVar2 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar1 = uVar1 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar1 * 0xc);
      iVar2 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar1 * 0xc + 4);
    }
  }
  while (iVar2 != -1) {
    FUN_00432df0();
    if (in_ECX == 0xffffffff) {
      iVar2 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar1 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar1 * 0xc);
      iVar2 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar1 * 0xc + 4);
    }
  }
  return;
}

Real disassembly confirms EBX -> packed_reference:
00432ed5: mov eax,ebx
00432ed7: call 0x432df0
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
