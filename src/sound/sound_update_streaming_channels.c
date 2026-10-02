// sound_update_streaming_channels  (Ghidra: FUN_00546b40)
// address 0x546b40, size 59 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Runs the streaming buffer-fill update for
//   every channel currently marked as streaming."; directsound_channel.streaming (0x009,
//   types/sound.h) tested per channel; forwards to FUN_005478c0 (0x5478c0, "Determines how much
//   ring-buffer space a streaming channel needs refilled and triggers the fill").
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// The loop bound 0x0074602a is directsound_first_channel_of_type[1] (types/sound.h,
//   0x00746028 + 2); since channels are allocated contiguously by type in
//   src/sound/sound_channel_create.c's caller, this doubles as "count of type-0 (mono 3D)
//   channels"; that is the only block that can be streaming, because sound_channel_reset keeps
//   only 3D weapon-fire channels streaming. FUN_005478c0's channel-index argument is elided at this call site
//   (`FUN_005478c0(0)`); passed here as the loop index, matching the register-elision pattern
//   documented throughout this module.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int16_t directsound_first_channel_of_type[4]; // 0x00746028
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern void sound_channel_stream_update(int16_t channel_index, uint8_t unused); // 0x5478c0, blam-cc: AX, stack

// Runs the streaming buffer-fill update (sound_channel_stream_update) for every channel of type 0
// currently marked as streaming (the mono 3D block: the only channels whose reset keeps them
// streaming, see sound_channel_reset).
void sound_update_streaming_channels(void)
{
    int16_t i;

    for (i = 0; i < directsound_first_channel_of_type[1]; i++) {
        if (directsound_channels[i].streaming != 0) {
            sound_channel_stream_update(i, 0); // pushes a 0 the callee never reads
        }
    }
}

#if 0
Original Ghidra decompilation (0x546b40):

void FUN_00546b40(void)

{
  short sVar1;

  sVar1 = 0;
  if (0 < DAT_0074602a) {
    do {
      if ((&DAT_00725439)[sVar1 * 0x678] != '\0') {
        FUN_005478c0(0);
      }
      sVar1 = sVar1 + 1;
    } while (sVar1 < DAT_0074602a);
  }
  return;
}

Disassembly (0x546b40..0x546b7b, capstone; phase-4 review):

0x546b40: push esi
0x546b41: xor esi, esi
0x546b43: cmp word ptr [0x74602a], si
0x546b4a: jle 0x546b79
0x546b4c: lea esp, [esp]
0x546b50: movsx eax, si
0x546b53: imul eax, eax, 0x678
0x546b59: mov cl, byte ptr [eax + 0x725439]
0x546b5f: test cl, cl
0x546b61: je 0x546b6f
0x546b63: push 0
0x546b65: mov eax, esi
0x546b67: call 0x5478c0
0x546b6c: add esp, 4
0x546b6f: inc esi
0x546b70: cmp si, word ptr [0x74602a]
0x546b77: jl 0x546b50
0x546b79: pop esi
0x546b7a: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
