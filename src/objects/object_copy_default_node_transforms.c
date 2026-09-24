// object_copy_default_node_transforms  (named by out/phase4/objects_types_notes.md, cited
// directly by this address/name at several places, e.g. "object_copy_default_node_transforms
// 0x4f6b70 copies from obj + *(int16 *)(obj+0x1ee) to obj + *(int16 *)(obj+0x1ea)")
// address 0x4f6b70, size 146 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.6
// evidence: types/objects.h object (definition_tag 0x000, node_function_values 0x1e8,
//   node_function_defaults 0x1ec, unknown_0d4 0x0d4, node_function_count 0x0d6);
//   types/tags.h Object.model, GBXModel.nodes (TagReflexive); global 0x008603b0 object_data,
//   global 0x0087bc14 tag_instances.
// register convention: object index in EAX, requested node-function count in DX (in_DX).
//   Confirmed against objdump -d -M intel bin/halo.exe: 0x4f6be7 movsx esi,dx compares the
//   register directly against object+0xd6 minus object+0xd4, with no stack access at all.
//   // blam-cc: EAX -> object_index, DX -> requested_count
// UNSURE: the model's node TagReflexive.count (uint32_t in types/tags.h) is read here with a
//   16-bit movsx (0x4f6bac), preserved as an explicit (int16_t) cast rather than widening it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count)
    // blam-cc: EAX -> object_index, DX -> requested_count
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    GBXModel *model = (GBXModel *)tag_instances[definition->model.tag_id.index & 0xffff].data;
    int32_t byte_count = (int16_t)model->nodes.count << 5; // node_count * 0x20

    uint8_t *src = (uint8_t *)obj + obj->node_function_defaults.offset;
    uint8_t *dst = (uint8_t *)obj + obj->node_function_values.offset;
    int32_t i;
    for (i = 0; i < byte_count; i++) {
        dst[i] = src[i];
    }

    if (requested_count >= (int16_t)(obj->node_function_count - obj->unknown_0d4)) {
        obj->unknown_0d4 = 0;
        obj->node_function_count = requested_count;
    }
}

#if 0
Original Ghidra decompilation (0x4f6b70):

void FUN_004f6b70(void)

{
  uint *puVar1;
  uint in_EAX;
  int iVar2;
  short in_DX;
  undefined4 *puVar3;
  undefined4 *puVar4;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  puVar3 = (undefined4 *)((int)*(short *)((int)puVar1 + 0x1ee) + (int)puVar1);
  puVar4 = (undefined4 *)((int)*(short *)((int)puVar1 + 0x1ea) + (int)puVar1);
  for (iVar2 = ((int)*(short *)(*(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 +
                                                            DAT_0087bc14) + 0x34) & 0xffff) * 0x20 +
                                         0x14 + DAT_0087bc14) + 0xb8) & 0x7ffffffU) << 3; iVar2 != 0
      ; iVar2 = iVar2 + -1) {
    *puVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    puVar4 = puVar4 + 1;
  }
  for (iVar2 = 0; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined1 *)puVar4 = *(undefined1 *)puVar3;
    puVar3 = (undefined4 *)((int)puVar3 + 1);
    puVar4 = (undefined4 *)((int)puVar4 + 1);
  }
  if ((int)*(short *)((int)puVar1 + 0xd6) - (int)(short)puVar1[0x35] <= (int)in_DX) {
    *(undefined2 *)(puVar1 + 0x35) = 0;
    *(short *)((int)puVar1 + 0xd6) = in_DX;
  }
  return;
}
#endif
