// unit_mark_zone_occupants_flag  (Ghidra: FUN_0056b290)
// address 0x56b290, size 235 bytes, name confidence 0.3, rewrite confidence 0.4
// functions.md: "Iterates a player/zone-indexed list of seat markers and marks each seated
// unit's flags field with bit 0x100000."
// evidence: types/units.h unit_flags._unit_flag_delete_when_dropped (0x100000); the same
// {unused, object_index, next_link} 0xc-stride list walk as
// unit_seat_candidates_from_zone_and_enter.c (0x56a4c0).
// blam-cc: in_EAX -> zone_list_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data;             // 0x008603b0
extern data_array *object_list_header_data; // 0x0087a464
extern data_array *object_list_reference_data;  // 0x0087a468

void unit_mark_zone_occupants_flag(uint32_t zone_list_index) // blam-cc: in_EAX
{
    uint32_t object_index = 0xffffffff; // iVar7
    uint32_t next_link = 0xffffffff;    // in_ECX

    if (zone_list_index != 0xffffffff) {
        uint32_t link = *(uint32_t *)((uint8_t *)object_list_header_data->data + (zone_list_index & 0xffff) * 0xc + 8);
        if (link == 0xffffffff) {
            object_index = 0xffffffff;
            next_link = 0xffffffff;
        } else {
            uint8_t *node = (uint8_t *)object_list_reference_data->data + (link & 0xffff) * 0xc;
            next_link = *(uint32_t *)(node + 8);
            object_index = *(uint32_t *)(node + 4);
        }
    }

    while (object_index != 0xffffffff) {
        object_header *found = (object_header *)0;
        if ((-1 < (int16_t)object_index) && ((int16_t)object_index < object_data->maximum_count)) {
            object_header *hdr = (object_header *)object_data->data + (int16_t)object_index;
            if ((hdr->identifier != 0) &&
                (((int16_t)(object_index >> 16) == 0) || (hdr->identifier == (int16_t)(object_index >> 16)))) {
                found = hdr;
            }
        }
        if ((found != (object_header *)0) && ((_object_mask_unit & (1 << (found->type & 0x1f))) != 0) &&
            (found->data != (object *)0)) {
            unit_data *unit = (unit_data *)((uint8_t *)found->data + k_unit_data_offset);
            unit->flags |= 0x100000;
        }

        if (next_link == 0xffffffff) {
            object_index = 0xffffffff;
            next_link = 0xffffffff;
        } else {
            uint8_t *node = (uint8_t *)object_list_reference_data->data + (next_link & 0xffff) * 0xc;
            next_link = *(uint32_t *)(node + 8);
            object_index = *(uint32_t *)(node + 4);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56b290):

void FUN_0056b290(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint in_EAX;
  uint in_ECX;
  short *psVar5;
  short sVar6;
  int iVar7;
  short sVar8;
  short *psVar9;

  iVar4 = DAT_0087a468;
  iVar3 = DAT_008603b0;
  iVar7 = -1;
  if (in_EAX != 0xffffffff) {
    uVar2 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar2 == 0xffffffff) {
      iVar7 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      iVar7 = *(int *)(DAT_0087a468 + 0x34) + (uVar2 & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar7 + 8);
      iVar7 = *(int *)(iVar7 + 4);
    }
  }
  while (iVar7 != -1) {
    psVar9 = (short *)0x0;
    if (((iVar7 != -1) && (sVar6 = (short)iVar7, -1 < sVar6)) && (sVar6 < *(short *)(iVar3 + 0x20)))
    {
      psVar5 = (short *)((int)*(short *)(iVar3 + 0x22) * (int)sVar6 + *(int *)(iVar3 + 0x34));
      sVar6 = *psVar5;
      if ((sVar6 != 0) && ((sVar8 = (short)((uint)iVar7 >> 0x10), sVar8 == 0 || (sVar6 == sVar8))))
      {
        psVar9 = psVar5;
      }
    }
    if (((psVar9 != (short *)0x0) && ((1 << (*(byte *)((int)psVar9 + 3) & 0x1f) & 3U) != 0)) &&
       (*(int *)(psVar9 + 4) != 0)) {
      puVar1 = (uint *)(*(int *)(psVar9 + 4) + 0x204);
      *puVar1 = *puVar1 | 0x100000;
    }
    if (in_ECX == 0xffffffff) {
      iVar7 = -1;
      in_ECX = 0xffffffff;
    }
    else {
      iVar7 = *(int *)(iVar4 + 0x34) + (in_ECX & 0xffff) * 0xc;
      in_ECX = *(uint *)(iVar7 + 8);
      iVar7 = *(int *)(iVar7 + 4);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
