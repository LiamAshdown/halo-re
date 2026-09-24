// video_resolution_list_build  (Ghidra: already named, __cdecl)
// address 0x4bad40, size 117 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: types/interface.h video_resolution / video_resolutions[0x20] / video_resolution_count.
// register convention: __cdecl, no arguments. Verified against objdump 0x4bad40..0x4badb4
// in the phase-4 review; the enumeration takes the D3D format 0x16 in EBX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include <string.h>

extern video_resolution video_resolutions[0x20]; // 0x006b6690
extern int32_t video_resolution_count;            // 0x007196cc

extern void video_display_modes_enumerate(uint32_t format); // 0x4baba0, blam-cc: EBX format
extern int video_resolution_compare(const video_resolution *a, const video_resolution *b); // 0x4bab50, this module
extern uint32_t video_refresh_rate_compare(const uint32_t *a, const uint32_t *b); // 0x4bab80, this module
extern void _qsort(void *base, uint32_t count, uint32_t size, int (__cdecl *compare)());

void __cdecl video_resolution_list_build(void)
{
    int i;

    video_resolution_count = 0;
    memset(video_resolutions, 0, sizeof(video_resolutions));

    video_display_modes_enumerate(0x16); // D3DFMT_X8R8G8B8, EBX
    _qsort(video_resolutions, (uint32_t)video_resolution_count, sizeof(video_resolution),
          (int (__cdecl *)())video_resolution_compare);

    for (i = 0; i < video_resolution_count; i++) {
        _qsort(video_resolutions[i].refresh_rates, video_resolutions[i].refresh_rate_count,
              sizeof(int32_t), (int (__cdecl *)())video_refresh_rate_compare);
    }
}

#if 0
Original Ghidra decompilation (0x4bad40):

void __cdecl video_resolution_list_build(void)

{
  int iVar1;
  undefined2 *_Base;
  undefined4 *puVar2;

  DAT_007196cc = 0;
  puVar2 = (undefined4 *)&DAT_006b6690;
  for (iVar1 = 0x260; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  video_display_modes_enumerate();
  _qsort(&DAT_006b6690,DAT_007196cc,0x4c,video_resolution_compare);
  iVar1 = 0;
  if (0 < (int)DAT_007196cc) {
    _Base = &DAT_006b66bc;
    do {
      _qsort(_Base,*(size_t *)(_Base + -2),4,video_refresh_rate_compare);
      iVar1 = iVar1 + 1;
      _Base = _Base + 0x26;
    } while (iVar1 < (int)DAT_007196cc);
  }
  return;
}
#endif
