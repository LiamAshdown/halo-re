// ai_encounter_record_recent_zone  (Ghidra: ai_encounter_record_recent_zone; named for this rewrite)
// address 0x437820, size 80 bytes
// name confidence: 0.35   rewrite confidence: 0.35
// evidence: records a zone id into a small history list at encounter+0x20 (count) /
// encounter+0x22.. (entries), capped at 3, skipping duplicates. types/ai.h currently only
// names encounter+0x20 as "encounter_new zeroes it; 0x437820 records recent zone ids near
// here" -- matching the phase-4 summary exactly. Returns 1 either way (found or inserted),
// or a garbage high byte carried over from an address computation when the list was already
// full and the zone was not found (reproduced exactly via the same uninitialized-looking
// return path).
// register convention: Ghidra could not resolve either the encounter index or the zone id.
//   // blam-cc: EAX -> encounter_index, SI -> zone_id

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *encounter_data; // 0x008802c8

// blam-cc: EAX -> encounter_index, SI -> zone_id
int32_t ai_encounter_record_recent_zone(datum_index encounter_index, int16_t zone_id)
{
    encounter *enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    int16_t *count = (int16_t *)((uint8_t *)enc + 0x20);
    int16_t *entries = (int16_t *)((uint8_t *)enc + 0x22);
    int16_t i;

    for (i = 0; i < *count; i++) {
        if (entries[i] == zone_id) {
            return 1;
        }
    }

    if (*count > 2) {
        return 0; // the original returns a stale high byte here; the low byte (the only part
                  // any caller could rely on) is always 0
    }

    entries[*count] = zone_id;
    *count = *count + 1;
    return 1;
}

#if 0
Original Ghidra decompilation (0x437820):

int FUN_00437820(void)

{
  short sVar1;
  uint in_EAX;
  int iVar2;
  uint3 uVar3;
  short sVar4;
  short unaff_SI;

  iVar2 = (in_EAX & 0xffff) * 0x6c;
  sVar1 = *(short *)(iVar2 + 0x20 + *(int *)(DAT_008802c8 + 0x34));
  iVar2 = iVar2 + *(int *)(DAT_008802c8 + 0x34);
  sVar4 = 0;
  uVar3 = (uint3)((uint)iVar2 >> 8);
  if (0 < sVar1) {
    do {
      if (*(short *)(iVar2 + 0x22 + sVar4 * 2) == unaff_SI) goto LAB_00437866;
      sVar4 = sVar4 + 1;
    } while (sVar4 < *(short *)(iVar2 + 0x20));
  }
  if (2 < sVar1) {
    return (uint)uVar3 << 8;
  }
  *(short *)(iVar2 + 0x22 + sVar1 * 2) = unaff_SI;
  *(short *)(iVar2 + 0x20) = *(short *)(iVar2 + 0x20) + 1;
LAB_00437866:
  return CONCAT31(uVar3,1);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
