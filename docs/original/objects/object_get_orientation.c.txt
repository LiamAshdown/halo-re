// object_get_orientation  (Ghidra: object_get_orientation, already named)
// address 0x4f6970, size 167 bytes
// name confidence: 0.9 (already carries this name from an earlier phase; matches
//   functions.md's summary and the code exactly)
// rewrite confidence: 0.85
// evidence: types/objects.h object (forward 0x074, up 0x080, parent_object 0x11c,
//   parent_marker_index 0x120, nodes.offset 0x1f2); global 0x008603b0 object_data; callee
//   matrix4x3_transform_normal (0x4cbec0, already established: out EAX, normal EDX, matrix
//   stack).
// register convention: out_forward pointer in EAX (may be NULL), object index in ECX,
//   out_up pointer as the single stack parameter (may be NULL). Confirmed against
//   objdump -d -M intel bin/halo.exe 0x4f6970..: mov ebx,[esp+0x8] for the stack arg before
//   ecx is masked into the object index, and eax is tested directly as the forward output.
//   // blam-cc: EAX -> out_forward, ECX -> object_index, stack -> out_up

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0

extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0

void object_get_orientation(real_vector3d *out_forward, uint32_t object_index, real_vector3d *out_up)
    // blam-cc: EAX -> out_forward, ECX -> object_index, stack -> out_up
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    if (obj->parent_object == k_datum_index_none) {
        if (out_forward != (real_vector3d *)0) {
            *out_forward = obj->forward;
        }
        if (out_up != (real_vector3d *)0) {
            *out_up = obj->up;
        }
        return;
    }

    {
        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);

        if (out_forward != (real_vector3d *)0) {
            matrix4x3_transform_normal(out_forward, &obj->forward, parent_node);
        }
        if (out_up != (real_vector3d *)0) {
            matrix4x3_transform_normal(out_up, &obj->up, parent_node);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f6970):

void object_get_orientation(undefined4 *param_1)

{
  int iVar1;
  undefined4 *in_EAX;
  uint in_ECX;
  int iVar2;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(uint *)(iVar2 + 0x11c) == 0xffffffff) {
    if (in_EAX != (undefined4 *)0x0) {
      *in_EAX = *(undefined4 *)(iVar2 + 0x74);
      in_EAX[1] = *(undefined4 *)(iVar2 + 0x78);
      in_EAX[2] = *(undefined4 *)(iVar2 + 0x7c);
    }
    if (param_1 != (undefined4 *)0x0) {
      *param_1 = *(undefined4 *)(iVar2 + 0x80);
      param_1[1] = *(undefined4 *)(iVar2 + 0x84);
      param_1[2] = *(undefined4 *)(iVar2 + 0x88);
      return;
    }
  }
  else {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar2 + 0x11c) & 0xffff) * 0xc);
    iVar2 = (int)*(short *)(iVar1 + 0x1f2) + *(char *)(iVar2 + 0x120) * 0x34 + iVar1;
    if (in_EAX != (undefined4 *)0x0) {
      matrix4x3_transform_normal(iVar2);
    }
    if (param_1 != (undefined4 *)0x0) {
      matrix4x3_transform_normal(iVar2);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
