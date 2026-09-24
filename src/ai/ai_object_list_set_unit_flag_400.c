// ai_object_list_set_unit_flag_400  (Ghidra: ai_object_list_set_unit_flag_400; named for this rewrite)
// address 0x4347b0, size 259 bytes
// name confidence: 0.3   rewrite confidence: 0.35
// evidence: walks an object_list (types/hs.h) exactly like ai_object_list_spawn_members
// (0x432a40, this batch), setting or clearing bit 0x400 of unit_data.flags (unit_flags,
// types/units.h; this particular bit is not yet named there) for every object in the list.
// Unlike its siblings in this address range, it does not recurse into child objects.
//   // blam-cc: EAX -> object_list_header, stack -> flag

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "ai.h"

extern data_array *object_data;                // 0x008603b0
extern data_array *object_list_header_data;    // 0x0087a464
extern data_array *object_list_reference_data; // 0x0087a468

extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0

void ai_object_list_set_unit_flag_400(datum_index object_list_header_handle, char flag)
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
        object *obj = object_try_and_get(object_index, 3);
        if (obj != 0) {
            unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
            if (flag == 0) {
                unit->flags &= ~0x400u;
            } else {
                unit->flags |= 0x400u;
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
Original Ghidra decompilation (0x4347b0):

void FUN_004347b0(char param_1)

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
        uVar6 = *(uint *)(iVar5 + 0x204) & 0xfffffbff;
      }
      else {
        uVar6 = *(uint *)(iVar5 + 0x204) | 0x400;
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
