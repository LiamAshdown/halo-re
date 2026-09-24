// sound_effects_object_apply_all_channels  (Ghidra: sound_effects_object_apply_all_channels,
// already named, __cdecl)
// address 0x551480, size 65 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Iterates all sound channels and invokes the effects
// object's per-channel apply method for each channel flagged as EAX-enabled."; iterates
// directsound_channels (types/sound.h, 0x00725430, stride 0x678) testing type_flags
// (0x725430+0x38 == 0x725468) bit 0 (_sound_channel_3d_bit). Calls the same vtable+8 slot as
// sound_effects_object_initialize_channel.c (0x551460); see that file's header for the
// initialize_channel-vs-"apply" naming discrepancy this rewrite does not resolve.
// register convention: __cdecl, no parameters.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern int16_t directsound_channel_count;                // 0x00725428
extern directsound_channel directsound_channels[k_maximum_sound_channels];     // 0x00725430
extern sound_effect_object *global_sound_effect_object;  // 0x00721f24

int __cdecl sound_effects_object_apply_all_channels(void)
{
    int16_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        if (directsound_channels[i].type_flags & _sound_channel_3d_bit) {
            global_sound_effect_object->vtable->initialize_channel(global_sound_effect_object, i);
        }
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x551480):

int __cdecl sound_effects_object_apply_all_channels(void)

{
  short sVar1;

  sVar1 = 0;
  if (0 < DAT_00725428) {
    do {
      if (((&DAT_00725468)[sVar1 * 0x678] & 1) != 0) {
        (**(code **)(*DAT_00721f24 + 8))((int)sVar1);
      }
      sVar1 = sVar1 + 1;
    } while (sVar1 < DAT_00725428);
  }
  return 1;
}

Disassembly (0x551480..0x5514c1, capstone; phase-4 review):

0x551480: push esi
0x551481: xor esi, esi
0x551483: cmp word ptr [0x725428], si
0x55148a: jle 0x5514ba
0x55148c: lea esp, [esp]
0x551490: movsx eax, si
0x551493: mov ecx, eax
0x551495: imul ecx, ecx, 0x678
0x55149b: test byte ptr [ecx + 0x725468], 1
0x5514a2: je 0x5514b0
0x5514a4: mov ecx, dword ptr [0x721f24]
0x5514aa: mov edx, dword ptr [ecx]
0x5514ac: push eax
0x5514ad: call dword ptr [edx + 8]
0x5514b0: inc esi
0x5514b1: cmp si, word ptr [0x725428]
0x5514b8: jl 0x551490
0x5514ba: mov eax, 1
0x5514bf: pop esi
0x5514c0: ret 
#endif
