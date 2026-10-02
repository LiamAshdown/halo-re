// unit_get_forward_vector_or_marker_normal  (Ghidra: FUN_00569720)
// address 0x569720, size 113 bytes, name confidence 0.35, rewrite confidence 0.95
// functions.md: "Returns a unit's stored direction/offset vector, transformed into world space
// through its parent object's skeleton node when attached."
// evidence: types/objects.h object.forward (0x74), object.parent_object (0x11c),
//   object.parent_marker_index (0x120), object.nodes (block_reference at 0x1f0, .offset field).
// blam-cc: in_ECX -> unit_index, in_EAX -> out (may be NULL).
// UNSURE: matrix4x3_transform_normal's exact argument shape (what it reads from EAX/ECX besides
//   the node-matrix pointer this rewrite passes) is not confirmed; the input direction it
//   transforms is presumably the unattached case's own object.forward, by symmetry with the
//   `if` branch, but that is not proven from this decompilation alone.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void matrix4x3_transform_normal(real_vector3d *out, real_vector3d *normal, real_matrix4x3 *m); // 0x4cbec0, EAX, EDX, stack

void unit_get_forward_vector_or_marker_normal(uint32_t unit_index, real_vector3d *out) // blam-cc: in_ECX, in_EAX
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;

    if (unit_obj->parent_object == k_datum_index_none) {
        if (out != (real_vector3d *)0) {
            *out = unit_obj->forward;
        }
        return;
    }

    object *parent = ((object_header *)object_data->data)[unit_obj->parent_object & 0xffff].data;
    if (out != (real_vector3d *)0) {
        real_matrix4x3 *node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset) + unit_obj->parent_marker_index;
        // 0x569783: EAX = out, EDX = the unit's forward (+0x74), stack = the parent's marker node matrix
        matrix4x3_transform_normal(out, &unit_obj->forward, node);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x569720):

void FUN_00569720(void)

{
  int iVar1;
  int iVar2;
  undefined4 *in_EAX;
  uint in_ECX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_ECX & 0xffff) * 0xc);
  if (*(uint *)(iVar1 + 0x11c) == 0xffffffff) {
    if (in_EAX != (undefined4 *)0x0) {
      *in_EAX = *(undefined4 *)(iVar1 + 0x74);
      in_EAX[1] = *(undefined4 *)(iVar1 + 0x78);
      in_EAX[2] = *(undefined4 *)(iVar1 + 0x7c);
      return;
    }
  }
  else {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar1 + 0x11c) & 0xffff) * 0xc);
    if (in_EAX != (undefined4 *)0x0) {
      matrix4x3_transform_normal
                ((int)*(short *)(iVar2 + 0x1f2) + *(char *)(iVar1 + 0x120) * 0x34 + iVar2);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
