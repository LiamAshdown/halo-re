// sound_driver_stop_all  (Ghidra: missed_546fa0; 0 callers in this module -- reached only through
// the DirectSound driver's stop_all vtable slot, sound_driver.stop_all 0x2c)
// address 0x546fa0, size 51 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: out/phase4/sound_types_notes.md driver slot table "0x2c 0x546fa0" and
//   types/sound.h sound_driver.stop_all comment; loops directsound_channel_count and, for each
//   hardware channel, calls sound_channel_reset (this module, 0x547f60, AX -> channel_index)
//   then forces directsound_channel.free (0x008, types/sound.h) back to 0 -- sound_channel_reset
//   itself sets free = 1 at the end, so this overrides it, leaving every channel reset but not
//   yet reusable until sound_channel_create/whatever re-marks it.
// register convention: plain __cdecl, no parameters; the loop index is passed to
//   sound_channel_reset in EAX (matches that function's own AX convention).
// blam-cc: void
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern int16_t directsound_channel_count;   // 0x00725428
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern void sound_channel_reset(int16_t channel_index); // 0x547f60, blam-cc: AX

// blam-cc: void
// Resets every hardware DirectSound channel (stopping its buffer and clearing its queued
// sources) and marks each one not-free, overriding sound_channel_reset's own free = 1.
void sound_driver_stop_all(void)
{
    int16_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        sound_channel_reset(i);
        directsound_channels[i].free = 0;
    }
}

#if 0
Original Ghidra decompilation (0x546fa0):

void missed_546fa0(void)

{
  int iVar1;
  short sVar2;

  sVar2 = 0;
  if (0 < DAT_00725428) {
    do {
      sound_channel_reset();
      iVar1 = (int)sVar2;
      sVar2 = sVar2 + 1;
      (&DAT_00725438)[iVar1 * 0x678] = 0;
    } while (sVar2 < DAT_00725428);
  }
  return;
}

Disassembly (0x546fa0..0x546fd2; phase-4 review):

0x546fa0: push esi
0x546fa1: xor esi, esi
0x546fa3: cmp word ptr [0x725428], si
0x546faa: jle 0x546fd1
0x546fac: lea esp, [esp]
0x546fb0: mov eax, esi
0x546fb2: call 0x547f60
0x546fb7: movsx eax, si
0x546fba: imul eax, eax, 0x678
0x546fc0: inc esi
0x546fc1: mov byte ptr [eax + 0x725438], 0
0x546fc8: cmp si, word ptr [0x725428]
0x546fcf: jl 0x546fb0
0x546fd1: pop esi
0x546fd2: ret
#endif
