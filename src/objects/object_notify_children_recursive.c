// object_notify_children_recursive  (Ghidra: FUN_004f7b00; renamed, Blam-style, not previously
// named)
// address 0x4f7b00, size 99 bytes
// name confidence: 0.35 (matches functions.md's summary: "Recursively visits an object and all
//   of its attached children/siblings, invoking a notification callback on each valid one")
// rewrite confidence: 0.5
// evidence: types/objects.h object (definition_tag 0x000, first_child_object 0x118,
//   next_object 0x114); global 0x008603b0 object_data, global 0x0087bc14 tag_instances;
//   callee predicted_resource_list_touch (0x4449f0, same callee as object_notify_predicted_resources_if_valid,
//   this batch).
// register convention: object index is the sole, genuinely-stack, parameter (Ghidra's own
//   "FUN_004f7b00(uint param_1)"). Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f7b00 mov eax,[esp+0x4] at entry.

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

extern void predicted_resource_list_touch(uint8_t *predicted_resources_field); // 0x4449f0, this batch

void object_notify_children_recursive(uint32_t object_index)
{
    while (object_index != k_datum_index_none) {
        object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

        if (obj->definition_tag != k_datum_index_none) {
            uint8_t *tag_data = (uint8_t *)tag_instances[obj->definition_tag & 0xffff].data;
            predicted_resource_list_touch(tag_data + 0x170);
        }

        object_notify_children_recursive(obj->first_child_object);
        object_index = obj->next_object;
    }
}

#if 0
Original Ghidra decompilation (0x4f7b00):

void FUN_004f7b00(uint param_1)

{
  int *piVar1;

  while (param_1 != 0xffffffff) {
    piVar1 = *(int **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
    if (*piVar1 != -1) {
      FUN_004449f0();
    }
    FUN_004f7b00(piVar1[0x46]);
    param_1 = piVar1[0x45];
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
