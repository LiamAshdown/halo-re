// object_reorient_relative_to_marker
// address 0x4f6180, size 358 bytes
// name confidence: 0.55 (still FUN_004f6180 in Ghidra; functions.md's summary matches the shape
//   of the code: "Recomputes an object's world orientation relative to a marker and then
//   reattaches/updates it via the attach helper")
// rewrite confidence: 0.75
// evidence: types/objects.h object (position 0x05c, forward 0x074, up 0x080) and object_marker
//   (node_index 0x00, transform 0x04, node_transform 0x38); types/math.h real_matrix4x3;
//   callees object_get_node_local_transform 0x4f6080, object_unlink_cluster_or_notify_parent
//   0x4f5de0, object_recompute_basis_from_marker_delta 0x4f62f0, object_set_cluster_and_parent
//   0x4f5c30, object_attach_to_object 0x4f6440.
// register convention: parent object index and parent marker name are the two stack arguments
//   ([ebp+0x8], [ebp+0xc]); the object being reoriented is in ESI and its own marker name in
//   EDI. Both registers are read before ever being written.
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4f6180..0x4f62e5). The previous
// draft modelled the stack as one anonymous 0x74-byte "frame" and reproduced the float algebra
// against the wrong offsets; the real layout is three overlapping named objects:
//   [ebp-0x118] .. [ebp-0xad]  object_marker for the object's own marker  (Ghidra local_11c)
//   [ebp-0x0a8] .. [ebp-0x3d]  object_marker for the parent's marker      (Ghidra local_ac)
//   [ebp-0x070] .. [ebp-0x3d]  IS parent_marker.node_transform -- the 0x34 bytes at offset
//                              0x38 inside the block above, which is why 0x4f61da passes
//                              [ebp-0x70] as object_recompute_basis_from_marker_delta's
//                              output matrix (Ghidra called it a separate local_74)
//   [ebp-0x038] .. [ebp-0x5]   the inverted transform (0x4f61f4 lea eax,[ebp-0x38] /
//                              lea ecx,[ebp-0x114] / call 0x4cb7a0 -- destination in EAX,
//                              source in ECX, and [ebp-0x114] is object_marker.transform)
// With that mapping the arithmetic is an ordinary basis rotation: the object's forward and up
// are the parent marker's node-space forward and up run through the inverted matrix
//   out = inv.forward * v.i + inv.left * v.j + inv.up * v.k
// and the position is the same matrix applied to parent_marker.node_transform.position
// (0x4f6205 lea eax,[ebx+0x5c] / lea edx,[ebp-0x48], where [ebp-0x48] is that position).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name,
    object_marker *marker, uint32_t flags); // 0x4f6080, this batch; all four on the stack
extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, out in EAX, in in ECX
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m);
    // 0x4cbde0; out in EAX, in in EDX, matrix on the stack
extern void object_recompute_basis_from_marker_delta(object *obj, object_marker *marker,
    real_matrix4x3 *output_matrix); // 0x4f62f0, this batch; obj in EAX
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch
extern void object_attach_to_object(uint32_t parent_index, uint32_t child_index, uint32_t marker_word);
    // 0x4f6440, this batch

void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
                                         uint32_t object_index, char *object_marker_name)
    // blam-cc: stack -> parent_index, parent_marker_name; ESI -> object_index,
    //          EDI -> object_marker_name
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    object_marker own_marker;     // [ebp-0x118]
    object_marker parent_marker;  // [ebp-0xa8]

    object_get_node_local_transform(parent_index, parent_marker_name, &parent_marker, 1);
    object_get_node_local_transform(object_index, object_marker_name, &own_marker, 1);
    object_unlink_cluster_or_notify_parent(object_index);

    if (object_marker_name != 0 && *object_marker_name != '\0') {
        object_recompute_basis_from_marker_delta(obj, &own_marker, &parent_marker.node_transform);
    } else {
        real_matrix4x3 inverse;   // [ebp-0x38]
        real_vector3d *forward = &parent_marker.node_transform.forward;
        real_vector3d *up = &parent_marker.node_transform.up;

        matrix4x3_inverse(&inverse, &own_marker.transform);
        matrix4x3_transform_point(&obj->position, &parent_marker.node_transform.position, &inverse);

        obj->forward.i = inverse.up.i * forward->k + inverse.forward.i * forward->i +
                         inverse.left.i * forward->j;
        obj->forward.j = inverse.up.j * forward->k + inverse.left.j * forward->j +
                         inverse.forward.j * forward->i;
        obj->forward.k = inverse.up.k * forward->k + inverse.left.k * forward->j +
                         inverse.forward.k * forward->i;

        obj->up.i = inverse.up.i * up->k + inverse.left.i * up->j + inverse.forward.i * up->i;
        obj->up.j = inverse.up.j * up->k + inverse.left.j * up->j + inverse.forward.j * up->i;
        obj->up.k = inverse.up.k * up->k + inverse.left.k * up->j + inverse.forward.k * up->i;
    }

    object_set_cluster_and_parent(object_index, 0);
    object_attach_to_object(parent_index, object_index,
                            *(uint32_t *)&parent_marker.node_index); // 0x4f62cd mov edx,[ebp-0xa8]
}

#if 0
Original Ghidra decompilation (0x4f6180):

void FUN_004f6180(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  uint unaff_ESI;
  char *unaff_EDI;
  undefined1 local_11c [112];
  undefined1 local_ac [56];
  undefined1 local_74 [4];
  float local_70;
  float local_6c;
  float local_68;
  float local_58;
  float local_54;
  float local_50;
  undefined1 local_3c [4];
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_ESI & 0xffff) * 0xc);
  FUN_004f6080(param_1,param_2,local_ac,1);
  FUN_004f6080();
  FUN_004f5de0();
  if ((unaff_EDI == (char *)0x0) || (*unaff_EDI == '\0')) {
    matrix4x3_inverse();
    matrix4x3_transform_point(local_3c);
    *(float *)(iVar1 + 0x74) = local_2c * local_6c + local_38 * local_70 + local_20 * local_68;
    *(float *)(iVar1 + 0x78) = local_34 * local_70 + local_28 * local_6c + local_1c * local_68;
    *(float *)(iVar1 + 0x7c) = local_30 * local_70 + local_24 * local_6c + local_18 * local_68;
    *(float *)(iVar1 + 0x80) = local_38 * local_58 + local_54 * local_2c + local_50 * local_20;
    *(float *)(iVar1 + 0x84) = local_34 * local_58 + local_28 * local_54 + local_1c * local_50;
    *(float *)(iVar1 + 0x88) = local_30 * local_58 + local_24 * local_54 + local_18 * local_50;
  }
  else {
    FUN_004f62f0(local_11c,local_74);
  }
  FUN_004f5c30();
  object_attach_to_object(param_1);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
