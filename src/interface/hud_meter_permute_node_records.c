// hud_meter_permute_node_records  (Ghidra: FUN_00493ea0, unnamed)
// address 0x493ea0, size 84 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED against objdump 0x493ea0..0x493ef3: dest[i] = source[lookup[i]] (0x34-byte node matrices) for every node of the target model (+0xb8))
// evidence: types/interface.h first_person_weapon_interface's unknown_108c field cites this
// address ("node scratch gathered by 0x493ea0 and 0x4924b0"); out/phase4/interface_functions.md
// "Copies a permuted set of animation-node records (indexed via a lookup table) into a
// contiguous output buffer"; the target reflexive count (+0xb8) is the exact same field
// hud_meter_find_matching_elements.c reads, and the lookup table this walks is that function's
// own output array, strongly suggesting `lookup` here is a weapon_hud_element/device_hud_element
// match table built by that function.
// register convention: destination in the recognized stack parameter (param_1), source in the
// second recognized stack parameter (param_2), target tag id in EAX (in_EAX), lookup table in
// EBX (unaff_EBX). // blam-cc: EAX -> target_tag_ref, EBX -> lookup, stack -> (dest, source)
// TYPES-GAP: the 0x34 byte per-record stride (13 dwords) is not matched to any node/transform
// type in types/units.h or types/tags.h by this batch; copied as raw bytes.

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

// blam-cc: EAX -> target_tag_ref, EBX -> lookup, stack -> (dest, source)
// For each of target_tag_ref's element-list entries (target tag data + 0xb8 count, the same
// reflexive hud_meter_find_matching_elements.c reads), copies one 0x34 byte record from
// source[lookup[i]] into dest[i] -- i.e. re-orders `source` into `dest` according to a
// previously-built match table.
void hud_meter_permute_node_records(uint8_t *dest, uint8_t *source, uint32_t target_tag_ref,
                                     int16_t *lookup)
{
    char *target_data = (char *)tag_instances[(uint16_t)target_tag_ref].data;
    int32_t count = *(int32_t *)(target_data + 0xb8);
    int32_t i;

    for (i = 0; i < count; i++) {
        memcpy(dest + i * 0x34, source + lookup[i] * 0x34, 0x34);
    }
}

#if 0
Original Ghidra decompilation (0x493ea0):

void FUN_00493ea0(int param_1,int param_2)

{
  int iVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  int unaff_EBX;
  undefined4 *puVar4;
  int iVar5;
  undefined4 *puVar6;

  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 0xb8)) {
    iVar5 = 0;
    do {
      sVar2 = sVar2 + 1;
      puVar4 = (undefined4 *)(*(short *)(unaff_EBX + iVar5 * 2) * 0x34 + param_2);
      puVar6 = (undefined4 *)(iVar5 * 0x34 + param_1);
      for (iVar3 = 0xd; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar4;
        puVar4 = puVar4 + 1;
        puVar6 = puVar6 + 1;
      }
      iVar5 = (int)sVar2;
    } while (iVar5 < *(int *)(iVar1 + 0xb8));
  }
  return;
}
#endif
