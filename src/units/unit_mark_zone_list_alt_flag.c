// unit_mark_zone_list_alt_flag  (Ghidra: FUN_0056c1d0)
// address 0x56c1d0, size 278 bytes, name confidence 0.3, rewrite confidence 0.4
// functions.md: "Iterates a zone/player-indexed list of seat markers and sets one of two
// alternate flag bits on each seated unit depending on param_1."
// evidence: types/units.h unit_data.flags (0x204); bits 0x10000000/0x20000000 are not named by
//   this module's unit_flags enum (out of the range the enum's evidence covers).
// blam-cc: in_EAX -> zone_list_index, param_1 -> use_second_bit.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;             // 0x008603b0
extern data_array *object_list_header_data; // 0x0087a464
extern data_array *object_list_link_array;  // 0x0087a468

void unit_mark_zone_list_alt_flag(uint32_t zone_list_index, uint8_t use_second_bit) // blam-cc: in_EAX, param_1
{
    uint32_t object_index = 0xffffffff;
    uint32_t next_link = 0xffffffff;

    if (zone_list_index != 0xffffffff) {
        uint32_t link = *(uint32_t *)((uint8_t *)object_list_header_data->data + (zone_list_index & 0xffff) * 0xc + 8);
        if (link == 0xffffffff) {
            object_index = 0xffffffff;
            next_link = 0xffffffff;
        } else {
            uint8_t *node = (uint8_t *)object_list_link_array->data + (link & 0xffff) * 0xc;
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
            if (!use_second_bit) {
                unit->flags |= 0x20000000;
            } else {
                unit->flags |= 0x10000000;
            }
        }

        if (next_link == 0xffffffff) {
            object_index = 0xffffffff;
            next_link = 0xffffffff;
        } else {
            uint8_t *node = (uint8_t *)object_list_link_array->data + (next_link & 0xffff) * 0xc;
            next_link = *(uint32_t *)(node + 8);
            object_index = *(uint32_t *)(node + 4);
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56c1d0):

void FUN_0056c1d0(char param_1)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;
  uint in_ECX;
  short sVar4;
  short sVar5;
  uint uVar6;

  iVar2 = DAT_008603b0;
  uVar6 = 0xffffffff;
  if (in_EAX != 0xffffffff) {
    uVar6 = *(uint *)(*(int *)(DAT_0087a464 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
    if (uVar6 == 0xffffffff) {
      uVar6 = 0xffffffff;
      in_ECX = 0xffffffff;
    }
    else {
      uVar6 = uVar6 & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar6 * 0xc);
      uVar6 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + uVar6 * 0xc + 4);
    }
  }
  while (uVar6 != 0xffffffff) {
    if (((uVar6 != 0xffffffff) && (sVar5 = (short)uVar6, -1 < sVar5)) &&
       (sVar5 < *(short *)(iVar2 + 0x20))) {
      iVar1 = *(int *)(iVar2 + 0x34);
      iVar3 = (int)*(short *)(iVar2 + 0x22) * (int)sVar5;
      sVar5 = *(short *)(iVar3 + iVar1);
      iVar3 = iVar3 + iVar1;
      if ((((sVar5 != 0) && ((sVar4 = (short)(uVar6 >> 0x10), sVar4 == 0 || (sVar5 == sVar4))))
          && ((1 << (*(byte *)(iVar3 + 3) & 0x1f) & 3U) != 0)) && (*(int *)(iVar3 + 8) != 0)) {
        iVar1 = *(int *)(iVar1 + 8 + (uVar6 & 0xffff) * 0xc);
        uVar6 = *(uint *)(iVar1 + 0x204);
        if (param_1 == '\0') {
          uVar6 = uVar6 | 0x20000000;
        }
        else {
          uVar6 = uVar6 | 0x10000000;
        }
        *(uint *)(iVar1 + 0x204) = uVar6;
      }
    }
    if (in_ECX == 0xffffffff) {
      uVar6 = 0xffffffff;
      in_ECX = 0xffffffff;
    }
    else {
      uVar6 = in_ECX & 0xffff;
      in_ECX = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + 8 + uVar6 * 0xc);
      uVar6 = *(uint *)(*(int *)(DAT_0087a468 + 0x34) + uVar6 * 0xc + 4);
    }
  }
  return;
}
#endif
