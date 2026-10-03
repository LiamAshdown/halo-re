// hs_object_list_any_angle_match  (Ghidra: FUN_004879b0)
// address 0x4879b0, size 274 bytes
// name confidence: 0.3 (out/phase4/hs_functions.md: "Returns true if any filtered object in a
//   reference list satisfies the angle-based predicate implemented by FUN_004878f0")
// rewrite confidence: 0.45
// evidence: types/hs.h object_list_header/object_list_reference; the inline validity check
//   (identifier nonzero, salt match, type bit 0 or 1 set, data pointer nonzero) matches
//   src/memory/datum_get.c's own salt check applied to an object_data entry directly rather
//   than through datum_get.
// register convention: reference-list header index in EAX (in_EAX); angle in degrees as the
//   recognized stack parameter (param_2); param_1 unused here, forwarded verbatim as the current
//   object index to hs_object_angle_predicate_helper's own (also unused) middle parameter.
//   // blam-cc: EAX -> header_index, stack -> (target_object, angle_degrees)

#include "tags.h"
#include "memory.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t hs_object_angle_predicate_helper(datum_index object_index, datum_index viewer_unit, float angle_degrees); // this module, 0x4878f0

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_data;             // 0x008603b0, stride 0x0c

// hs_object_header_entry: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// Returns 1 if any object referenced by the list headed by `header_index` is a live, valid biped
// or vehicle (per its object_data entry) whose data pointer is set and which satisfies
// hs_object_angle_predicate_helper for `angle_degrees`; otherwise 0.
uint32_t hs_object_list_any_angle_match(datum_index header_index, datum_index target_object, float angle_degrees)
{
    object_list_header *header;
    object_list_reference *reference;
    datum_index next;
    int32_t object_index;
    hs_object_header_entry *entry;
    int16_t index;
    int16_t salt;

    object_index = -1;
    next = 0xffffffff;
    if (header_index != k_datum_index_none) {
        header = (object_list_header *)((uint8_t *)object_list_header_data->data +
            (header_index & 0xffff) * 0x0c);
        next = header->first_reference;
        if (next == k_datum_index_none) {
            object_index = -1;
            next = 0xffffffff;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    }

    if (object_index == -1) {
        return 0;
    }

    do {
        index = (int16_t)object_index;
        if (object_index != -1 && index >= 0 && index < object_data->maximum_count) {
            entry = (hs_object_header_entry *)((uint8_t *)object_data->data +
                index * object_data->size);
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (1 << (entry->type_flag & 0x1f) & 3) != 0 && entry->data != 0 &&
                    hs_object_angle_predicate_helper(target_object, object_index, angle_degrees) != 0) {
                    // FIXED (0x487a62..0x487a68): EAX = the target (stack arg 1), stack = (this unit,
                    // the angle); the draft had no target parameter.
                    return 1;
                }
            }
        }
        if (next == 0xffffffff) {
            object_index = -1;
        } else {
            reference = (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                (next & 0xffff) * 0x0c);
            next = reference->next;
            object_index = reference->object_index;
        }
    } while (object_index != -1);
    return 0;
}

#if 0
Original Ghidra decompilation (0x4879b0):

undefined4 FUN_004879b0(undefined4 param_1,undefined4 param_2)

{
  char cVar1;
  uint in_EAX;
  uint uVar2;
  short *psVar3;
  uint in_ECX;
  short sVar4;
  int iVar5;
  int iVar6;

  iVar6 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar6 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar6 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  iVar5 = DAT_008603b0;
  if (iVar6 == -1) {
    return 0;
  }
  do {
    if (((iVar6 != -1) && (sVar4 = (short)iVar6, -1 < sVar4)) && (sVar4 < *(short *)(iVar5 + 0x20)))
    {
      psVar3 = (short *)((int)*(short *)(iVar5 + 0x22) * (int)sVar4 + *(int *)(iVar5 + 0x34));
      if ((((*psVar3 != 0) &&
           ((sVar4 = (short)((uint)iVar6 >> 0x10), sVar4 == 0 || (*psVar3 == sVar4)))) &&
          ((1 << (*(byte *)((int)psVar3 + 3) & 0x1f) & 3U) != 0)) &&
         ((*(int *)(psVar3 + 4) != 0 &&
          (cVar1 = FUN_004878f0(iVar6,param_2), iVar5 = DAT_008603b0, cVar1 != '\0')))) {
        return 1;
      }
    }
    if (in_ECX == 0xffffffff) {
      iVar6 = -1;
    }
    else {
      uVar2 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar6 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  } while (iVar6 != -1);
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
