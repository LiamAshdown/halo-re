// object_snap_to_parent_marker_and_detach  (Ghidra: FUN_004f6610; renamed from
// out/phase4/objects_types_notes.md, which cites this address by this exact name: "object
// 0x114, 0x118, 0x11c, 0x120 | object_attach_to_object 0x4f6440 (...),
// object_snap_to_parent_marker_and_detach 0x4f6610 (clears 0x11c to -1 and 0x120 to 0xff)")
// address 0x4f6610, size 453 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.65
// evidence: types/objects.h object_header (flags at 0x02, active bit); object (position 0x05c,
//   forward 0x074, up 0x080, velocity 0x068, angular_velocity 0x08c, parent_object 0x11c,
//   parent_marker_index 0x120, flags 0x10 with _object_do_not_delete_bit, nodes.offset 0x1f2);
//   types/math.h real_matrix4x3; global 0x008603b0 object_data; global 0x00696664
//   matrix4x3_multiply_procedure; callees object_unlink_cluster_or_notify_parent (0x4f5de0),
//   matrix4x3_from_forward_up (0x4cb970), matrix4x3_multiply (0x4cc0d0), object_set_cluster_and_parent
//   (0x4f5c30). Cross-checked against object_get_position/_get_orientation/_get_world_matrix
//   (0x4f6900/0x4f6970/0x4f6a20, this batch), which use the identical
//   "parent + parent->nodes.offset + parent_marker_index*0x34" node-pointer arithmetic and the
//   same EAX=out/EDX=in calling convention for matrix4x3_transform_point/_normal, confirming
//   that an attached object's own position/forward/up fields are stored in the PARENT MARKER'S
//   local space, not world space.
// register convention: the object index is a plain STACK argument (0x4f6619 mov edx,[ebp+0x8]),
//   not a register parameter, despite Ghidra's "void FUN_004f6610(uint param_1)" -- confirmed
//   against objdump -d -M intel bin/halo.exe at 0x4f6610.
// resolved from disassembly (objdump -d -M intel bin/halo.exe, 0x4f6610..0x4f67d4):
//   - The child's own position/forward/up (object+0x5c/0x74/0x80) are, while attached, expressed
//     in the coordinate space of the parent's marker node. This function computes their WORLD
//     values one last time before detaching, by composing two matrix4x3_multiply calls:
//     local_7c = parent_node * {identity rotation, position = child's local position}, which
//     yields {rotation = parent_node's world rotation, position = child's world position};
//     then local_7c = local_7c * {rotation built from the child's own forward/up, position = 0},
//     which yields {rotation = parent_node's world rotation composed with the child's own local
//     orientation, position unchanged}. The final forward/up/position are the object's world
//     values and are written back into the same fields.
//   - The object's velocity and angular_velocity are copied from the PARENT's velocity and
//     angular_velocity (captured before object_unlink_cluster_or_notify_parent runs), so a
//     detached object keeps the motion of whatever it was riding/attached to.
//   - The final if-chain reactivates the object_header active bit when the object has no
//     do-not-delete flag, is genuinely unparented, and was not already active -- the same
//     condition object_mark_pending_delete's callers use elsewhere in this module.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "fn_math.h"
#include "fn_objects.h"

extern data_array *object_data; // 0x008603b0
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664

extern void object_unlink_cluster_or_notify_parent(uint32_t object_index); // 0x4f5de0, this batch, blam-cc: EAX -> object_index


