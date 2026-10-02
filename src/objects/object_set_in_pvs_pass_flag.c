// object_set_in_pvs_pass_flag  (Ghidra: FUN_004f67e0; renamed, Blam-style, not previously named)
// address 0x4f67e0, size 106 bytes
// name confidence: 0.35 (matches functions.md's summary: "Sets or clears a per-object state
//   flag (byte at table-entry+2), invoking a follow-up routine only when transitioning from
//   active to inactive"; the flag toggled is object_header's bit 0x40, which
//   types/objects.h already names _object_header_in_pvs_pass_bit from objects_update.c)
// rewrite confidence: 0.6
// evidence: types/objects.h object_header (flags at 0x02, active/in_pvs_pass bits), object
//   (parent_object 0x11c, location_cluster_index 0x09c); global 0x008603b0 object_data;
//   callee object_mark_pending_delete (0x4f50f0, this batch).
// register convention: object index in EAX (in_EAX), boolean in BL (unaff_BL). Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f67e1 mov ecx,eax / and ecx,0xffff, 0x4f67ff test bl,bl.
//   // blam-cc: EAX -> object_index, BL -> in_pvs
// UNSURE: single caller, not yet reached by this batch's address range, so the exact call site
//   that decides the boolean is not examined here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0, this batch

void object_set_in_pvs_pass_flag(uint32_t object_index, uint8_t in_pvs) // blam-cc: EAX -> object_index, BL -> in_pvs
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;

    if (in_pvs != 0) {
        header->flags |= _object_header_in_pvs_pass_bit;
        if ((obj->parent_object == k_datum_index_none) && (obj->location_cluster_index == -1)) {
            if ((header->flags & _object_header_active_bit) != 0) {
                header->flags &= (uint8_t)~_object_header_active_bit;
            }
        }
    } else {
        uint8_t flags = header->flags & (uint8_t)~_object_header_in_pvs_pass_bit;
        header->flags = flags;
        if ((flags & _object_header_active_bit) == 0) {
            object_mark_pending_delete(object_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f67e0):

void FUN_004f67e0(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  char unaff_BL;

  iVar3 = DAT_008603b0;
  iVar4 = (in_EAX & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
  iVar5 = *(int *)(DAT_008603b0 + 0x34) + iVar4;
  if (unaff_BL == '\0') {
    bVar1 = *(byte *)(iVar5 + 2);
    *(byte *)(iVar5 + 2) = bVar1 & 0xbf;
    if ((bVar1 & 1) == 0) {
      FUN_004f50f0();
      return;
    }
  }
  else {
    *(byte *)(iVar5 + 2) = *(byte *)(iVar5 + 2) | 0x40;
    if ((*(int *)(iVar2 + 0x11c) == -1) && (*(short *)(iVar2 + 0x9c) == -1)) {
      iVar4 = *(int *)(iVar3 + 0x34) + iVar4;
      bVar1 = *(byte *)(iVar4 + 2);
      if ((bVar1 & 1) != 0) {
        *(byte *)(iVar4 + 2) = bVar1 & 0xfe;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
