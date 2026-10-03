// object_regions_initialize_permutations  (named by out/phase4/objects_types_notes.md:
// "object_permutation_find_matching_group 0x4f8d80, object_regions_initialize_permutations
// 0x4f8dd0")
// address 0x4f8dd0, size 284 bytes
// name confidence: 0.85 (fixed by the types notes' own citation of this address by this name)
// rewrite confidence: 0.5
// evidence: types/objects.h object (region_permutations 0x180); types/tags.h GBXModel.regions,
// ModelRegion, ModelRegionPermutation; global 0x008603b0 object_data, global 0x00719cd0
// random_seed_global; callee object_permutation_find_matching_group (0x4f8d80, this batch).
// register convention: object index in EAX, probability-group filter in CX, GBXModel pointer as
//   the sole stack parameter. Confirmed against objdump -d -M intel bin/halo.exe: 0x4f8dda
//   mov edi,ecx (the filter, carried in EDI's low word across the loop) and 0x4f8de5
//   mov ecx,[esp+0x58] (the model pointer).
//   // blam-cc: EAX -> object_index, CX -> group, stack -> model
// UNSURE: return value packs the success flag in AL with garbage in the upper 3 bytes
//   (Ghidra's CONCAT31 pattern); narrowed to a plain bool here since every use only tests it as
//   one (per FUN_004f8f50, this batch).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern uint32_t random_seed_global; // 0x00719cd0

extern int16_t object_permutation_find_matching_group(ModelRegion *region, int16_t group, int16_t *out); // 0x4f8d80, this batch

uint8_t object_regions_initialize_permutations(uint32_t object_index, int16_t group, GBXModel *model)
    // blam-cc: EAX -> object_index, CX -> group, stack -> model
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    uint8_t all_assigned = 1;
    int16_t region_index;

    for (region_index = 0; region_index < (int16_t)model->regions.count; region_index++) {
        ModelRegion *region = (ModelRegion *)model->regions.pointer + region_index;
        int16_t matches[32]; // UNSURE: sized generously; the original reserves a large stack buffer
        int16_t match_count = object_permutation_find_matching_group(region, group, matches);
        int16_t chosen;

        if (match_count == 0) {
            if (group != -1) {
                match_count = object_permutation_find_matching_group(region, 0, matches);
            }
            if (match_count == 0) {
                obj->region_permutations[region_index] = 0;
                all_assigned = 0;
                continue;
            }
        }

        if (match_count == 1) {
            chosen = 0;
        } else {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            chosen = (int16_t)(((int32_t)(random_seed_global >> 0x10) * match_count) >> 0x10);
        }
        obj->region_permutations[region_index] = (uint8_t)matches[chosen];
    }

    return all_assigned;
}

#if 0
Original Ghidra decompilation (0x4f8dd0):

undefined4 FUN_004f8dd0(int param_1)

{
  int iVar1;
  int iVar2;
  short sVar3;
  uint in_EAX;
  undefined4 uVar4;
  int iVar5;
  short in_CX;
  short sVar6;
  int iVar7;
  int iVar8;
  short asStackY_10040 [32752];
  undefined1 local_49;
  int local_44;
  short local_40 [32];

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  iVar7 = 0;
  uVar4 = CONCAT31((int3)((uint)iVar1 >> 8),1);
  local_49 = 1;
  local_44 = 0;
  if (0 < *(int *)(param_1 + 0xc4)) {
    do {
      iVar8 = iVar7 * 0x4c + *(int *)(param_1 + 200);
      sVar3 = FUN_004f8d80(local_40);
      if (sVar3 == 0) {
        if (in_CX != -1) {
          iVar2 = *(int *)(iVar8 + 0x40);
          sVar3 = 0;
          sVar6 = 0;
          if (0 < iVar2) {
            iVar8 = *(int *)(iVar8 + 0x44);
            iVar5 = 0;
            do {
              iVar5 = iVar5 * 0x58 + iVar8;
              if (((*(byte *)(iVar5 + 0x20) & 1) == 0) && (*(short *)(iVar5 + 0x24) == 0)) {
                local_40[sVar3] = sVar6;
                sVar3 = sVar3 + 1;
              }
              sVar6 = sVar6 + 1;
              iVar5 = (int)sVar6;
            } while (iVar5 < iVar2);
          }
          if (sVar3 != 0) goto LAB_004f8e82;
        }
        *(undefined1 *)(iVar7 + 0x180 + iVar1) = 0;
        local_49 = 0;
      }
      else {
LAB_004f8e82:
        if (sVar3 == 1) {
          sVar3 = 0;
        }
        else {
          DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
          sVar3 = (short)((DAT_00719cd0 >> 0x10) * (int)sVar3 >> 0x10);
        }
        *(char *)(iVar7 + 0x180 + iVar1) = (char)local_40[sVar3];
      }
      local_44 = local_44 + 1;
      iVar7 = (int)(short)local_44;
    } while (iVar7 < *(int *)(param_1 + 0xc4));
    uVar4 = CONCAT31((int3)((uint)local_44 >> 8),local_49);
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
