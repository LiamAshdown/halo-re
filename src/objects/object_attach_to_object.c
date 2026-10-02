// object_attach_to_object
// address 0x4f6440, size 451 bytes
// name confidence: 0.85 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Attaches object param_2 to parent object param_1 at marker
//   param_3, refusing the operation if it would create an attachment cycle")
// rewrite confidence: 0.8
// evidence: types/objects.h object_header (flags at 0x02), object (position 0x05c, forward
//   0x074, up 0x080, parent_object 0x11c, parent_marker_index 0x120, flags 0x10 with
//   _object_needs_cluster_update_bit, nodes.offset 0x1f2); types/math.h real_matrix4x3;
//   global 0x008603b0 object_data; callees object_unlink_cluster_or_notify_parent (0x4f5de0),
//   object_set_cluster_and_parent (0x4f5c30), object_recalculate_bounding_radius (0x4f8310).
// register convention: ALL THREE arguments are stack arguments -- 0x4f6446 mov edx,[ebp+0x8],
//   0x4f6460 mov ecx,[ebp+0xc], 0x4f64c7 movsx ecx,WORD PTR [ebp+0x10]. An earlier draft
//   claimed EAX/ECX/EDX. The marker index is read with movsx from a WORD, so it is a SIGNED
//   16-bit value, not the uint8_t the earlier draft used.
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4f6440..):
//   - 0x4f64cb movsx edx,WORD PTR [eax+0x1f2] / imul ecx,ecx,0x34 / lea eax,[ebp-0x40] /
//     call 0x4cb7a0: matrix4x3_inverse(&inverse /*EAX*/, &parent->nodes[marker_index] /*ECX*/).
//   - 0x4f64e1 lea eax,[esi+0x5c] / lea ecx,[ebp-0x40] / push ecx / mov edx,eax / call 0x4cbde0:
//     matrix4x3_transform_point transforms the CHILD'S OWN position in place (EAX and EDX are
//     the same pointer) by that inverse, i.e. it rewrites the child placement into the parent
//     marker's node space.
//   - The nine floats the earlier draft kept as an anonymous frame are the rotation rows of that
//     same inverse matrix; the algebra it produced was correct, only unnamed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch
extern void matrix4x3_inverse(real_matrix4x3 *out, real_matrix4x3 *in); // 0x4cb7a0, out in EAX, in in ECX
extern void matrix4x3_transform_point(real_point3d *out, real_point3d *in, real_matrix4x3 *m);
    // 0x4cbde0; out in EAX, in in EDX, matrix on the stack
extern void object_set_cluster_and_parent(uint32_t object_index, bsp_leaf_reference *location); // 0x4f5c30, this batch
extern void object_recalculate_bounding_radius(uint32_t object_index); // 0x4f8310

void object_attach_to_object(uint32_t parent_index, uint32_t child_index, int16_t marker_index)
    // blam-cc: stack -> parent_index, child_index, marker_index
{
    uint32_t ancestor = parent_index;

    while (ancestor != k_datum_index_none) {
        object *ancestor_obj;
        if (ancestor == child_index) {
            return; // attaching would create a cycle
        }
        ancestor_obj = ((object_header *)object_data->data)[ancestor & 0xffff].data;
        ancestor = ancestor_obj->parent_object;
    }

    {
        object_header *child_header = (object_header *)object_data->data + (child_index & 0xffff);
        object *child = child_header->data;
        int needs_cluster_update = (child->flags >> 0xb) & 1;
        object *parent;
        real_matrix4x3 *parent_node;
        real_matrix4x3 inverse;   // [ebp-0x40], Ghidra local_44
        real_vector3d v;

        if (needs_cluster_update) {
            object_unlink_cluster_or_notify_parent(child_index);
        }

        parent = ((object_header *)object_data->data)[parent_index & 0xffff].data;
        parent_node = (real_matrix4x3 *)((uint8_t *)parent + parent->nodes.offset +
                                         marker_index * 0x34);

        matrix4x3_inverse(&inverse, parent_node);
        matrix4x3_transform_point(&child->position, &child->position, &inverse);

        v = child->forward;
        child->forward.i = inverse.forward.i * v.i + inverse.left.i * v.j + inverse.up.i * v.k;
        child->forward.j = inverse.forward.j * v.i + inverse.left.j * v.j + inverse.up.j * v.k;
        child->forward.k = inverse.forward.k * v.i + inverse.left.k * v.j + inverse.up.k * v.k;

        v = child->up;
        child->up.i = inverse.forward.i * v.i + inverse.left.i * v.j + inverse.up.i * v.k;
        child->up.j = inverse.forward.j * v.i + inverse.left.j * v.j + inverse.up.j * v.k;
        child->up.k = inverse.forward.k * v.i + inverse.left.k * v.j + inverse.up.k * v.k;

        child->parent_object = parent_index;
        child->parent_marker_index = (uint8_t)marker_index;

        if (needs_cluster_update) {
            object_set_cluster_and_parent(child_index, 0);
            child_header = (object_header *)object_data->data + (child_index & 0xffff);
        }

        if ((child_header->flags & _object_header_active_bit) != 0) {
            child_header->flags &= (uint8_t)~_object_header_active_bit;
        }
        child_header->flags |= _object_header_just_created_bit;

        object_recalculate_bounding_radius(child_index);
    }
}

