// input_device_find_index_by_guid  (Ghidra: already named)
// address 0x4916e0, size 100 bytes
// name confidence: 0.65   rewrite confidence: 0.75
// evidence: out/phase4/input_types_notes.md: "input_device_find_index_by_guid 0x4916e0 (+0x20c
// 16-byte compare, +0x21c pre-check)". +0x20c and +0x21c are controls_gamepad_record::device_key
// offsets ([0] product guid low dword, [4] instance number); Ghidra's own __cdecl signature
// confirms one pointer argument.
// register convention: __cdecl, one stack argument (the record to match).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern int32_t input_device_count;    // 0x006b1844
extern input_device input_devices[8]; // 0x006b1868

// Finds the index of the registered input device whose device_key matches record's (instance
// number first, then the 4-dword product GUID), or 0xffffffff if none match.
uint32_t input_device_find_index_by_guid(controls_gamepad_record *record)
{
    int32_t i;
    int32_t k;
    uint8_t match;

    for (i = 0; i < input_device_count; i++) {
        if (record->device_key[4] == input_devices[i].record.device_key[4]) {
            match = 1;
            for (k = 0; k < 4; k++) {
                if (record->device_key[k] != input_devices[i].record.device_key[k]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                return (uint32_t)(uint16_t)i;
            }
        }
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4916e0):

uint __cdecl input_device_find_index_by_guid(int param_1)

{
  int iVar1;
  int *piVar2;
  uint uVar3;
  int *piVar4;
  int *piVar5;
  bool bVar6;

  uVar3 = 0;
  if (0 < DAT_006b1844) {
    piVar2 = &DAT_006b1a74;
    do {
      if (*(int *)(param_1 + 0x21c) == piVar2[4]) {
        iVar1 = 4;
        bVar6 = true;
        piVar4 = (int *)(param_1 + 0x20c);
        piVar5 = piVar2;
        do {
          if (iVar1 == 0) break;
          iVar1 = iVar1 + -1;
          bVar6 = *piVar4 == *piVar5;
          piVar4 = piVar4 + 1;
          piVar5 = piVar5 + 1;
        } while (bVar6);
        if (bVar6) {
          return uVar3 & 0xffff;
        }
      }
      uVar3 = uVar3 + 1;
      piVar2 = piVar2 + 0x90;
    } while ((int)uVar3 < DAT_006b1844);
  }
  return 0xffffffff;
}
#endif
