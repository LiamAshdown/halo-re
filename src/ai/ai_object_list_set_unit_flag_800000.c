// ai_object_list_set_unit_flag_800000  (Ghidra: FUN_00561d50; named for this rewrite)
// address 0x561d50, size 259 bytes
// name confidence: 0.55   rewrite confidence: 0.65
// evidence: byte-for-byte the same walk/shape as its siblings ai_object_list_set_unit_flag_400
//   (0x434730) and ai_object_list_set_unit_flag_800 (0x4348c0, this directory) -- same
//   object_list walk (types/hs.h, the 0x0087a464 / 0x0087a468 pair every ai_object_list_*
//   function in this directory uses), same unit-type filter, same "set or clear a bit of
//   unit_data.flags for every member" body, differing only in which bit: this one is
//   `_unit_flag_unknown_800000` (types/units.h already documents that bit as "0x561d50 sets or
//   clears it over a chain", i.e. this exact address, from before this address was rewritten).
// register convention: object_list_header handle in EAX, flag in a stack byte parameter.
// Confirmed against objdump 0x561d50..0x561e5b (identical shape to 0x4348c0's own confirmed
// register convention).
//   // blam-cc: EAX -> object_list_header_handle, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

void ai_object_list_set_unit_flag_800000(datum_index object_list_header_handle, char flag)
{
    datum_index node_index = (datum_index)k_datum_index_none;
    datum_index object_index = (datum_index)k_datum_index_none;

    if (object_list_header_handle != (datum_index)k_datum_index_none) {
        object_list_header *header =
            (object_list_header *)((uint8_t *)object_list_header_data->data +
                                    (object_list_header_handle & 0xffff) * 0x0c);
        node_index = header->first_reference;
        if (node_index != (datum_index)k_datum_index_none) {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            node_index = node->next;
            object_index = node->object_index;
        }
    }

    while (object_index != (datum_index)k_datum_index_none) {
        object_header *entry = 0;
        if (0 <= (int16_t)object_index && (int16_t)object_index < object_data->maximum_count) {
            object_header *candidate = &((object_header *)object_data->data)[(int16_t)object_index];
            if (candidate->identifier != 0 &&
                ((int16_t)(object_index >> 0x10) == 0 || candidate->identifier == (int16_t)(object_index >> 0x10))) {
                entry = candidate;
            }
        }

        if (entry != 0 && ((1 << (entry->type & 0x1f)) & 3) != 0 && entry->data != 0) {
            unit_data *unit = (unit_data *)((uint8_t *)entry->data + k_unit_data_offset);
            if (flag == 0) {
                unit->flags &= ~(uint32_t)_unit_flag_unknown_800000;
            } else {
                unit->flags |= (uint32_t)_unit_flag_unknown_800000;
            }
        }

        if (node_index == (datum_index)k_datum_index_none) {
            object_index = (datum_index)k_datum_index_none;
        } else {
            object_list_reference *node =
                (object_list_reference *)((uint8_t *)object_list_reference_data->data +
                                           (node_index & 0xffff) * 0x0c);
            object_index = node->object_index;
            node_index = node->next;
        }
    }
}

#if 0
Original Ghidra decompilation (0x561d50):

void FUN_00561d50(char param_1)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  uint in_ECX;
  short *psVar3;
  short sVar4;
  int iVar5;
  uint uVar6;
  short sVar7;
  short *psVar8;

  iVar2 = DAT_0087a468;
  iVar1 = DAT_008603b0;
  iVar5 = -1;
  if (in_EAX != 0xffffffff) {
    uVar6 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar6 == 0xffffffff) {
      iVar5 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      iVar5 = *(int *)(DAT_0087a468 + 0x34) + (uVar6 & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar5 + 8);
      iVar5 = *(int *)(iVar5 + 4);
    }
  }
  while (iVar5 != -1) {
    psVar8 = (short *)0x0;
    if (((iVar5 != -1) && (sVar4 = (short)iVar5, -1 < sVar4)) && (sVar4 < *(short *)(iVar1 + 0x20)))
    {
      psVar3 = (short *)((int)*(short *)(iVar1 + 0x22) * (int)sVar4 + *(int *)(iVar1 + 0x34));
      sVar4 = *psVar3;
      if ((sVar4 != 0) && ((sVar7 = (short)((uint)iVar5 >> 0x10), sVar7 == 0 || (sVar4 == sVar7))))
      {
        psVar8 = psVar3;
      }
    }
    if (((psVar8 != (short *)0x0) && ((1 << (*(byte *)((int)psVar8 + 3) & 0x1f) & 3U) != 0)) &&
       (iVar5 = *(int *)(psVar8 + 4), iVar5 != 0)) {
      if (param_1 == '\0') {
        uVar6 = *(uint *)(iVar5 + 0x204) & 0xff7fffff;
      }
      else {
        uVar6 = *(uint *)(iVar5 + 0x204) | 0x800000;
      }
      *(uint *)(iVar5 + 0x204) = uVar6;
    }
    if (in_ECX == 0xffffffff) {
      iVar5 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      iVar5 = *(int *)(iVar2 + 0x34) + (in_ECX & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar5 + 8);
      iVar5 = *(int *)(iVar5 + 4);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
