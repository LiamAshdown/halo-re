// unit_find_seats_matching_name_and_flags  (Ghidra: FUN_0056a310)
// address 0x56a310, size 402 bytes, name confidence 0.35, rewrite confidence 0.3
// functions.md: "Scans the unit's seat/marker definitions for unoccupied entries whose name
// matches (or is empty) and whose type flags match flag_selector, returning up to max_indices
// matching seat indices."
// evidence: types/tags.h Unit.seats (0x2e4 count / 0x2e8 pointer), UnitSeat (0x11c stride,
//   label TagString at +4, flags dword at +0).
// blam-cc: param_1 -> unit_index, param_2 -> name_filter (may be NULL/empty), param_3 ->
//   flag_selector, param_4 -> out_indices, param_5 -> max_indices.
// UNSURE: strstr's exact semantics (a case-sensitive/wildcard string match against the
//   already-lowercased label) and the exact meaning of each flag_selector case's bit test are
//   not recovered beyond the raw bit arithmetic.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int tolower(int c); // 0x624687 (the locale-aware tolower, not _tolower)
extern int32_t strstr(uint8_t *lowered_label, char *name_filter); // 0x625430, UNSURE signature
extern uint8_t unit_is_seat_occupied(int32_t parent_index, int16_t seat_index); // 0x56cc10, UNSURE signature

int16_t unit_find_seats_matching_name_and_flags(uint32_t unit_index, char *name_filter, uint16_t flag_selector,
                                                int16_t *out_indices, int16_t max_indices)
{
    object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    Unit *unit_tag = (Unit *)tag_instances[unit_obj->definition_tag & 0xffff].data;

    uint8_t name_is_empty;
    if (name_filter != (char *)0) {
        int32_t len = 0;
        while (name_filter[len] != '\0') len++;
        name_is_empty = (len == 0);
    } else {
        name_is_empty = 1;
    }

    int16_t out_count = 0;
    if ((int32_t)unit_tag->seats.count < 1) {
        return 0;
    }

    UnitSeat *seats = (UnitSeat *)unit_tag->seats.pointer;
    for (int16_t seat_index = 0; seat_index < (int32_t)unit_tag->seats.count; seat_index++) {
        if (max_indices <= out_count) break;

        char lowered[256];
        char *src = seats[seat_index].label.string;
        int32_t i = 0;
        do {
            lowered[i] = (char)tolower((uint8_t)src[i]);
            i++;
        } while (src[i - 1] != '\0');

        if ((!name_is_empty) && (strstr((uint8_t *)lowered, name_filter) == 0)) {
            continue;
        }

        uint32_t flags = seats[seat_index].flags;
        uint8_t match;
        switch (flag_selector) {
        case 0:
            match = (~(uint8_t)(flags >> 2)) & 1;
            break;
        case 1:
            match = (uint8_t)(flags >> 3) & 1;
            break;
        case 2:
            if ((flags & 4) || (flags & 8)) {
                continue;
            }
            match = 1;
            break;
        case 3:
            match = (uint8_t)(flags >> 2) & 1;
            break;
        default:
            match = 1;
            break;
        }

        if (match & 1) {
            if (unit_is_seat_occupied(unit_index, seat_index) == 0) {
                out_indices[out_count] = seat_index;
                out_count++;
            }
        }
    }
    return out_count;
}

#if 0
Original Ghidra decompilation (0x56a310):

uint FUN_0056a310(uint param_1,char *param_2,undefined2 param_3,int param_4,short param_5)

{
  byte *pbVar1;
  int iVar2;
  bool bVar3;
  byte bVar4;
  char cVar5;
  short sVar6;
  char *pcVar7;
  uint uVar8;
  uint *puVar9;
  uint *puVar10;
  int iVar11;
  short sVar12;
  byte *pbVar13;
  byte local_100 [256];

  iVar2 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc) &
                   0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar12 = 0;
  uVar8 = 0;
  if (param_2 != (char *)0x0) {
    pcVar7 = param_2;
    do {
      cVar5 = *pcVar7;
      pcVar7 = pcVar7 + 1;
    } while (cVar5 != '\0');
    uVar8 = (int)pcVar7 - (int)(param_2 + 1);
    bVar3 = false;
    if (uVar8 != 0) goto LAB_0056a378;
  }
  bVar3 = true;
LAB_0056a378:
  sVar6 = 0;
  if (*(int *)(iVar2 + 0x2e4) < 1) {
    return uVar8 & 0xffff0000;
  }
  puVar9 = (uint *)0x0;
  do {
    puVar9 = (uint *)((int)puVar9 * 0x11c + *(int *)(iVar2 + 0x2e8));
    if (param_5 <= sVar12) break;
    puVar10 = puVar9 + 1;
    iVar11 = -(int)puVar10;
    do {
      uVar8 = *puVar10;
      (local_100 + iVar11)[(int)puVar10] = (byte)uVar8;
      puVar10 = (uint *)((int)puVar10 + 1);
    } while ((byte)uVar8 != 0);
    pbVar13 = local_100;
    bVar4 = local_100[0];
    while (bVar4 != 0) {
      iVar11 = _tolower((uint)*pbVar13);
      *pbVar13 = (byte)iVar11;
      pbVar1 = pbVar13 + 1;
      pbVar13 = pbVar13 + 1;
      bVar4 = *pbVar1;
    }
    if ((!bVar3) && (iVar11 = FUN_00625430(local_100,param_2), iVar11 == 0)) goto LAB_0056a46e;
    switch(param_3) {
    case 0:
      bVar4 = ~(byte)(*puVar9 >> 2);
      break;
    case 1:
      bVar4 = (byte)(*puVar9 >> 3);
      break;
    case 2:
      if (((*puVar9 & 4) != 0) || ((*puVar9 & 8) != 0)) goto LAB_0056a46e;
      goto switchD_0056a416_default;
    case 3:
      bVar4 = (byte)(*puVar9 >> 2);
      break;
    default:
      goto switchD_0056a416_default;
    }
    if ((bVar4 & 1) != 0) {
switchD_0056a416_default:
      cVar5 = FUN_0056cc10();
      if (cVar5 == '\0') {
        *(short *)(param_4 + sVar12 * 2) = sVar6;
        sVar12 = sVar12 + 1;
      }
    }
LAB_0056a46e:
    sVar6 = sVar6 + 1;
    puVar9 = (uint *)(int)sVar6;
  } while ((int)puVar9 < *(int *)(iVar2 + 0x2e4));
  return CONCAT22((short)((uint)puVar9 >> 0x10),sVar12);
}
#endif
