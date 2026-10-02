// object_notify_node_array_if_animated  (Ghidra: FUN_004f8b10; renamed, Blam-style, not
// previously named)
// address 0x4f8b10, size 82 bytes
// name confidence: 0.3 (matches functions.md's summary: "Runs a node-based effect update only
//   when the object's type has both referenced tag definitions present")
// rewrite confidence: 0.5
// evidence: types/objects.h object (nodes 0x1f0); types/tags.h Object.model,
//   Object.animation_graph; global 0x008603b0 object_data, 0x0087bc14 tag_instances; callee
//   object_type_definitions_notify_0x4c (0x4f42c0, outside this batch).
// register convention: object index in EAX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f8b1a mov ebx,eax at entry (ebx then carries the object index across the call).
//   // blam-cc: EAX -> object_index
// UNSURE: this call site passes the object's node array pointer as an extra, visible stack
//   argument (0x4f8b56 push ecx; call 0x4f42c0), which src/objects/object_type_definitions_notify_0x4c.c
//   (written outside this batch) does not declare; see the equivalent note in
//   object_clear_references_to_object.c for the same discrepancy with a sibling notify hook.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_type_definitions_notify_0x4c(uint32_t object_index, void *nodes); // 0x4f42c0, outside this batch; see UNSURE above

void object_notify_node_array_if_animated(uint32_t object_index) // blam-cc: EAX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if ((definition->model.tag_id.index != 0xffff) && (definition->animation_graph.tag_id.index != 0xffff)) {
        object_type_definitions_notify_0x4c(object_index, (uint8_t *)obj + obj->nodes.offset);
    }
}

#if 0
Original Ghidra decompilation (0x4f8b10):

void FUN_004f8b10(void)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if ((*(int *)(iVar2 + 0x34) != -1) && (*(int *)(iVar2 + 0x44) != -1)) {
    FUN_004f42c0((int)*(short *)((int)puVar1 + 0x1f2) + (int)puVar1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
