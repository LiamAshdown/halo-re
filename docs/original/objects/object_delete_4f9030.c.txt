// object_delete_4f9030  (already named from an earlier phase; the numeric suffix survives
// because two other addresses in this module were already named plain "object_delete")
// address 0x4f9030, size 210 bytes
// name confidence: 0.65 (already carries this name; matches functions.md's summary: "Fully
//   destroys an object, recursively deleting its children, running per-type cleanup callbacks,
//   and releasing its allocated data")
// rewrite confidence: 0.6
// evidence: types/objects.h object (flags 0x10 with _object_in_tracked_list_bit and bit 0x800
//   _object_needs_cluster_update_bit, first_child_object 0x118, next_object 0x114),
//   object_header (flags 0x02 bit 0 active); global 0x008603b0 object_data, global 0x0069b354
//   object_delete_callbacks (3 entries); callees object_list_membership_set (0x4f7450, this
//   batch), object_unlink_cluster_or_notify_parent (0x4f5de0, established),
//   object_type_definitions_notify_0x30 (0x4f3f90, outside this batch, established:
//   EBX -> object_index), widget_delete_all (0x4ffbe0, this batch, UNSURE: EAX assumed pending
//   its own rewrite), object_delete_attachments (0x4f9900, this batch, UNSURE: EAX assumed),
//   object_block_data_free (0x4f7de0, this batch, established: EAX -> array, EDX -> handle).
// register convention: both parameters are plain stack arguments (Ghidra's own
//   "object_delete_4f9030(uint param_1,char param_2)"). Confirmed against objdump -d -M intel
//   bin/halo.exe: 0x4f9031 mov ebx,[esp+0x8] at entry.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern void (*object_delete_callbacks[3])(uint32_t object_index); // 0x0069b354

extern void object_list_membership_set(uint32_t object_index, char add); // 0x4f7450, this batch
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_type_definitions_notify_0x30(uint32_t object_index); // 0x4f3f90, outside this batch, EBX -> object_index
extern void widget_delete_all(uint32_t object_index); // 0x4ffbe0, this batch, UNSURE: EAX assumed
extern void object_delete_attachments(uint32_t object_index); // 0x4f9900, this batch, UNSURE: EAX assumed
extern void object_block_data_free(data_array *array, datum_index handle); // 0x4f7de0, this batch

void object_delete_4f9030(uint32_t object_index, char recurse_siblings)
{
    object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
    object *obj = header->data;
    int i;

    if ((obj->flags & _object_in_tracked_list_bit) != 0) {
        object_list_membership_set(object_index, 0);
    }

    for (i = 0; i < 3; i++) {
        object_delete_callbacks[i](object_index);
    }

    if (obj->first_child_object != k_datum_index_none) {
        object_delete_4f9030(obj->first_child_object, 1);
    }
    if ((recurse_siblings != 0) && (obj->next_object != k_datum_index_none)) {
        object_delete_4f9030(obj->next_object, 1);
    }

    if ((header->flags & _object_header_active_bit) != 0) {
        header->flags &= (uint8_t)~_object_header_active_bit;
    }

    widget_delete_all(object_index);
    object_delete_attachments(object_index);

    if ((obj->flags & _object_needs_cluster_update_bit) != 0) {
        object_unlink_cluster_or_notify_parent(object_index);
    }

    object_type_definitions_notify_0x30(object_index);
    object_block_data_free(object_data, object_index);
}

#if 0
Original Ghidra decompilation (0x4f9030):

void object_delete_4f9030(uint param_1,char param_2)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  undefined **ppuVar4;
  int iVar5;

  iVar5 = (param_1 & 0xffff) * 0xc;
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
  if ((*(uint *)(iVar2 + 0x10) & 0x10000) != 0) {
    FUN_004f7450(0);
  }
  ppuVar4 = &PTR_FUN_0069b354;
  iVar3 = 3;
  do {
    (*(code *)*ppuVar4)(param_1);
    ppuVar4 = ppuVar4 + 1;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  if (*(int *)(iVar2 + 0x118) != -1) {
    object_delete_4f9030(*(int *)(iVar2 + 0x118),1);
  }
  if ((param_2 != '\0') && (*(int *)(iVar2 + 0x114) != -1)) {
    object_delete_4f9030(*(int *)(iVar2 + 0x114),1);
  }
  bVar1 = *(byte *)(*(int *)(DAT_008603b0 + 0x34) + 2 + iVar5);
  if ((bVar1 & 1) != 0) {
    *(byte *)(*(int *)(DAT_008603b0 + 0x34) + iVar5 + 2) = bVar1 & 0xfe;
  }
  widget_delete_all();
  object_delete_attachments();
  if ((*(uint *)(iVar2 + 0x10) & 0x800) != 0) {
    FUN_004f5de0();
  }
  FUN_004f3f90();
  FUN_004f7de0();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
