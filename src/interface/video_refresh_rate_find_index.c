// video_refresh_rate_find_index  (Ghidra: already named)
// address 0x4bae80, size 59 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// phase-4 review: checked against objdump 0x4bae80..0x4baeba (ECX resolution, EDI rate, -1 when absent).
// evidence: types/interface.h video_resolution; src/interface/README.md: "Finds the index
// of a given refresh-rate value within a specific resolution's rate list, both passed in
// registers; returns -1 if not found."
// register convention: ECX resolution index, EDI refresh rate value (both registers, per
// Ghidra's unresolved in_ECX/unaff_EDI).
//   // blam-cc: resolution_index -> ECX, refresh_rate -> EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"

extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_resolution_count;            // 0x007196cc

// blam-cc: resolution_index -> ECX, refresh_rate -> EDI
uint32_t video_refresh_rate_find_index(int32_t resolution_index, int32_t refresh_rate)
{
    uint32_t i;

    if (resolution_index < 0 || resolution_index >= video_resolution_count) {
        return 0xffffffffu;
    }
    if (video_resolutions[resolution_index].refresh_rate_count == 0) {
        return 0xffffffffu;
    }
    for (i = 0; i < (uint32_t)video_resolutions[resolution_index].refresh_rate_count; i++) {
        if (video_resolutions[resolution_index].refresh_rates[i] == refresh_rate) {
            return i;
        }
    }
    return 0xffffffffu;
}

#if 0
Original Ghidra decompilation (0x4bae80):

uint video_refresh_rate_find_index(void)

{
  int in_ECX;
  int *piVar1;
  uint uVar2;
  int unaff_EDI;

  if ((-1 < in_ECX) && (in_ECX < DAT_007196cc)) {
    uVar2 = 0xffffffff;
    if ((&DAT_006b66b8)[in_ECX * 0x13] != 0) {
      piVar1 = (int *)(&DAT_006b66bc + in_ECX * 0x26);
      uVar2 = 0;
      while (*piVar1 != unaff_EDI) {
        uVar2 = uVar2 + 1;
        piVar1 = piVar1 + 1;
        if ((uint)(&DAT_006b66b8)[in_ECX * 0x13] <= uVar2) {
          return 0xffffffff;
        }
      }
    }
    return uVar2;
  }
  return 0xffffffff;
}
#endif
