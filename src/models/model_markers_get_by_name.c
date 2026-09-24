// model_markers_get_by_name  (Ghidra: model_markers_get_by_name, already named)
// address 0x4d7850, size 302 bytes
// name confidence: 0.5   rewrite confidence: 0.8 (review pass: checked against objdump)
// evidence: out/phase4/models_types_notes.md object_marker section, which also corrects
//   objects.h's own comment: "in mirrored mode 0x4d7850 negates +0x48, +0x4c and +0x50. That
//   is node_transform.left at 0x38 + 0x10 ... On the models path, +0x04 is the node-relative
//   marker matrix and +0x38 is the world marker matrix" (not an identity/copy, as objects.h's
//   own comment for the unrelated fallback path says). ModelMarkerInstance field offsets
//   (region/permutation/node index bytes, translation, rotation) and ModelMarker.instances
//   +0x34 match types/tags.h. The model_marker_group_index_from_name() and
//   matrix4x3_from_quaternion() calls drop their arguments in Ghidra's decompilation; the
//   first is a straightforward EAX/ECX-to-EAX/stack passthrough of this function's own
//   parameters (matching that function's established convention), the second is inferred from
//   the fields patched immediately after it (translation into the fresh matrix's position),
//   the same SQT-to-matrix pattern used throughout this module.
// register convention: model tag id in ECX (in_ECX), name in EAX (in_EAX, passed through to
//   model_marker_group_index_from_name); region permutations, node remap table, node matrices,
//   mirrored flag, output array and its maximum count as the recognized stack parameters, in
//   that order.
//   // blam-cc: ECX -> model_tag_id, EAX -> name, stack -> region_permutations, node_remap,
//   //           node_matrices, mirrored, out, maximum

#include "tags.h"
#include "math.h"
#include "cache.h"
#include "models.h"
#include "objects.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern void (*matrix4x3_multiply_procedure)(real_matrix4x3 *a, real_matrix4x3 *b, real_matrix4x3 *out); // 0x00696664
extern void matrix4x3_from_quaternion(real_quaternion *q, real_matrix4x3 *out); // 0x4cbad0
extern int16_t model_marker_group_index_from_name(datum_index model_tag_id, const char *name); // 0x4d77c0, this batch

// Resolves a marker group by name and fills object_marker records for every instance in it
// (optionally filtered by region_permutations, one entry per region: NULL means take every
// instance) up to `maximum` entries, remapping each instance's node index through node_remap
// if given, and negating the world matrix's left axis when mirrored. Returns the number of
// entries written, or 0 if the group name does not resolve or has no instances.
int16_t model_markers_get_by_name(datum_index model_tag_id, const char *name, uint8_t *region_permutations,
                                   int16_t *node_remap, real_matrix4x3 *node_matrices, uint8_t mirrored,
                                   object_marker *out, int16_t maximum)
{
    int16_t group_index;
    GBXModel *model;
    ModelMarker *marker;
    ModelMarkerInstance *instances;
    int16_t result_count;
    int16_t i;

    group_index = model_marker_group_index_from_name(model_tag_id, name);
    if (group_index == -1) {
        return 0;
    }

    model = (GBXModel *)tag_instances[model_tag_id & 0xffff].data;
    marker = &((ModelMarker *)model->markers.pointer)[group_index];
    if (marker->instances.count <= 0) {
        return 0;
    }
    instances = (ModelMarkerInstance *)marker->instances.pointer;

    result_count = 0;
    for (i = 0; (int32_t)i < marker->instances.count; i++) { // movsx i, cmp against the int32 count
        ModelMarkerInstance *instance = &instances[i];

        if (region_permutations != 0 && region_permutations[instance->region_index] != instance->permutation_index) {
            continue;
        }
        if (maximum <= result_count) {
            return result_count;
        }

        {
            object_marker *entry = &out[result_count];
            int16_t node_index;

            result_count = result_count + 1;
            node_index = (node_remap == 0) ? (int16_t)instance->node_index : node_remap[instance->node_index];
            entry->node_index = node_index;

            matrix4x3_from_quaternion((real_quaternion *)&instance->rotation, &entry->transform);
            entry->transform.position = *(real_point3d *)&instance->translation;

            matrix4x3_multiply_procedure(&node_matrices[node_index], &entry->transform, &entry->node_transform);

            if (mirrored != 0) {
                entry->node_transform.left.i = -entry->node_transform.left.i;
                entry->node_transform.left.j = -entry->node_transform.left.j;
                entry->node_transform.left.k = -entry->node_transform.left.k;
            }
        }
    }
    return result_count;
}

#if 0
Original Ghidra decompilation (0x4d7850):

short model_markers_get_by_name
                (int param_1,int param_2,int param_3,char param_4,int param_5,short param_6)

{
  short sVar1;
  ushort uVar2;
  short sVar3;
  int iVar4;
  byte *pbVar5;
  uint in_ECX;
  int extraout_EDX;
  short sVar6;
  int iVar7;
  ushort *puVar8;

  sVar6 = 0;
  sVar1 = model_marker_group_index_from_name();
  if (sVar1 == -1) {
    return 0;
  }
  iVar4 = *(int *)(*(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0xb0);
  iVar7 = sVar1 * 0x40 + iVar4;
  sVar3 = 0;
  if (0 < *(int *)(sVar1 * 0x40 + 0x34 + iVar4)) {
    iVar4 = 0;
    do {
      pbVar5 = (byte *)(iVar4 * 0x20 + *(int *)(iVar7 + 0x38));
      if ((param_1 == 0) || (*(byte *)((uint)*pbVar5 + param_1) == pbVar5[1])) {
        if (param_6 <= sVar6) {
          return sVar6;
        }
        puVar8 = (ushort *)(sVar6 * 0x6c + param_5);
        sVar6 = sVar6 + 1;
        if (param_2 == 0) {
          uVar2 = (ushort)pbVar5[2];
        }
        else {
          uVar2 = *(ushort *)(param_2 + (uint)pbVar5[2] * 2);
        }
        *puVar8 = uVar2;
        matrix4x3_from_quaternion();
        *(undefined4 *)(extraout_EDX + 0x28) = *(undefined4 *)(pbVar5 + 4);
        *(undefined4 *)(extraout_EDX + 0x2c) = *(undefined4 *)(pbVar5 + 8);
        *(undefined4 *)(extraout_EDX + 0x30) = *(undefined4 *)(pbVar5 + 0xc);
        (*(code *)PTR_matrix4x3_multiply_00696664)
                  ((short)*puVar8 * 0x34 + param_3,extraout_EDX,puVar8 + 0x1c);
        if (param_4 != '\0') {
          *(float *)(puVar8 + 0x24) = -*(float *)(puVar8 + 0x24);
          *(float *)(puVar8 + 0x26) = -*(float *)(puVar8 + 0x26);
          *(float *)(puVar8 + 0x28) = -*(float *)(puVar8 + 0x28);
        }
      }
      sVar3 = sVar3 + 1;
      iVar4 = (int)sVar3;
    } while (iVar4 < *(int *)(iVar7 + 0x34));
    return sVar6;
  }
  return 0;
}
#endif
