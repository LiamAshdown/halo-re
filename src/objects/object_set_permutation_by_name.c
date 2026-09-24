// object_set_permutation_by_name  (Ghidra: object_set_permutation_by_name, already named)
// address 0x4f6c60, size 240 bytes
// name confidence: 0.8 (already carries this name from an earlier phase; matches
//   functions.md's summary: "Sets an object's active region permutation by matching a
//   permutation name, optionally restricted to one region")
// rewrite confidence: 0.55
// evidence: types/objects.h object (definition_tag 0x000, region_permutations 0x180);
//   types/tags.h Object.model, GBXModel.regions, ModelRegion (name, permutations),
//   ModelRegionPermutation (name); global 0x008603b0 object_data, global 0x0087bc14
//   tag_instances; callee __stricmp (0x628d8b, libc).
// register convention: object index in EAX, name/region/flag are stack parameters in Ghidra's
//   own declared order. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f6c72 masks eax
//   as the index before any stack access, and 0x4f6cc2 mov ax,[esp+0x20] reads the region
//   filter from the stack.
//   // blam-cc: EAX -> object_index, stack -> name, region_filter, use_matched_index
// UNSURE: when a name match is found but use_matched_index is false, the stored permutation
//   index is forced to 0 rather than the matched index -- preserved exactly, meaning unclear.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int32_t __stricmp(const char *a, const char *b); // 0x628d8b, libc

void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
    char use_matched_index) // blam-cc: EAX -> object_index, stack -> name, region_filter, use_matched_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->model.tag_id.index == 0xffff) {
        return;
    }

    {
        GBXModel *model = (GBXModel *)tag_instances[definition->model.tag_id.index & 0xffff].data;
        int16_t region_index;
        for (region_index = 0; region_index < (int16_t)model->regions.count; region_index++) {
            ModelRegion *region;
            if (region_filter != -1 && region_filter != region_index) {
                continue;
            }
            region = (ModelRegion *)model->regions.pointer + region_index;
            if ((int16_t)region->permutations.count > 0) {
                ModelRegionPermutation *permutations = (ModelRegionPermutation *)region->permutations.pointer;
                int16_t permutation_index;
                for (permutation_index = 0; permutation_index < (int16_t)region->permutations.count; permutation_index++) {
                    if (__stricmp(permutations[permutation_index].name.string, name) == 0) {
                        int16_t stored = permutation_index;
                        if (use_matched_index == 0) {
                            stored = 0;
                        }
                        obj->region_permutations[region_index] = (uint8_t)stored;
                        break;
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f6c60):

void object_set_permutation_by_name(char *param_1,short param_2,char param_3)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  short sVar4;
  uint in_EAX;
  int iVar5;
  short sVar6;
  int iVar7;
  int iVar8;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  uVar2 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34);
  if (uVar2 != 0xffffffff) {
    iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar6 = 0;
    if (0 < *(int *)(iVar3 + 0xc4)) {
      iVar7 = 0;
      do {
        if ((param_2 == -1) || (param_2 == sVar6)) {
          iVar8 = iVar7 * 0x4c + *(int *)(iVar3 + 200);
          sVar4 = 0;
          if (0 < *(int *)(iVar7 * 0x4c + 0x40 + *(int *)(iVar3 + 200))) {
            iVar5 = 0;
            do {
              iVar5 = __stricmp((char *)(iVar5 * 0x58 + *(int *)(iVar8 + 0x44)),param_1);
              if (iVar5 == 0) {
                if (param_3 == '\0') {
                  sVar4 = 0;
                }
                *(char *)((int)puVar1 + iVar7 + 0x180) = (char)sVar4;
                break;
              }
              sVar4 = sVar4 + 1;
              iVar5 = (int)sVar4;
            } while (iVar5 < *(int *)(iVar8 + 0x40));
          }
        }
        sVar6 = sVar6 + 1;
        iVar7 = (int)sVar6;
      } while (iVar7 < *(int *)(iVar3 + 0xc4));
    }
  }
  return;
}
#endif
