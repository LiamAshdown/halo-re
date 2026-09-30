// hud_meter_find_matching_elements  (Ghidra: FUN_00493f00, unnamed)
// address 0x493f00, size 258 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED against objdump 0x493f00..0x49400a: for each target model node (+0xb8/+0xbc, 0x9c each) the first source animation-graph node (+0x68/+0x6c, 0x40 each) with the same name; out[target] = source index, unmatched targets leave out untouched and clear the result)
// evidence: out/phase4/interface_functions.md "Scans a HUD interface's meter/element list for
// entries matching a target table, recording matches by index for later use."; types/interface.h
// first_person_weapon_interface's weapon_hud_element/device_hud_element fields both cite this
// address as their filler.
// register convention: source tag id in EAX (in_EAX), target tag id in ECX (in_ECX), output
// array as the recognized stack parameter (param_1). // blam-cc: EAX -> source_tag_ref, ECX -> target_tag_ref,
// stack -> out
// TYPES-GAP / UNSURE: both tag reflexives walked here (target+0xb8/+0xbc, a count/pointer pair
// over 0x9c byte name records; source+0x68/+0x6c, a count/pointer pair over 0x40 byte name
// records) are unidentified -- not matched to any *HUDInterface* reflexive in types/tags.h by
// this batch. Reproduced as raw offsets. The two call sites in
// first_person_weapon_interface_initialize.c only show this function's output-array argument;
// its source/target tag id arguments there are supplied as that function's own weapon tag and
// weapon-hud-interface tag references, which is this batch's best guess, not an attested fact.
// UNSURE: the inner match test is a manual byte-pair comparison (case-sensitive, matching
// Ghidra literally) reproduced here as `strcmp`, which is exactly equivalent. A secondary
// `sVar9 != -1` check guarding whether a found match is actually recorded (vs. breaking without
// recording) can only ever be false if the source index counter wrapped through -1, which no
// realistic element list reaches; dropped as unreachable rather than modelled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"
#include <string.h>

extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> source_tag_ref, ECX -> target_tag_ref, stack -> out
// For every entry in target's element list (target tag data + 0xb8 count, +0xbc pointer to
// 0x9c-byte name records), searches source's element list (source tag data + 0x68 count, +0x6c
// pointer to 0x40-byte name records, matched in name order starting after the previous match) for
// a name match, writing the found source index into out[i] on success. Returns 1 (as the low
// byte of a value whose upper bits are decompiler noise) only if every target entry found a
// match; returns 0 as soon as one target entry runs out of source entries to check, but keeps
// scanning the remaining target entries regardless.
uint8_t hud_meter_find_matching_elements(uint32_t source_tag_ref, uint32_t target_tag_ref,
                                          int16_t *out)
{
    char *target_data = (char *)tag_instances[(uint16_t)target_tag_ref].data;
    char *source_data = (char *)tag_instances[(uint16_t)source_tag_ref].data;
    uint8_t all_matched = 1;
    int32_t target_count = *(int32_t *)(target_data + 0xb8);
    int32_t target_index;
    int32_t source_count = *(int32_t *)(source_data + 0x68);
    char *source_names = *(char **)(source_data + 0x6c);
    char *target_names = *(char **)(target_data + 0xbc);

    if (target_count <= 0) {
        return 1;
    }

    for (target_index = 0; target_index < target_count; target_index++) {
        char *target_name = target_names + target_index * 0x9c;
        int16_t source_index;
        uint8_t found = 0;

        for (source_index = 0; source_index < source_count; source_index++) {
            char *source_name = source_names + source_index * 0x40;
            if (strcmp(source_name, target_name) == 0) { // matches equal-length byte-pair
                                                           // compare Ghidra shows
                found = 1;
                break;
            }
        }
        if (found) {
            out[target_index] = source_index;
        } else {
            all_matched = 0;
        }
    }
    return all_matched;
}

#if 0
Original Ghidra decompilation (0x493f00):

undefined4 FUN_00493f00(int param_1)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  undefined4 uVar5;
  uint in_ECX;
  int iVar6;
  byte *pbVar7;
  byte *pbVar8;
  short sVar9;
  bool bVar10;
  undefined1 local_15;
  int local_14;
  int local_10;

  iVar2 = *(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar4 = (in_EAX & 0xffff) * 0x20;
  iVar3 = *(int *)(iVar4 + 0x14 + DAT_0087bc14);
  uVar5 = CONCAT31((int3)((uint)iVar4 >> 8),1);
  local_15 = 1;
  local_14 = 0;
  if (0 < *(int *)(iVar2 + 0xb8)) {
    local_10 = 0;
    do {
      iVar4 = *(int *)(iVar3 + 0x68);
      sVar9 = 0;
      if (0 < iVar4) {
        iVar6 = 0;
        do {
          pbVar7 = (byte *)(iVar6 * 0x40 + *(int *)(iVar3 + 0x6c));
          pbVar8 = (byte *)(local_10 * 0x9c + *(int *)(iVar2 + 0xbc));
          do {
            bVar1 = *pbVar8;
            bVar10 = bVar1 < *pbVar7;
            if (bVar1 != *pbVar7) {
LAB_00493fb4:
              iVar6 = (1 - (uint)bVar10) - (uint)(bVar10 != 0);
              goto LAB_00493fb9;
            }
            if (bVar1 == 0) break;
            bVar1 = pbVar8[1];
            bVar10 = bVar1 < pbVar7[1];
            if (bVar1 != pbVar7[1]) goto LAB_00493fb4;
            pbVar8 = pbVar8 + 2;
            pbVar7 = pbVar7 + 2;
          } while (bVar1 != 0);
          iVar6 = 0;
LAB_00493fb9:
          if (iVar6 == 0) {
            if (sVar9 != -1) {
              *(short *)(param_1 + local_10 * 2) = sVar9;
              goto LAB_00493fca;
            }
            break;
          }
          sVar9 = sVar9 + 1;
          iVar6 = (int)sVar9;
        } while (iVar6 < iVar4);
      }
      local_15 = 0;
LAB_00493fca:
      local_14 = local_14 + 1;
      local_10 = (int)(short)local_14;
    } while (local_10 < *(int *)(iVar2 + 0xb8));
    uVar5 = CONCAT31((int3)((uint)local_14 >> 8),local_15);
  }
  return uVar5;
}
#endif
