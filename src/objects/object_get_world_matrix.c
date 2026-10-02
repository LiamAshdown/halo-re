// object_get_world_matrix  (Ghidra: object_get_world_matrix, already named)
// address 0x4f6a20, size 127 bytes
// name confidence: 0.9 (already carries this name from an earlier phase; matches
//   functions.md's summary and the code exactly)
// rewrite confidence: 0.7
// evidence: types/objects.h object (position 0x05c, forward 0x074, up 0x080,
//   parent_object 0x11c, parent_marker_index 0x120, nodes.offset 0x1f2); types/math.h
//   real_matrix4x3 (position at +0x28); global 0x008603b0 object_data; global 0x00696664
//   matrix4x3_multiply_procedure; callee matrix4x3_from_forward_up (0x4cb970).
// register convention: object index in EAX, output matrix pointer in EDI (Ghidra's
//   "unaff_EDI", not recognized as a formal parameter). Confirmed against
//   objdump -d -M intel bin/halo.exe: 0x4f6a40 push edi immediately before call 0x4cb970 (the
//   stack "out" argument of matrix4x3_from_forward_up), and 0x4f6a9b mov eax,edi / ret, so the
//   function also returns the same pointer it was handed.
//   // blam-cc: EAX -> object_index, EDI -> out
// resolved from disassembly: the attached path multiplies matrix4x3_multiply(parent_node, out,
//   out), i.e. it composes the parent marker's world transform with the rotation-plus-local-
//   position matrix already written into *out, in place -- the same composition
//   object_snap_to_parent_marker_and_detach (0x4f6610, this batch) performs in two steps.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern void matrix4x3_from_forward_up(real_vector3d *up, real_vector3d *forward, real_matrix4x3 *out); // 0x4cb970

real_matrix4x3 *object_get_world_matrix(uint32_t object_index, real_matrix4x3 *out) // blam-cc: EAX -> object_index, EDI -> out
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;

    matrix4x3_from_forward_up(&obj->up, &obj->forward, out);
    out->position = obj->position;

    if (obj->parent_object != k_datum_index_none) {
        object *parent = ((object_header *)object_data->data)[obj->parent_object & 0xffff].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
            (int8_t)obj->parent_marker_index * 0x34);
        matrix4x3_multiply_procedure(parent_node, out, out);
    }

    return out;
}

#if 0
Original Ghidra decompilation (0x4f6a20):

void object_get_world_matrix(void)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  int unaff_EDI;

  iVar2 = DAT_008603b0;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  FUN_004cb970();
  *(undefined4 *)(unaff_EDI + 0x28) = *(undefined4 *)(iVar1 + 0x5c);
  *(undefined4 *)(unaff_EDI + 0x2c) = *(undefined4 *)(iVar1 + 0x60);
  *(undefined4 *)(unaff_EDI + 0x30) = *(undefined4 *)(iVar1 + 100);
  if (*(uint *)(iVar1 + 0x11c) != 0xffffffff) {
    iVar2 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc);
    (*(code *)PTR_matrix4x3_multiply_00696664)
              ((int)*(short *)(iVar2 + 0x1f2) + *(char *)(iVar1 + 0x120) * 0x34 + iVar2);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
