// object_mark_pending_delete
// address 0x4f50f0, size 57 bytes
// name confidence: 0.7 (still FUN_004f50f0 in Ghidra; types/objects.h's
//   _object_header_active_bit comment names this function directly: "object_mark_pending_delete
//   sets it")
// rewrite confidence: 0.75
// evidence: types/objects.h object_header (flags at 0x02, data at 0x08),
//   object (flags at 0x10 with _object_do_not_delete_bit, parent_object at 0x11c).
// register convention: object index in EAX (in_EAX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

void object_mark_pending_delete(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if ((header->flags & _object_header_active_bit) == 0 &&
        (obj->flags & _object_do_not_delete_bit) == 0 &&
        obj->parent_object == k_datum_index_none) {
        header->flags |= _object_header_active_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4f50f0):

void FUN_004f50f0(void)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  uint in_EAX;

  bVar2 = *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + (in_EAX & 0xffff) * 0xc);
  iVar1 = *(int *)(DAT_008603b0 + 0x34) + (in_EAX & 0xffff) * 0xc;
  iVar3 = *(int *)(iVar1 + 8);
  if ((((bVar2 & 1) == 0) && ((*(uint *)(iVar3 + 0x10) & 0x100000) == 0)) &&
     (*(int *)(iVar3 + 0x11c) == -1)) {
    *(byte *)(iVar1 + 2) = bVar2 | 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
