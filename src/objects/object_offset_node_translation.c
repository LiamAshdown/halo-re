// object_offset_node_translation  (named by out/phase4/objects_types_notes.md: "0x0d6 node
// function count | object_copy_default_node_transforms 0x4f6b70,
// object_offset_node_translation 0x4f6c10")
// address 0x4f6c10, size 68 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.6
// evidence: types/objects.h object (node_function_count 0x0d6, node_function_values 0x1e8);
//   global 0x008603b0 object_data.
// register convention: object index in EAX, delta vector in EDX. Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f6c36 fld [eax+ecx+0x10] then 0x4f6c3c fadd [edx].
//   // blam-cc: EAX -> object_index, EDX -> delta
// UNSURE: the +0x10 field this adds into is inside the first (index 0) 0x20-byte
//   node_function_values entry; its own layout is not otherwise established by this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

void object_offset_node_translation(uint32_t object_index, real_vector3d *delta) // blam-cc: EAX -> object_index, EDX -> delta
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->node_function_count != 0) {
        real_vector3d *translation = (real_vector3d *)
            ((uint8_t *)obj + obj->node_function_values.offset + 0x10);
        translation->i += delta->i;
        translation->j += delta->j;
        translation->k += delta->k;
    }
}

#if 0
Original Ghidra decompilation (0x4f6c10):

void FUN_004f6c10(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  float *in_EDX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  if (*(short *)(iVar1 + 0xd6) != 0) {
    iVar2 = *(short *)(iVar1 + 0x1ea) + iVar1;
    *(float *)(iVar2 + 0x10) = *(float *)(*(short *)(iVar1 + 0x1ea) + 0x10 + iVar1) + *in_EDX;
    *(float *)(iVar2 + 0x14) = in_EDX[1] + *(float *)(iVar2 + 0x14);
    *(float *)(iVar2 + 0x18) = in_EDX[2] + *(float *)(iVar2 + 0x18);
  }
  return;
}
#endif
