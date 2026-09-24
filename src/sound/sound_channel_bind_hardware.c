// sound_channel_bind_hardware  (Ghidra: FUN_005482e0)
// address 0x5482e0, size 150 bytes
// name confidence: 0.35   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Finds and assigns a free or reclaimable sound
//   channel of the required type to the given logical sound-class/id."; types/sound.h
//   sound_channel_binding (hardware_channel_index 0x00, channel_type 0x02),
//   directsound_first_channel_of_type (0x00746028), directsound_channel.type_flags/
//   sound_channel_index/free (0x038/0x002/0x008), sound_channel_type_flags[4] (0x0069f528).
//   Reuses src/sound/sound_channel_claim_if_finished.c (0x547ff0).
// register convention: logical channel index as the recognized stack parameter (param_1).
// blam-cc: stack -> logical_channel_index
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern sound_channel_binding directsound_bindings[k_maximum_sound_channels]; // 0x007252e4
extern int16_t directsound_first_channel_of_type[4]; // 0x00746028
extern int16_t directsound_channel_count;   // 0x00725428
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern uint16_t sound_channel_type_flag_table[4]; // 0x0069f528

extern uint32_t sound_channel_claim_if_finished(int16_t channel_index); // 0x547ff0

// blam-cc: stack -> logical_channel_index
// If `logical_channel_index` has no hardware channel bound yet, scans forward from that channel
// type's first slot for one that is unbound and either already marked available or just finished
// playing (sound_channel_claim_if_finished), stopping at the end of that type's block or the
// channel table. On success, binds the hardware channel back to this logical channel.
void sound_channel_bind_hardware(int16_t logical_channel_index)
{
    sound_channel_binding *binding = &directsound_bindings[logical_channel_index];
    int16_t candidate = directsound_first_channel_of_type[binding->channel_type];

    if (binding->hardware_channel_index == -1) {
        do {
            if (candidate >= directsound_channel_count ||
                directsound_channels[candidate].type_flags != sound_channel_type_flag_table[binding->channel_type]) {
                break;
            }

            if (directsound_channels[candidate].sound_channel_index == -1 &&
                (directsound_channels[candidate].free == 0 ||
                 sound_channel_claim_if_finished(candidate) != 0)) {
                binding->hardware_channel_index = candidate;
            }

            candidate++;
        } while (binding->hardware_channel_index == -1);

        if (binding->hardware_channel_index == -1) {
            return;
        }
    }

    directsound_channels[binding->hardware_channel_index].sound_channel_index = logical_channel_index;
}

#if 0
Original Ghidra decompilation (0x5482e0):

void FUN_005482e0(short param_1)

{
  short *psVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  int iVar5;
  short sVar6;

  iVar5 = (int)param_1;
  psVar1 = &DAT_007252e4 + iVar5 * 2;
  sVar6 = *(short *)(&DAT_00746028 + (short)(&DAT_007252e6)[iVar5 * 2] * 2);
  sVar3 = *psVar1;
  if (sVar3 == -1) {
    do {
      if ((DAT_00725428 <= sVar6) ||
         (iVar4 = sVar6 * 0x678,
         *(short *)(&DAT_00725468 + iVar4) != (&DAT_0069f528)[(short)(&DAT_007252e6)[iVar5 * 2]]))
      break;
      if ((*(short *)(&DAT_00725432 + iVar4) == -1) &&
         (((&DAT_00725438)[iVar4] == '\0' || (cVar2 = FUN_00547ff0(), cVar2 != '\0')))) {
        *psVar1 = sVar6;
      }
      sVar6 = sVar6 + 1;
    } while (*psVar1 == -1);
    sVar3 = *psVar1;
    if (sVar3 == -1) {
      return;
    }
  }
  *(short *)(&DAT_00725432 + sVar3 * 0x678) = param_1;
  return;
}

Disassembly (0x5482e0..0x548376, capstone; phase-4 review):

0x5482e0: push ebp
0x5482e1: mov bp, word ptr [esp + 8]
0x5482e6: push esi
0x5482e7: movsx esi, bp
0x5482ea: movsx eax, word ptr [esi*4 + 0x7252e6]
0x5482f2: lea esi, [esi*4 + 0x7252e4]
0x5482f9: push edi
0x5482fa: mov di, word ptr [eax*2 + 0x746028]
0x548302: mov ax, word ptr [esi]
0x548305: cmp ax, 0xffff
0x548309: jne 0x548365
0x54830b: jmp 0x548310
0x54830d: lea ecx, [ecx]
0x548310: cmp di, word ptr [0x725428]
0x548317: jge 0x54835c
0x548319: movsx ecx, word ptr [esi + 2]
0x54831d: movsx eax, di
0x548320: imul eax, eax, 0x678
0x548326: add eax, 0x725430
0x54832b: mov dx, word ptr [eax + 0x38]
0x54832f: cmp dx, word ptr [ecx*2 + 0x69f528]
0x548337: jne 0x54835c
0x548339: cmp word ptr [eax + 2], -1
0x54833e: jne 0x548355
0x548340: mov cl, byte ptr [eax + 8]
0x548343: test cl, cl
0x548345: je 0x548352
0x548347: mov eax, edi
0x548349: call 0x547ff0
0x54834e: test al, al
0x548350: je 0x548355
0x548352: mov word ptr [esi], di
0x548355: inc edi
0x548356: cmp word ptr [esi], -1
0x54835a: je 0x548310
0x54835c: mov ax, word ptr [esi]
0x54835f: cmp ax, 0xffff
0x548363: je 0x548375
0x548365: movsx eax, ax
0x548368: imul eax, eax, 0x678
0x54836e: mov word ptr [eax + 0x725432], bp
0x548375: pop edi
#endif
