// sound_driver_channel_play  (Ghidra: missed_548380; 0 callers in this module -- reached only
// through the DirectSound driver's channel_play vtable slot, sound_driver.channel_play 0x18)
// address 0x548380, size 69 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md driver slot table "0x18 0x548380 -> 0x547c80" and
//   types/sound.h sound_driver.channel_play comment ("CL = crosslap; the third argument is not
//   read"); sound_channel_binding.hardware_channel_index (0x00, types/sound.h) matches
//   directsound_bindings by offset; reuses sound_channel_bind_hardware (0x5482e0, this module,
//   stack -> logical_channel_index) and sound_channel_queue_source (0x547c80, this module,
//   stack -> (channel_index, source, sound_class), CL -> crosslap).
// register convention: Ghidra's own parameter scan only found 4 stack words (channel_index,
//   source, unused, sound_class); the disassembly shows a 5th, `crosslap`, read straight into CL
//   for the tail call rather than pushed as its own dword -- matching the driver's own
//   channel_play(channel_index, source, unused, sound_class, crosslap) prototype exactly (see
//   types/sound.h). `unused` (the 3rd stack word) is never read.
// blam-cc: stack -> (channel_index, source, unused, sound_class, crosslap)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. Fixes against the Ghidra draft: recovers the 5th
// (crosslap) parameter that Ghidra's own signature dropped.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4

extern void sound_channel_bind_hardware(int16_t logical_channel_index); // 0x5482e0, blam-cc: stack
extern void sound_channel_queue_source(int16_t channel_index, SoundPermutation *source, int16_t sound_class,
    uint8_t crosslap); // 0x547c80, blam-cc: stack -> (channel_index, source, sound_class), CL -> crosslap

// blam-cc: stack -> (channel_index, source, unused, sound_class, crosslap)
// Binds a hardware channel to `channel_index` if it does not already have one, then queues
// `source` for playback on that hardware channel. `unused` is never read.
void sound_driver_channel_play(int16_t channel_index, SoundPermutation *source, int16_t unused, int16_t sound_class,
    uint8_t crosslap)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        sound_channel_bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        sound_channel_queue_source(directsound_bindings[channel_index].hardware_channel_index, source, sound_class,
            crosslap);
    }
}

#if 0
Original Ghidra decompilation (0x548380):

void missed_548380(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  if ((&DAT_007252e4)[(short)param_1 * 2] == -1) {
    sound_channel_bind_hardware(param_1);
  }
  if ((&DAT_007252e4)[(short)param_1 * 2] != -1) {
    sound_channel_queue_source((int)(short)(&DAT_007252e4)[(short)param_1 * 2],param_2,param_4);
  }
  return;
}

Disassembly (0x548380..0x5483c4; phase-4 review):

0x548380: mov eax, dword ptr [esp + 4]
0x548384: push esi
0x548385: movsx esi, ax
0x548388: cmp word ptr [esi*4 + 0x7252e4], 0xffff
0x548391: lea esi, [esi*4 + 0x7252e4]
0x548398: jne 0x5483a3
0x54839a: push eax
0x54839b: call 0x5482e0
0x5483a0: add esp, 4
0x5483a3: movsx esi, word ptr [esi]
0x5483a6: cmp si, 0xffff
0x5483aa: je 0x5483c3
0x5483ac: mov eax, dword ptr [esp + 0x14]
0x5483b0: mov ecx, dword ptr [esp + 0xc]
0x5483b4: push eax
0x5483b5: push ecx
0x5483b6: mov cl, byte ptr [esp + 0x20]
0x5483ba: push esi
0x5483bb: call 0x547c80
0x5483c0: add esp, 0xc
0x5483c3: pop esi
0x5483c4: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
