// object_remove_from_sibling_list  (Ghidra: FUN_004f8fe0; renamed per types/objects.h's own
// object 0x114/0x118/0x11c/0x120 comment, which cites this exact address by this name:
// "object_remove_from_sibling_list 0x4f8fe0")
// address 0x4f8fe0, size 69 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.55
// evidence: types/objects.h object (next_object 0x114); global 0x008603b0 object_data.
// register convention: a list-slot pointer in EDX, the target object index in EDI. Confirmed
//   against objdump -d -M intel bin/halo.exe: 0x4f8fe0 cmp [edx],0xffffffff at entry with no
//   stack access.
//   // blam-cc: EDX -> slot, EDI -> target_object_index
// UNSURE: src/objects/object_unlink_cluster_or_notify_parent.c (written outside this batch)
//   declares this callee as taking only an object index in EAX; this rewrite's own
//   disassembly instead shows a slot pointer in EDX and the target in EDI, with no EAX read at
//   all. That earlier file's extern is not corrected here (out of scope).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

void object_remove_from_sibling_list(datum_index *slot, uint32_t target_object_index) // blam-cc: EDX -> slot, EDI -> target_object_index
{
    if (*slot != k_datum_index_none) {
        object *node;
        do {
            node = ((object_header *)object_data->data)[*slot & 0xffff].data;
            if (*slot == target_object_index) {
                break;
            }
            slot = &node->next_object;
        } while (*slot != k_datum_index_none);

        if (*slot == target_object_index) {
            *slot = node->next_object;
            node->next_object = k_datum_index_none;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f8fe0):

void FUN_004f8fe0(void)

{
  int iVar1;
  uint *in_EDX;
  uint unaff_EDI;

  if (*in_EDX != 0xffffffff) {
    while (iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*in_EDX & 0xffff) * 0xc),
          *in_EDX != unaff_EDI) {
      in_EDX = (uint *)(iVar1 + 0x114);
      if (*in_EDX == 0xffffffff) {
        return;
      }
    }
    *in_EDX = *(uint *)(iVar1 + 0x114);
    *(undefined4 *)(iVar1 + 0x114) = 0xffffffff;
  }
  return;
}
#endif
