// object_refresh_region_permutations  (Ghidra: FUN_004f8f50; renamed, Blam-style, not
// previously named)
// address 0x4f8f50, size 143 bytes
// name confidence: 0.35 (matches functions.md's summary: "Refreshes an object's region
//   permutations when its cached probability-group state is out of date")
// rewrite confidence: 0.4
// evidence: types/tags.h Object.model; global 0x008603b0 object_data, 0x0087bc14
//   tag_instances; callees object_regions_initialize_permutations (0x4f8dd0, this batch),
//   object_get_first_region_probability_group (0x4f8ef0, this batch).
// register convention: object index in EBX. Confirmed against objdump -d -M intel bin/halo.exe:
//   0x4f8f5f mov eax,ebx at entry with no stack access at all.
//   // blam-cc: EBX -> object_index
// UNSURE: the cached probability-group field at object+0xbe falls inside types/objects.h's
//   documented uint32_t unknown_0bc (0xbc..0xbf); read/written here as a raw int16 at +0xbe
//   rather than a named field.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t object_regions_initialize_permutations(uint32_t object_index, int16_t group, GBXModel *model); // 0x4f8dd0, this batch
extern int16_t object_get_first_region_probability_group(uint32_t object_index, GBXModel *model); // 0x4f8ef0, this batch

void object_refresh_region_permutations(uint32_t object_index) // blam-cc: EBX -> object_index
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    if (definition->model.tag_id.index != 0xffff) {
        GBXModel *model = (GBXModel *)tag_instances[definition->model.tag_id.index & 0xffff].data;
        int16_t *cached_group = (int16_t *)((uint8_t *)obj + 0xbe); // UNSURE: see file header

        if ((*cached_group <= 0) || (object_regions_initialize_permutations(object_index, *cached_group, model) == 0)) {
            int16_t new_group;
            object_regions_initialize_permutations(object_index, -1, model);
            new_group = object_get_first_region_probability_group(object_index, model);
            *cached_group = new_group;
            if (new_group > 0) {
                object_regions_initialize_permutations(object_index, new_group, model);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4f8f50):

void FUN_004f8f50(void)

{
  uint *puVar1;
  uint uVar2;
  undefined4 uVar3;
  char cVar4;
  short sVar5;
  uint unaff_EBX;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
  uVar2 = *(uint *)(*(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x34);
  if ((uVar2 != 0xffffffff) &&
     ((uVar3 = *(undefined4 *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
      *(short *)((int)puVar1 + 0xbe) < 1 || (cVar4 = FUN_004f8dd0(uVar3), cVar4 == '\0')))) {
    FUN_004f8dd0(uVar3);
    sVar5 = FUN_004f8ef0();
    *(short *)((int)puVar1 + 0xbe) = sVar5;
    if (0 < sVar5) {
      FUN_004f8dd0(uVar3);
    }
  }
  return;
}
#endif
