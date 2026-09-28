// hs_object_set_permutation_by_name  (Ghidra: FUN_00488670)
// address 0x488670, size 195 bytes
// name confidence: 0.4 (out/phase4/hs_functions.md: "Resolves a name string to an index within a
//   named sub-list of an object's tag data (e.g. a seat, region, or marker list) and passes the
//   result to a setter"; the callee object_set_permutation_by_name and the 0x4c-stride table with
//   a name at its head pin this to the model permutation list specifically)
// rewrite confidence: 0.85 (FIXED the object_set_permutation_by_name call (EAX object, stack name/region/1) and the documented convention (object and permutation on the stack, EBX region name))
// evidence: out/phase4/hs_types_notes.md object/tag_instance chain (object data pointer at +8 of
//   an object_headers element, tag_instance.data at +0x14 of a 0x20-stride tag_instances element);
//   callee name object_set_permutation_by_name.
// register convention: object index in EAX (in_EAX); permutation name string in EBX (unaff_EBX);
//   the value forwarded unchanged to object_set_permutation_by_name (param_2) as the recognized
//   stack parameter.
//   // blam-cc: EBX -> name, stack -> object_index, param_2
// UNSURE: the two-level tag lookup (object's own tag -> a referenced tag's permutations list at
// definition+200/+0xc4, stride 0x4c) is preserved exactly by offset, but the intermediate tag
// group/field names (a model tag's permutations block, most likely) are not established anywhere
// in types/tags.h under those exact offsets for this module, so raw offsets are kept.

#include "crt.h"
#include "tags.h"
#include "cache.h"
#include "memory.h"
#include "hs.h"

extern void object_set_permutation_by_name(uint32_t object_index, char *name, int16_t region_filter,
    char use_matched_index); // 0x4f6c60, EAX object, stack (name, region, use)

extern data_array *object_headers;  // 0x008603b0, stride 0x0c, object data pointer at +0x08
extern tag_instance *tag_instances; // 0x0087bc14

// Resolves `name` against the case-insensitive list of permutation names on the tag referenced
// by object_index's own tag definition (offset 0x34 of that definition), and forwards the
// matched index (or -1 for an empty name, not found, or a missing reference) to
// object_set_permutation_by_name.
void hs_object_set_permutation_by_name(datum_index object_index, void *param_2, char *name)
{
    void **object_data;
    uint32_t referenced_tag_id;
    uint8_t *definition;
    int32_t permutation_count;
    int32_t index;
    int32_t match_index;

    if (object_index != k_datum_index_none) {
        match_index = -1;
        if (name[0] != '\0') {
            object_data = *(void ***)((uint8_t *)object_headers->data +
                (object_index & 0xffff) * 0x0c + 8);
            referenced_tag_id = *(uint32_t *)((uint8_t *)tag_instances[
                (*(uint32_t *)object_data & 0xffff) & 0xffff].data + 0x34);
            if (referenced_tag_id != 0xffffffff) {
                definition = (uint8_t *)tag_instances[(referenced_tag_id & 0xffff) & 0xffff].data;
                permutation_count = *(int32_t *)(definition + 0xc4);
                index = 0;
                if (0 < permutation_count) {
                    do {
                        if (_stricmp((char *)(*(int32_t *)(definition + 200) + index * 0x4c),
                                name) == 0) {
                            match_index = index;
                            break;
                        }
                        index = index + 1;
                    } while (index < permutation_count);
                }
            }
        }
        // FIXED (0x48871d..0x488731): EAX = the object, stack (permutation name, region index, 1); the
        //   draft dropped the object.
        object_set_permutation_by_name(object_index, (char *)param_2, (int16_t)match_index, 1);
    }
}

#if 0
Original Ghidra decompilation (0x488670):

void FUN_00488670(uint param_1,undefined4 param_2)

{
  uint uVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  char *unaff_EBX;
  char *pcVar5;
  char *pcVar6;
  int iVar7;
  bool bVar8;
  int local_4;

  if (param_1 != 0xffffffff) {
    iVar4 = 1;
    bVar8 = true;
    pcVar5 = unaff_EBX;
    pcVar6 = "";
    do {
      if (iVar4 == 0) break;
      iVar4 = iVar4 + -1;
      bVar8 = *pcVar5 == *pcVar6;
      pcVar5 = pcVar5 + 1;
      pcVar6 = pcVar6 + 1;
    } while (bVar8);
    local_4 = -1;
    iVar4 = local_4;
    if ((!bVar8) &&
       (uVar1 = *(uint *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (param_1 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                  DAT_0087bc14) + 0x34), uVar1 != 0xffffffff)) {
      iVar2 = *(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      iVar7 = 0;
      if (0 < *(int *)(iVar2 + 0xc4)) {
        iVar3 = 0;
        do {
          iVar3 = __stricmp((char *)(iVar3 * 0x4c + *(int *)(iVar2 + 200)),unaff_EBX);
          iVar4 = iVar7;
          if (iVar3 == 0) break;
          iVar7 = iVar7 + 1;
          iVar3 = (int)(short)iVar7;
          iVar4 = local_4;
        } while (iVar3 < *(int *)(iVar2 + 0xc4));
      }
    }
    local_4 = iVar4;
    object_set_permutation_by_name(param_2,local_4,1);
  }
  return;
}
#endif
