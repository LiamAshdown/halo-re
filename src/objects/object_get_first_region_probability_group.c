// object_get_first_region_probability_group  (Ghidra: FUN_004f8ef0; renamed, Blam-style, not
// previously named)
// address 0x4f8ef0, size 93 bytes
// name confidence: 0.35 (matches functions.md's summary: "Returns the probability group of the
//   first region whose active permutation has one set")
// rewrite confidence: 0.55
// evidence: types/objects.h object (region_permutations 0x180); types/tags.h GBXModel.regions,
//   ModelRegion, ModelRegionPermutation.permutation_number; global 0x008603b0 object_data.
// register convention: object index in EAX, GBXModel pointer in EDI. Confirmed against objdump
//   -d -M intel bin/halo.exe: 0x4f8efa and eax,0xffff and 0x4f8f00 mov ebp,[edi+0xc4], neither
//   preceded by a stack read.
//   // blam-cc: EAX -> object_index, EDI -> model

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

int16_t object_get_first_region_probability_group(uint32_t object_index, GBXModel *model) // blam-cc: EAX -> object_index, EDI -> model
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    int16_t group = 0;
    int16_t region_index;

    for (region_index = 0; (group == 0) && (region_index < (int16_t)model->regions.count); region_index++) {
        ModelRegion *region = (ModelRegion *)model->regions.pointer + region_index;
        uint8_t active_permutation = obj->region_permutations[region_index];
        if (active_permutation < (int16_t)region->permutations.count) {
            ModelRegionPermutation *perm = (ModelRegionPermutation *)region->permutations.pointer + active_permutation;
            group = perm->permutation_number;
        }
    }

    return group;
}

#if 0
Original Ghidra decompilation (0x4f8ef0):

void FUN_004f8ef0(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  uint uVar3;
  short sVar4;
  int unaff_EDI;

  sVar1 = 0;
  sVar4 = 0;
  if (0 < *(int *)(unaff_EDI + 0xc4)) {
    do {
      if (sVar1 != 0) {
        return;
      }
      uVar3 = (uint)*(byte *)(sVar4 + 0x180 +
                             *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc));
      iVar2 = sVar4 * 0x4c + *(int *)(unaff_EDI + 200);
      if ((int)uVar3 < *(int *)(iVar2 + 0x40)) {
        sVar1 = *(short *)(uVar3 * 0x58 + 0x24 + *(int *)(iVar2 + 0x44));
      }
      sVar4 = sVar4 + 1;
    } while ((int)sVar4 < *(int *)(unaff_EDI + 0xc4));
  }
  return;
}
#endif
