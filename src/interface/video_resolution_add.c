// video_resolution_add  (Ghidra: already named)
// address 0x4badc0, size 189 bytes
// name confidence: 0.85   rewrite confidence: 0.9
// evidence: rewritten from objdump 0x4badc0..0x4bae7c in the phase-4 review (the first
// rewrite lost the EDX destination of the formatter and wrote the name terminator through a
// separate selected_rate field). Finds the entry with this width and height or appends one
// (at most 0x20), writes width, height and the name (string_format_wide_va, EDX name,
// L"%d x %d" 0x0066b164) and forces name[15] (+0x26) to 0, then records the refresh rate
// unless it is already listed or the entry has 8.
// register convention: EAX height; stack width, refresh rate.
//   // blam-cc: height -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_resolution_count;           // 0x007196cc

extern void string_format_wide_va(uint16_t *dest, const uint16_t *format, ...); // 0x557930, blam-cc: EDX dest

// blam-cc: height -> EAX
void video_resolution_add(int32_t height, int32_t width, int32_t refresh_rate)
{
    static const uint16_t name_format[8] = {'%', 'd', ' ', 'x', ' ', '%', 'd', 0}; // 0x0066b164
    video_resolution *entry;
    int32_t index = -1;
    int32_t i;
    uint32_t j;

    for (i = 0; i < video_resolution_count; i++) {
        if (video_resolutions[i].width == width && video_resolutions[i].height == height) {
            index = i;
            break;
        }
    }
    if (index == -1) {
        if (video_resolution_count >= 0x20) {
            return;
        }
        index = video_resolution_count++;
    }

    entry = &video_resolutions[index];
    entry->width = width;
    entry->height = height;
    string_format_wide_va(entry->name, name_format, width, height);
    entry->name[15] = 0;
    for (j = 0; j < entry->refresh_rate_count; j++) {
        if (entry->refresh_rates[j] == refresh_rate) {
            return;
        }
    }
    if (entry->refresh_rate_count < 8) {
        entry->refresh_rates[entry->refresh_rate_count] = refresh_rate;
        entry->refresh_rate_count++;
    }
}

#if 0
Original Ghidra decompilation (0x4badc0):

void video_resolution_add(int param_1,int param_2)

{
  uint uVar1;
  int in_EAX;
  int iVar2;
  uint uVar3;
  int *piVar4;
  int iVar5;

  iVar2 = 0;
  iVar5 = -1;
  if (0 < DAT_007196cc) {
    piVar4 = (int *)&DAT_006b6694;
    do {
      if ((piVar4[-1] == param_1) && (iVar5 = iVar2, *piVar4 == in_EAX)) break;
      iVar2 = iVar2 + 1;
      piVar4 = piVar4 + 0x13;
      iVar5 = -1;
    } while (iVar2 < DAT_007196cc);
  }
  if (iVar5 == -1) {
    if (0x1f < DAT_007196cc) {
      return;
    }
    iVar5 = DAT_007196cc;
    DAT_007196cc = DAT_007196cc + 1;
  }
  *(int *)(&DAT_006b6690 + iVar5 * 0x26) = param_1;
  *(int *)(&DAT_006b6694 + iVar5 * 0x26) = in_EAX;
  string_format_wide_va(L"%d x %d",param_1);
  uVar1 = (&DAT_006b66b8)[iVar5 * 0x13];
  uVar3 = 0;
  *(undefined2 *)(&DAT_006b66b6 + iVar5 * 0x4c) = 0;
  if (uVar1 != 0) {
    piVar4 = (int *)(&DAT_006b66bc + iVar5 * 0x26);
    do {
      if (*piVar4 == param_2) {
        return;
      }
      uVar3 = uVar3 + 1;
      piVar4 = piVar4 + 1;
    } while (uVar3 < (uint)(&DAT_006b66b8)[iVar5 * 0x13]);
  }
  if (uVar1 < 8) {
    *(int *)(&DAT_006b66bc + (iVar5 * 0x13 + uVar1) * 2) = param_2;
    (&DAT_006b66b8)[iVar5 * 0x13] = (&DAT_006b66b8)[iVar5 * 0x13] + 1;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