#if 0
Original Ghidra decompilation (0x4f6440):

void object_attach_to_object(uint param_1,uint param_2,undefined1 param_3)

{
  byte *pbVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  byte bVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  undefined1 local_44 [4];
  float local_40;
  float local_3c;
  float local_38;
  float local_34;
  float local_30;
  float local_2c;
  float local_28;
  float local_24;
  float local_20;
  int local_8;

  iVar8 = DAT_008603b0;
  uVar7 = param_1;
  while( true ) {
    if (uVar7 == 0xffffffff) {
      local_8 = (param_2 & 0xffff) * 0xc;
      iVar6 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + local_8);
      uVar7 = *(uint *)(iVar6 + 0x10) >> 0xb;
      if ((uVar7 & 1) != 0) {
        FUN_004f5de0();
      }
      matrix4x3_inverse();
      matrix4x3_transform_point(local_44);
      fVar2 = *(float *)(iVar6 + 0x74);
      fVar3 = *(float *)(iVar6 + 0x78);
      fVar4 = *(float *)(iVar6 + 0x7c);
      *(float *)(iVar6 + 0x74) = local_40 * fVar2 + local_34 * fVar3 + local_28 * fVar4;
      *(float *)(iVar6 + 0x78) = local_3c * fVar2 + local_30 * fVar3 + local_24 * fVar4;
      *(float *)(iVar6 + 0x7c) = local_38 * fVar2 + local_2c * fVar3 + local_20 * fVar4;
      fVar2 = *(float *)(iVar6 + 0x80);
      fVar3 = *(float *)(iVar6 + 0x84);
      fVar4 = *(float *)(iVar6 + 0x88);
      *(float *)(iVar6 + 0x80) = local_40 * fVar2 + local_34 * fVar3 + local_28 * fVar4;
      *(float *)(iVar6 + 0x84) = local_3c * fVar2 + local_30 * fVar3 + local_24 * fVar4;
      *(float *)(iVar6 + 0x88) = local_38 * fVar2 + local_2c * fVar3 + local_20 * fVar4;
      *(uint *)(iVar6 + 0x11c) = param_1;
      *(undefined1 *)(iVar6 + 0x120) = param_3;
      if ((uVar7 & 1) != 0) {
        FUN_004f5c30(param_2,0);
        iVar8 = DAT_008603b0;
      }
      bVar5 = *(byte *)(*(int *)(iVar8 + 0x34) + 2 + local_8);
      if ((bVar5 & 1) != 0) {
        *(byte *)(*(int *)(iVar8 + 0x34) + local_8 + 2) = bVar5 & 0xfe;
      }
      pbVar1 = (byte *)(*(int *)(iVar8 + 0x34) + local_8 + 2);
      *pbVar1 = *pbVar1 | 0x10;
      object_recalculate_bounding_radius(param_2);
      return;
    }
    if (uVar7 == param_2) break;
    uVar7 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar7 & 0xffff) * 0xc) + 0x11c);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
