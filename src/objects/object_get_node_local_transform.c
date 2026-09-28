// object_get_node_local_transform
// address 0x4f6080, size 252 bytes
// name confidence: 0.85 (types/objects.h names and cites this exact address as
//   "object_get_node_local_transform" in both the object_marker struct comment and the
//   object.region_permutations field comment)
// rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x4f6080..0x4f617b (6 stack args to 0x4d7850 + ECX model / EAX name; identity fallback; mirrored flag 0x1000).)
// evidence: types/objects.h object_marker (node_index 0x00, transform real_matrix4x3 0x04,
//   node_transform real_matrix4x3 0x38); object (region_permutations 0x180, flags 0x10 with
//   _object_mirrored_geometry_bit, nodes.offset 0x1f2); types/math.h real_matrix4x3
//   (scale/forward/left/up/position); callee model_markers_get_by_name 0x4d7850.
// register convention (corrected, see FIXED below): all four arguments on the stack -- object index,
//   marker name, destination object_marker*, and maximum_markers, forwarded unchanged to
//   model_markers_get_by_name.
// UNSURE: when re-deriving the identity-fallback field offsets by hand from the decompiled
//   pointer arithmetic (param_3 is a short* here, so "param_3+N" is a byte offset of 2*N), the
//   negated-when-mirrored triple lands on node_transform.left (marker+0x48/0x4c/0x50), not on
//   transform's own fields; this differs from the phrasing in types/objects.h's object_marker
//   comment ("the axis components at 0x24, 0x28, 0x2c are negated"), which was not re-derived
//   for this specific rewrite. The arithmetic below is traced directly from this function's own
//   decompilation. UNSURE: the bitmask `(obj->flags >> 0xc) & 0xffffff01` passed to
//   model_markers_get_by_name is preserved as literal arithmetic, not decoded further.
// reconciled: R40 objects.h object_marker comment now agrees with this file: mirrored mode negates node_transform.left (+0x48/+0x4c/+0x50; fchs at 0x4f6152..0x4f6162), no code change

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations,
                                         int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored,
                                         object_marker *out, int16_t maximum); // 0x4d7850; ECX model_tag_id, EAX name

// FIXED (objdump 0x4f6080): all four arguments are on the stack in declaration order ([esp+4] object index,
//   [esp+0x10] from entry the flag); the notes named EAX/ECX/EDX, which the original never reads.
int32_t object_get_node_local_transform(uint32_t object_index, char *marker_name, object_marker *marker,
                                         uint32_t maximum_markers)
    // blam-cc: stack -> object_index, marker_name, marker, maximum_markers
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    void *node_array = (uint8_t *)obj + obj->nodes.offset;
    // 0x4f60b2..0x4f60e4: ECX = the object definition's model tag (+0x34), EAX = the marker name; on the stack
    // the region permutations (+0x180), no node remap, the node matrices, the mirrored bit (flags bit 12),
    // the output marker and maximum_markers as the maximum
    int32_t result = model_markers_get_by_name(
        *(datum_index *)((uint8_t *)tag_instances[obj->definition_tag & 0xffff].data + 0x34), marker_name,
        (uint8_t *)obj + 0x180, (int16_t *)0, (real_matrix4x3 *)node_array, (uint8_t)((obj->flags >> 0xc) & 1),
        marker, (int16_t)maximum_markers);

    if ((int16_t)result == 0) {
        marker->node_index = 0;
        marker->transform.scale = 1.0f;
        marker->transform.forward.i = 1.0f;
        marker->transform.forward.j = 0.0f;
        marker->transform.forward.k = 0.0f;
        marker->transform.left.i = 0.0f;
        marker->transform.left.j = 1.0f;
        marker->transform.left.k = 0.0f;
        marker->transform.up.i = 0.0f;
        marker->transform.up.j = 0.0f;
        marker->transform.up.k = 1.0f;
        marker->transform.position.x = 0.0f;
        marker->transform.position.y = 0.0f;
        marker->transform.position.z = 0.0f;

        obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
        marker->node_transform = *(real_matrix4x3 *)((uint8_t *)obj + obj->nodes.offset);

        if ((obj->flags & _object_mirrored_geometry_bit) != 0) {
            marker->node_transform.left.i = -marker->node_transform.left.i;
            marker->node_transform.left.j = -marker->node_transform.left.j;
            marker->node_transform.left.k = -marker->node_transform.left.k;
        }

        if (marker_name != 0 && *marker_name == '\0') {
            result = 1;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4f6080):

undefined4 FUN_004f6080(uint param_1,char *param_2,undefined2 *param_3,undefined4 param_4)

{
  int iVar1;
  undefined4 uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;

  iVar4 = (param_1 & 0xffff) * 0xc;
  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar4);
  uVar2 = model_markers_get_by_name
                    (iVar1 + 0x180,0,*(short *)(iVar1 + 0x1f2) + iVar1,
                     *(uint *)(iVar1 + 0x10) >> 0xc & 0xffffff01,param_3,param_4);
  if ((short)uVar2 == 0) {
    *param_3 = 0;
    *(undefined4 *)(param_3 + 2) = 0x3f800000;
    *(undefined4 *)(param_3 + 4) = 0x3f800000;
    *(undefined4 *)(param_3 + 0xc) = 0x3f800000;
    *(undefined4 *)(param_3 + 0x14) = 0x3f800000;
    iVar3 = DAT_008603b0;
    *(undefined4 *)(param_3 + 6) = 0;
    *(undefined4 *)(param_3 + 8) = 0;
    *(undefined4 *)(param_3 + 10) = 0;
    *(undefined4 *)(param_3 + 0xe) = 0;
    *(undefined4 *)(param_3 + 0x10) = 0;
    *(undefined4 *)(param_3 + 0x12) = 0;
    *(undefined4 *)(param_3 + 0x16) = 0;
    *(undefined4 *)(param_3 + 0x18) = 0;
    *(undefined4 *)(param_3 + 0x1a) = 0;
    iVar4 = *(int *)(*(int *)(iVar3 + 0x34) + 8 + iVar4);
    puVar5 = (undefined4 *)(*(short *)(iVar4 + 0x1f2) + iVar4);
    puVar6 = (undefined4 *)(param_3 + 0x1c);
    for (iVar3 = 0xd; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar6 = *puVar5;
      puVar5 = puVar5 + 1;
      puVar6 = puVar6 + 1;
    }
    if ((*(uint *)(iVar1 + 0x10) & 0x1000) != 0) {
      *(float *)(param_3 + 0x24) = -*(float *)(param_3 + 0x24);
      *(float *)(param_3 + 0x26) = -*(float *)(param_3 + 0x26);
      *(float *)(param_3 + 0x28) = -*(float *)(param_3 + 0x28);
    }
    if ((param_2 != (char *)0x0) && (*param_2 == '\0')) {
      uVar2 = 1;
    }
  }
  return uVar2;
}
#endif
