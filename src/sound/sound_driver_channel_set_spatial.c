// sound_driver_channel_set_spatial  (Ghidra: missed_548470; 0 callers in this module -- reached
// only through the DirectSound driver's channel_set_spatial vtable slot,
// sound_driver.channel_set_spatial 0x30)
// address 0x548470, size 88 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md driver slot table "0x30 0x548470 -> 0x5472d0" and
//   types/sound.h sound_driver.channel_set_spatial comment (its 7-parameter signature matches
//   this function's own 7 stack parameters exactly); reuses sound_channel_bind_hardware
//   (0x5482e0, this module) and sound_channel_set_spatial (0x5472d0, this module, whose own
//   documented convention is "stack -> (channel_index, obstruction, occlusion, underwater,
//   sound_class), BL -> spatialized, EDI -> spatial" -- this wrapper loads `spatialized` into BL
//   and `spatial` into EDI before the call, matching that convention).
// register convention: all 7 parameters are this function's own recognized stack parameters
//   (unlike its callee, which takes two of them in registers).
// blam-cc: stack -> (channel_index, spatialized, spatial, obstruction, occlusion, underwater,
// sound_class)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4

extern void sound_channel_bind_hardware(int16_t logical_channel_index); // 0x5482e0, blam-cc: stack
extern void sound_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial,
    float obstruction, float occlusion, uint8_t underwater, int16_t sound_class); // 0x5472d0,
    // blam-cc: stack -> (channel_index, obstruction, occlusion, underwater, sound_class),
    // BL -> spatialized, EDI -> spatial

// blam-cc: stack -> (channel_index, spatialized, spatial, obstruction, occlusion, underwater,
// sound_class)
// Binds a hardware channel to `channel_index` if it does not already have one, then forwards the
// spatial parameters to that hardware channel.
void sound_driver_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial,
    float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    if (directsound_bindings[channel_index].hardware_channel_index == -1) {
        sound_channel_bind_hardware(channel_index);
    }
    if (directsound_bindings[channel_index].hardware_channel_index != -1) {
        sound_channel_set_spatial(directsound_bindings[channel_index].hardware_channel_index, spatialized, spatial,
            obstruction, occlusion, underwater, sound_class);
    }
}

#if 0
Original Ghidra decompilation (0x548470):

void missed_548470(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                  undefined4 param_5,undefined4 param_6,short param_7)

{
  short sVar1;

  if ((&DAT_007252e4)[(short)param_1 * 2] == -1) {
    sound_channel_bind_hardware(param_1);
  }
  sVar1 = (&DAT_007252e4)[(short)param_1 * 2];
  if (sVar1 != -1) {
    sound_channel_set_spatial
              (CONCAT22((short)((uint)(&DAT_007252e4 + (short)param_1 * 2) >> 0x10),sVar1),param_4,
               param_5,param_6,(int)param_7);
  }
  return;
}

Disassembly (0x548470..0x5484c7; phase-4 review):

0x548470: mov eax, dword ptr [esp + 4]
0x548474: push esi
0x548475: movsx esi, ax
0x548478: cmp word ptr [esi*4 + 0x7252e4], 0xffff
0x548481: lea esi, [esi*4 + 0x7252e4]
0x548488: jne 0x548493
0x54848a: push eax
0x54848b: call 0x5482e0
0x548490: add esp, 4
0x548493: mov si, word ptr [esi]
0x548496: cmp si, 0xffff
0x54849a: je 0x5484c6
0x54849c: movsx eax, word ptr [esp + 0x20]
0x5484a1: mov ecx, dword ptr [esp + 0x1c]
0x5484a5: mov edx, dword ptr [esp + 0x18]
0x5484a9: push ebx
0x5484aa: mov bl, byte ptr [esp + 0x10]
0x5484ae: push edi
0x5484af: mov edi, dword ptr [esp + 0x18]
0x5484b3: push eax
0x5484b4: mov eax, dword ptr [esp + 0x20]
0x5484b8: push ecx
0x5484b9: push edx
0x5484ba: push eax
0x5484bb: push esi
0x5484bc: call 0x5472d0
0x5484c1: add esp, 0x14
0x5484c4: pop edi
0x5484c5: pop ebx
0x5484c6: pop esi
0x5484c7: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
