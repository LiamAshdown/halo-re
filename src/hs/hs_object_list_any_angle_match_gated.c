// hs_object_list_any_angle_match_gated  (Ghidra: FUN_00487ad0)
// address 0x487ad0, size 308 bytes
// name confidence: 0.25 (out/phase4/hs_functions.md: "Variant of the reference-list angle test
//   that adds an extra caller-supplied gating flag before invoking the geometry predicate")
// rewrite confidence: 0.9 (VERIFIED against 0x487ad0 (list walk, header salt/type/data checks, cutscene flag position, EDI/ECX/stack call))
// evidence: near-duplicate of hs_object_list_any_angle_match.c (0x4879b0), inlining
//   unit_point_within_look_cone directly instead of going through hs_object_angle_predicate_helper.
// register convention: reference-list header index in EAX (in_EAX); extra gating flag in BX
//   (unaff_BX); angle in degrees as the recognized stack parameter (param_1).
//   // blam-cc: EAX -> header_index, BX -> gate, stack -> angle_degrees
// UNSURE: `gate`'s true meaning (unaff_BX) could not be recovered; it is ANDed in unchanged.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

extern uint8_t unit_point_within_look_cone(float cone_angle, uint32_t unit_index, real_point3d *world_point);
    // 0x56c100, blam-cc: stack, ECX, EDI
extern Scenario *global_scenario;

extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468
extern data_array *object_headers;             // 0x008603b0, stride 0x0c

// hs_object_header_entry: defined in types/hs.h (foreign-module slice; was a local TYPES-GAP copy)

// As hs_object_list_any_angle_match, but requires `gate` to be nonzero (in addition to the object
// being a live, valid biped/vehicle with a data pointer) before evaluating the angle test itself.
uint32_t hs_object_list_any_angle_match_gated(datum_index header_index, int16_t gate,
    float angle_degrees)
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
            object_index = reference->object_index;
            next = reference->next;
        }
    }

    if (object_index == -1) {
        return 0;
    }

    do {
        index = (int16_t)object_index;
        if (object_index != -1 && index >= 0 && index < object_headers->maximum_count) {
            entry = (hs_object_header_entry *)((uint8_t *)object_headers->data +
                index * object_headers->size);
            if (entry->identifier != 0) {
                salt = (int16_t)((uint32_t)object_index >> 0x10);
                if ((salt == 0 || entry->identifier == salt) &&
                    (1 << (entry->type_flag & 0x1f) & 3) != 0 && entry->data != 0 &&
                    gate != 0 && unit_point_within_look_cone(angle_degrees * 0.017453292f, object_index,
                        (real_point3d *)((uint8_t *)global_scenario->cutscene_flags.pointer + gate * 0x5c + 0x24)) != 0) {
                    // FIXED (0x487b86..0x487bac): ECX = the unit, EDI = the cutscene flag's position
                    // (scenario +0x4e8 flags, 0x5c each, +0x24); the draft passed only the angle.
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
Original Ghidra decompilation (0x487ad0):

undefined4 FUN_00487ad0(float param_1)

{
  char cVar1;
  uint in_EAX;
  uint uVar2;
  short *psVar3;
  uint in_ECX;
  short sVar4;
  short unaff_BX;
  int iVar5;
  int iVar6;

  iVar5 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar5 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      uVar2 = uVar2 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  }
  iVar6 = DAT_008603b0;
  if (iVar5 == -1) {
    return 0;
  }
  do {
    if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) && (sVar4 < *(short *)(iVar6 + 0x20)))
    {
      psVar3 = (short *)((int)*(short *)(iVar6 + 0x22) * (int)sVar4 + *(int *)(iVar6 + 0x34));
      if (((*psVar3 != 0) &&
          ((sVar4 = (short)((uint)iVar5 >> 0x10), sVar4 == 0 || (*psVar3 == sVar4)))) &&
         (((1 << (*(byte *)((int)psVar3 + 3) & 0x1f) & 3U) != 0 &&
          (((*(int *)(psVar3 + 4) != 0 && (unaff_BX != 0)) &&
           (cVar1 = FUN_0056c100(param_1 * 0.017453292), iVar6 = DAT_008603b0, cVar1 != '\0')))))) {
        return 1;
      }
    }
    if (in_ECX == 0xffffffff) {
      iVar5 = -1;
    }
    else {
      uVar2 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar2 * 0xc);
      iVar5 = *(int *)(*(int *)(DAT_0087a468 + 0x34) + uVar2 * 0xc + 4);
    }
  } while (iVar5 != -1);
  return 0;
}
#endif
