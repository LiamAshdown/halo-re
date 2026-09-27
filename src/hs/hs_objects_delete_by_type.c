// hs_objects_delete_by_type  (Ghidra: hs_objects_delete_by_type, already named)
// address 0x4887d0, size 152 bytes
// name confidence: 0.55 (out/phase4/hs_functions.md: "Deletes every live object of a given object
//   type, then runs garbage collection and compacts the block list")
// rewrite confidence: 0.4
// evidence: identical object_iterator_next usage and per-object field_04 dispatch (0/3) to
//   hs_object_runtime_cleanup.c, whose iterator shape and object_record slice are reused.
// register convention: TagID to match in ESI (unaff_ESI). No other arguments.
//   // blam-cc: ESI -> tag_id
// UNSURE: `*piVar3 == tag_id` compares the FIRST 4 bytes of whatever object_iterator_next
// returns against `tag_id`; modeled here as the object record's own leading TagID field (offset
// 0x00), which is the standard position for an object's tag reference, but this is inferred, not
// directly evidenced anywhere in this module.

#include "tags.h"
#include "memory.h"
#include "hs.h"

extern void objects_garbage_collection(void); // objects module, 0x4f9c60
extern void block_list_compact(memory_pool *arena); // 0x4d1eb0, EBX
extern memory_pool *object_memory_pool; // 0x006b8cb4
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(datum_index object_index, int32_t param_2); // objects module, 0x4f59d0

// hs_object_iterator_state: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

extern void *object_iterator_next(hs_object_iterator_state *iterator); // objects module, 0x4f6f20

extern data_array *object_headers; // 0x008603b0, stride 0x0c, object data pointer at +0x08

// hs_object_record: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Deletes (or resets, for field_04 == 3) every live object whose tag_id matches `tag_id`, then
// runs a garbage collection pass and compacts the block list.
void hs_objects_delete_by_type(uint32_t tag_id)
{
    hs_object_iterator_state iter;
    hs_object_record *element;
    datum_index object_index;
    hs_object_record *object;

    iter.type_filter = -1;
    iter.next_index = 0;
    iter.index = (datum_index)0xffffffff;
    element = (hs_object_record *)object_iterator_next(&iter);
    for (;;) {
        object_index = iter.index;
        if (element == 0) {
            objects_garbage_collection();
            block_list_compact(object_memory_pool); // 0x488858: EBX = the object pool
            return;
        }
        if (element->tag_id == tag_id) {
            object = *(hs_object_record **)((uint8_t *)object_headers->data +
                (object_index & 0xffff) * 0x0c + 8);
            // role 0 unparents (EDI object) and then deletes like role 3 (the binary falls through)
            if (object->unknown_04 == 0) {
                object_delete_unparented(object_index);
                object_delete_recursive(object_index, 0);
            } else if (object->unknown_04 == 3) {
                object_delete_recursive(object_index, 0);
            }
        }
        element = (hs_object_record *)object_iterator_next(&iter);
    }
}

#if 0
Original Ghidra decompilation (0x4887d0):

void hs_objects_delete_by_type(void)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  int unaff_ESI;
  undefined4 local_10;
  undefined1 local_c;
  undefined2 local_a;
  uint local_8;
  undefined4 local_4;

  local_10 = 0xffffffff;
  local_8 = 0xffffffff;
  local_4 = 0x86868686;
  local_c = 0;
  local_a = 0;
  piVar3 = (int *)object_iterator_next(&local_10);
  uVar2 = local_8;
  do {
    local_8 = uVar2;
    if (piVar3 == (int *)0x0) {
      objects_garbage_collection();
      block_list_compact();
      return;
    }
    if (*piVar3 == unaff_ESI) {
      iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 4);
      if (iVar1 == 0) {
        FUN_004f5aa0();
      }
      else if (iVar1 != 3) goto LAB_00488841;
      FUN_004f59d0(uVar2,0);
    }
LAB_00488841:
    piVar3 = (int *)object_iterator_next(&local_10);
    uVar2 = local_8;
  } while( true );
}
#endif
