// object_clear_pending_delete_flag
// address 0x4f5130, size 33 bytes
// name confidence: 0.7 (still FUN_004f5130 in Ghidra; types/objects.h's
//   _object_header_active_bit comment names this function directly as the paired clearer of
//   object_mark_pending_delete)
// rewrite confidence: 0.8
// evidence: types/objects.h object_header (flags at 0x02).
// register convention: object index in EAX (in_EAX).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

void object_clear_pending_delete_flag(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);

    if ((header->flags & _object_header_active_bit) != 0) {
        header->flags &= (uint8_t)~_object_header_active_bit;
    }
}

#if 0
Original Ghidra decompilation (0x4f5130):

void FUN_004f5130(void)

{
  int iVar1;
  byte bVar2;
  uint in_EAX;

  iVar1 = *(int *)(DAT_008603b0 + 0x34) + (in_EAX & 0xffff) * 0xc;
  bVar2 = *(byte *)(iVar1 + 2);
  if ((bVar2 & 1) != 0) {
    *(byte *)(iVar1 + 2) = bVar2 & 0xfe;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