void object_snap_to_parent_marker_and_detach(uint32_t object_index) // blam-cc: stack -> object_index
{
    object_header *headers = (object_header *)object_data->data;
    object *child = headers[object_index & 0xffff].data;
    object *old_parent = headers[child->parent_object & 0xffff].data;

    object_unlink_cluster_or_notify_parent(object_index);

    // object_data->data is re-read here, matching the compiled code; the object pool itself
    // never moves, so this is the same pointer, just reloaded defensively after the call.
    {
        object *parent_node_owner = ((object_header *)object_data->data)[child->parent_object & 0xffff].data;
        real_matrix4x3 *parent_node = (real_matrix4x3 *)((uint8_t *)parent_node_owner +
            parent_node_owner->nodes.offset + (int8_t)child->parent_marker_index * 0x34);

        real_matrix4x3 own_rotation;   // local_b4: {scale 1, child's own forward/left/up, position 0}
        real_matrix4x3 local_transform; // the synthetic matrix built from child's OLD local position
        real_matrix4x3 world;           // local_7c, the composed result

        matrix4x3_from_forward_up(&child->up, &child->forward, &own_rotation);

        local_transform.scale = 1.0f;
        local_transform.forward.i = 1.0f; local_transform.forward.j = 0.0f; local_transform.forward.k = 0.0f;
        local_transform.left.i = 0.0f;    local_transform.left.j = 1.0f;    local_transform.left.k = 0.0f;
        local_transform.up.i = 0.0f;      local_transform.up.j = 0.0f;      local_transform.up.k = 1.0f;
        local_transform.position = child->position;

        matrix4x3_multiply_procedure(parent_node, &local_transform, &world);
        matrix4x3_multiply_procedure(&world, &own_rotation, &world);

        child->forward = world.forward;
        child->up = world.up;
        child->position = world.position;
    }

    child->velocity = old_parent->velocity;
    child->angular_velocity = old_parent->angular_velocity;

    child->parent_marker_index = 0xff;
    child->parent_object = k_datum_index_none;

    object_set_cluster_and_parent(object_index, 0);

    {
        object_header *header = (object_header *)object_data->data + (object_index & 0xffff);
        object *obj = header->data;
        if (((header->flags & _object_header_active_bit) == 0) &&
            ((obj->flags & _object_do_not_delete_bit) == 0) &&
            (obj->parent_object == k_datum_index_none)) {
            header->flags |= _object_header_active_bit;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f6610):

void FUN_004f6610(uint param_1)

{
  char cVar1;
  byte bVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined1 local_b4 [56];
  undefined1 local_7c [4];
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  int local_8;

  iVar5 = DAT_008603b0;
  iVar6 = (param_1 & 0xffff) * 0xc;
  iVar4 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar6);
  local_8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar4 + 0x11c) & 0xffff) * 0xc);
  FUN_004f5de0();
  iVar5 = *(int *)(*(int *)(iVar5 + 0x34) + 8 + (*(uint *)(iVar4 + 0x11c) & 0xffff) * 0xc);
  sVar3 = *(short *)(iVar5 + 0x1f2);
  cVar1 = *(char *)(iVar4 + 0x120);
  local_1c = *(undefined4 *)(iVar4 + 0x5c);
  local_18 = *(undefined4 *)(iVar4 + 0x60);
  local_14 = *(undefined4 *)(iVar4 + 100);
  local_44 = 0x3f800000;
  local_40 = 0x3f800000;
  local_3c = 0;
  local_38 = 0;
  local_34 = 0;
  local_30 = 0x3f800000;
  local_2c = 0;
  local_28 = 0;
  local_24 = 0;
  local_20 = 0x3f800000;
  FUN_004cb970(local_b4);
  (*(code *)PTR_matrix4x3_multiply_00696664)(cVar1 * 0x34 + sVar3 + iVar5,&local_44,local_7c);
  (*(code *)PTR_matrix4x3_multiply_00696664)(local_7c,local_b4,local_7c);
  *(undefined4 *)(iVar4 + 0x74) = local_78;
  *(undefined4 *)(iVar4 + 0x78) = local_74;
  *(undefined4 *)(iVar4 + 0x7c) = local_70;
  *(undefined4 *)(iVar4 + 0x80) = local_60;
  *(undefined4 *)(iVar4 + 0x84) = local_5c;
  *(undefined4 *)(iVar4 + 0x88) = local_58;
  *(undefined4 *)(iVar4 + 0x5c) = local_54;
  *(undefined4 *)(iVar4 + 0x60) = local_50;
  *(undefined4 *)(iVar4 + 100) = local_4c;
  *(undefined4 *)(iVar4 + 0x68) = *(undefined4 *)(local_8 + 0x68);
  *(undefined4 *)(iVar4 + 0x6c) = *(undefined4 *)(local_8 + 0x6c);
  *(undefined4 *)(iVar4 + 0x70) = *(undefined4 *)(local_8 + 0x70);
  *(undefined4 *)(iVar4 + 0x8c) = *(undefined4 *)(local_8 + 0x8c);
  *(undefined4 *)(iVar4 + 0x90) = *(undefined4 *)(local_8 + 0x90);
  *(undefined4 *)(iVar4 + 0x94) = *(undefined4 *)(local_8 + 0x94);
  *(undefined1 *)(iVar4 + 0x120) = 0xff;
  *(undefined4 *)(iVar4 + 0x11c) = 0xffffffff;
  FUN_004f5c30(param_1,0);
  iVar4 = *(int *)(iVar6 + 8 + *(int *)(DAT_008603b0 + 0x34));
  iVar6 = iVar6 + *(int *)(DAT_008603b0 + 0x34);
  bVar2 = *(byte *)(iVar6 + 2);
  if ((((bVar2 & 1) == 0) && ((*(uint *)(iVar4 + 0x10) & 0x100000) == 0)) &&
     (*(int *)(iVar4 + 0x11c) == -1)) {
    *(byte *)(iVar6 + 2) = bVar2 | 1;
  }
  return;
}
#endif
