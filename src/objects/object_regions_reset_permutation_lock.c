// object_regions_reset_permutation_lock
// address 0x4f03e0, size 132 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md names this function directly:
// "object_regions_reset_permutation_lock 0x4f03e0 forces the same bytes to 0 or 1")
// rewrite confidence: 0.6
// evidence: types/objects.h object.region_permutations (0x180); types/tags.h
// ModelCollisionGeometry.regions (TagReflexive at 0x240), ModelCollisionGeometryRegion.flags
// (0x20, bit 0x10 == disappears_when_shield_is_off) and .permutations (TagReflexive at 0x48).
// register convention: uint32_t object_index in EAX (in_EAX); a bool lock flag in the low byte
// of EBX (unaff_BL).
// blam-cc: EAX=object_index, BL=unlock

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_objects.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

void object_regions_reset_permutation_lock(uint32_t object_index, int8_t unlock)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    ModelCollisionGeometry *geometry =
        (ModelCollisionGeometry *)tag_instances[definition->collision_model.tag_id.index].data;
    ModelCollisionGeometryRegion *regions = (ModelCollisionGeometryRegion *)geometry->regions.pointer;
    int32_t region_count = (int32_t)geometry->regions.count;
    int32_t region_index;

    for (region_index = 0; region_index < region_count; region_index++) {
        ModelCollisionGeometryRegion *region = &regions[region_index];

        if ((region->flags & 0x10) != 0 && (int32_t)region->permutations.count > 1) {
            obj->region_permutations[region_index] = (unlock == 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f03e0):

void FUN_004f03e0(void)

{
  uint *puVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  int iVar4;
  char unaff_BL;
  short sVar5;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar2 = *(int *)((*(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x7c) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar5 = 0;
  if (0 < *(int *)(iVar2 + 0x240)) {
    iVar4 = 0;
    do {
      iVar3 = iVar4 * 0x54 + *(int *)(iVar2 + 0x244);
      if (((*(byte *)(iVar3 + 0x20) & 0x10) != 0) && (1 < *(int *)(iVar3 + 0x48))) {
        *(bool *)(iVar4 + 0x180 + (int)puVar1) = unaff_BL == '\0';
      }
      sVar5 = sVar5 + 1;
      iVar4 = (int)sVar5;
    } while (iVar4 < *(int *)(iVar2 + 0x240));
  }
  return;
}
#endif
