// object_get_position  (Ghidra: object_get_position, already named)
// address 0x4f6900, size 107 bytes
// name confidence: 0.9 (already carries this name from an earlier phase; matches
//   functions.md's summary and the code exactly)
// rewrite confidence: 0.85
// evidence: types/objects.h object (position 0x05c, parent_object 0x11c,
//   parent_marker_index 0x120, nodes.offset 0x1f2); global 0x008603b0 object_data; callee
//   matrix4x3_transform_point (0x4cbde0, already established: out EAX, in EDX, matrix stack).
// register convention: out pointer in EAX, object index in ECX. Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f6900 mov edx,DAT_008603b0 / ... and ecx,0xffff (the
//   index), and eax is never written before being used as the copy/transform destination.
//   // blam-cc: EAX -> out, ECX -> object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m); // 0x4cbde0

void object_get_position(real_point3d *out, uint32_t object_index) // blam-cc: EAX -> out, ECX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->parent_object == k_datum_index_none) {
        *out = obj->position;
        return;
    }

    {
        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);
        matrix4x3_transform_point(out, &obj->position, parent_node);
    }
}

#if 0
Original Ghidra decompilation (0x4f6900):

void object_get_position(void)

{
  int iVar1;
  int iVar2;
  undefined4 *in_EAX;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(uint *)(iVar1 + 0x11c) == 0xffffffff) {
    *in_EAX = *(undefined4 *)(iVar1 + 0x5c);
    in_EAX[1] = *(undefined4 *)(iVar1 + 0x60);
    in_EAX[2] = *(undefined4 *)(iVar1 + 100);
    return;
  }
  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc);
  matrix4x3_transform_point
            ((int)*(short *)(iVar2 + 0x1f2) + *(char *)(iVar1 + 0x120) * 0x34 + iVar2);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
