// sound_driver_set_paused  (Ghidra: missed_546fe0; 0 callers in this module -- reached only
// through the DirectSound driver's set_paused vtable slot, sound_driver.set_paused 0x28)
// address 0x546fe0, size 131 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md driver slot table "0x28 0x546fe0" and
//   types/sound.h sound_driver.set_paused comment; directsound_channel.state/gain/buffer/
//   source_crosslap (0x000/0x03c/0x670/0x090, types/sound.h) match by offset; IDirectSoundBuffer
//   vtable slot 0x30 is Play (per the module header's own vtable-slot note); reuses
//   sound_channel_stream_update (0x5478c0, this module, AX -> channel_index, stack -> unused).
// register convention: stack -> paused (a byte pushed as a caller-sized stack word, per the
//   sound_driver.set_paused(uint8_t paused) prototype).
// blam-cc: stack -> paused
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t directsound_paused;          // 0x00746118
extern int16_t directsound_channel_count;   // 0x00725428
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430


// blam-cc: stack -> paused
// Only acts when the paused state actually changes. Going from paused to unpaused, every
// occupied hardware channel has its cached gain zeroed and, if it still has a live buffer, is
// restarted looping (Play, flags = DSBPLAY_LOOPING), then its streaming state is refreshed.
// Going the other way (or any other transition) just records the new state.
void sound_driver_set_paused(uint8_t paused)
{
    int16_t i;

    if (paused != directsound_paused) {
        if (paused == 0) {
            for (i = 0; i < directsound_channel_count; i++) {
                directsound_channel *channel = &directsound_channels[i];

                if (channel->state != _directsound_channel_idle) {
                    channel->gain = 0.0f;
                    if (channel->buffer != 0) {
                        ((directsound_buffer_play_proc)(*(void ***)channel->buffer)[0x30 / 4])(channel->buffer, 0, 0, 1);
                    }
                    sound_channel_stream_update(i, channel->source_crosslap);
                }
            }
        }
        directsound_paused = paused;
    }
}

#if 0
Original Ghidra decompilation (0x546fe0):

void missed_546fe0(char param_1)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  short sVar4;

  cVar2 = DAT_00746118;
  if (((param_1 != DAT_00746118) && (cVar2 = param_1, param_1 == '\0')) &&
     (sVar4 = 0, cVar2 = param_1, 0 < DAT_00725428)) {
    do {
      iVar3 = (int)sVar4;
      if ((&DAT_00725430)[iVar3 * 0x33c] != 0) {
        (&DAT_0072546c)[iVar3 * 0x19e] = 0;
        if ((iVar3 * 0x678 != -0x725430) &&
           (piVar1 = (int *)(&DAT_00725aa0)[iVar3 * 0x19e], piVar1 != (int *)0x0)) {
          (**(code **)(*piVar1 + 0x30))(piVar1,0,0,1);
        }
        sound_channel_stream_update((&DAT_007254c0)[iVar3 * 0x678]);
      }
      sVar4 = sVar4 + 1;
      cVar2 = param_1;
    } while (sVar4 < DAT_00725428);
  }
  DAT_00746118 = cVar2;
  return;
}

Disassembly (0x546fe0..0x547062; phase-4 review):

0x546fe0: mov al, byte ptr [esp + 4]
0x546fe4: cmp al, byte ptr [0x746118]
0x546fea: je 0x547062
0x546fec: test al, al
0x546fee: jne 0x54705d
0x546ff0: push edi
0x546ff1: xor edi, edi
0x546ff3: cmp word ptr [0x725428], di
0x546ffa: jle 0x54705c
0x546ffc: push esi
0x546ffd: lea ecx, [ecx]
0x547000: movsx esi, di
0x547003: imul esi, esi, 0x678
0x547009: cmp word ptr [esi + 0x725430], 0
0x547011: lea esi, [esi + 0x725430]
0x547017: je 0x54704d
0x547019: test esi, esi
0x54701b: mov dword ptr [esi + 0x3c], 0
0x547022: je 0x54703a
0x547024: mov eax, dword ptr [esi + 0x670]
0x54702a: test eax, eax
0x54702c: je 0x54703a
0x54702e: mov ecx, dword ptr [eax]
0x547030: push 1
0x547032: push 0
0x547034: push 0
0x547036: push eax
0x547037: call dword ptr [ecx + 0x30]
0x54703a: xor edx, edx
0x54703c: mov dl, byte ptr [esi + 0x90]
0x547042: mov eax, edi
0x547044: push edx
0x547045: call 0x5478c0
0x54704a: add esp, 4
0x54704d: inc edi
0x54704e: cmp di, word ptr [0x725428]
0x547055: jl 0x547000
0x547057: mov al, byte ptr [esp + 0xc]
0x54705b: pop esi
0x54705c: pop edi
0x54705d: mov byte ptr [0x746118], al
0x547062: ret
#endif
