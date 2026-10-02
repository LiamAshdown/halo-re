// sound_impulse_time  (Ghidra: FUN_00543fc0; earlier draft name sound_scripted_ticks_remaining)
// address 0x543fc0, size 56 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Returns the number of ticks remaining until a
//   looping-sound datum's next scheduled refresh, clamped to zero."; reads the same Sound tag
//   scripting_time field (0x90) that src/sound/sound_impulse_start.c writes, and the
//   same game_time_globals.game_time (0x006f1d6c + 0xc) it uses to compute that deadline.
//   Phase-4 review: hs sound_impulse_time <sound> returns exactly this (ticks until the
//   scripted impulse set by sound_impulse_start 0x543e10 ends, 0 when none); body matches the
//   disassembly (0x543fc0..0x543ff7).
// register convention: Sound tag handle in ECX (in_ECX).
// blam-cc: ECX -> sound_tag_handle

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;  // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c

// blam-cc: ECX -> sound_tag_handle
// Ticks remaining until `sound_tag_handle`'s scripting_time deadline (0 if it has none set, or
// if the deadline has already passed).
int32_t sound_impulse_time(datum_index sound_tag_handle)
{
    int32_t remaining = 0;

    if (sound_tag_handle != k_datum_index_none) {
        Sound *tag = (Sound *)tag_instances[sound_tag_handle & 0xffff].data;

        if ((int32_t)tag->scripting_time != -1) {
            remaining = (int32_t)tag->scripting_time - game_time->game_time;
            if (remaining < 1) {
                remaining = 0;
            }
        }
    }

    return remaining;
}

#if 0
Original Ghidra decompilation (0x543fc0):

uint FUN_00543fc0(void)

{
  int iVar1;
  uint uVar2;
  uint in_ECX;

  uVar2 = 0;
  if ((in_ECX != 0xffffffff) &&
     (iVar1 = *(int *)(*(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x90), iVar1 != -1
     )) {
    uVar2 = iVar1 - *(int *)(DAT_006f1d6c + 0xc);
    uVar2 = ((int)uVar2 < 1) - 1 & uVar2;
  }
  return uVar2;
}

Disassembly (0x543fc0..0x543ff8, capstone; phase-4 review):

0x543fc0: xor eax, eax
0x543fc2: cmp ecx, -1
0x543fc5: je 0x543ff7
0x543fc7: mov edx, dword ptr [0x87bc14]
0x543fcd: and ecx, 0xffff
0x543fd3: shl ecx, 5
0x543fd6: mov ecx, dword ptr [ecx + edx + 0x14]
0x543fda: mov ecx, dword ptr [ecx + 0x90]
0x543fe0: cmp ecx, -1
0x543fe3: je 0x543ff7
0x543fe5: mov eax, dword ptr [0x6f1d6c]
0x543fea: sub ecx, dword ptr [eax + 0xc]
0x543fed: xor eax, eax
0x543fef: test ecx, ecx
0x543ff1: setle al
0x543ff4: dec eax
0x543ff5: and eax, ecx
0x543ff7: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
