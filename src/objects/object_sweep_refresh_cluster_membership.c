// object_sweep_refresh_cluster_membership  (Ghidra: FUN_004f74f0; renamed, Blam-style, not
// previously named)
// address 0x4f74f0, size 123 bytes
// name confidence: 0.35 (matches functions.md's summary: "Sweeps all objects performing a
//   per-object visibility/PVS-related update pass")
// rewrite confidence: 0.5
// evidence: types/objects.h object (flags 0x10 with _object_needs_cluster_update_bit,
//   parent_object 0x11c); callees object_iterator_next (0x4f6f20, this batch),
//   object_unlink_cluster_or_notify_parent (0x4f5de0, established: EAX -> object_index),
//   object_type_definitions_notify_0x54 (0x4f43a0, outside this batch, established:
//   EBX -> object_index).
// register convention: no parameters. Confirmed against objdump -d -M intel bin/halo.exe: the
//   whole function reads nothing from the incoming stack or registers before building its own
//   local iterator.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern object *object_iterator_next(object_iterator *iterator); // 0x4f6f20, this batch
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0
extern void object_type_definitions_notify_0x54(uint32_t object_index); // 0x4f43a0, outside this batch

void object_sweep_refresh_cluster_membership(void)
{
    object_iterator iterator;
    object *obj;

    iterator.type_mask = 0xffffffff;
    iterator.flags_mask = 0;
    iterator.index = 0;
    iterator.handle = k_datum_index_none;

    obj = object_iterator_next(&iterator);
    while (obj != (object *)0) {
        if (((obj->flags & _object_needs_cluster_update_bit) != 0) &&
            (obj->parent_object == k_datum_index_none)) {
            object_unlink_cluster_or_notify_parent(iterator.handle);
            obj->flags |= _object_needs_cluster_update_bit;
        }
        object_type_definitions_notify_0x54(iterator.handle);
        obj = object_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x4f74f0):

void FUN_004f74f0(void)

{
  int iVar1;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0x86868686;
  local_10 = 0xffffffff;
  local_c = 0;
  local_a = 0;
  local_8 = 0xffffffff;
  iVar1 = object_iterator_next(&local_10);
  while (iVar1 != 0) {
    if (((*(uint *)(iVar1 + 0x10) & 0x800) != 0) && (*(int *)(iVar1 + 0x11c) == -1)) {
      FUN_004f5de0();
      *(uint *)(iVar1 + 0x10) = *(uint *)(iVar1 + 0x10) | 0x800;
    }
    FUN_004f43a0();
    iVar1 = object_iterator_next(&local_10);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
