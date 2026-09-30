// sound_find_lowest_priority_channel  (Ghidra: FUN_0054c440, still unnamed there)
// address 0x54c440, size 410 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Finds the lowest-priority currently-playing channel
// eligible to be reused by a new sound instance."; the type-flags test matches every bit of
// types/sound.h's sound_channel_type_flags documentation (3d/stereo/44khz/compressed), confirmed
// bit by bit against Sound.sample_rate/channel_count/format and the candidate's location.type.
// register convention: sound handle as the recognized parameter (param_1).

// Phase-4 review (disassembly appended below): the location argument of
//   sound_location_distance_squared is ECX = &sound.location of the same sound whose
//   listener_index is pushed; the first priority test is compare(EAX = occupant, ECX =
//   candidate, candidate distance) and the second compare(EAX = occupant, ECX = current best,
//   best distance) -- the draft had the handles swapped and measured the best distance from the
//   candidate's location.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t sound_channel_count; // 0x007252b4
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60


// Scans every logical channel for one whose type flags match `sound_handle`'s spatialization,
// sample rate, channel count and format needs: returns the first free one immediately, otherwise
// the occupied one whose sound the candidate outranks by the widest margin (lowest class
// priority, then farthest), or -1.
int16_t sound_find_lowest_priority_channel(datum_index sound_handle)
{
    sound *candidate;
    Sound *definition;
    int16_t channel_index;
    int16_t best_channel;
    float candidate_distance_squared;
    float best_distance_squared = 0.0f;
    uint16_t type_flags;
    datum_index best_handle = k_datum_index_none;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)tag_instances[candidate->definition_index & 0xffff].data;
    best_channel = -1;
    candidate_distance_squared = sound_location_distance_squared(candidate->listener_index, &candidate->location);

    for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
        type_flags = sound_channels[channel_index].type_flags;

        if (((type_flags & _sound_channel_stereo_bit) != 0 ||
             (uint32_t)(!(type_flags & _sound_channel_3d_bit)) == (uint32_t)(candidate->location.type == _sound_location_none)) &&
            (((type_flags >> 2) & 1) == (uint16_t)definition->sample_rate &&
             (uint32_t)(!((type_flags >> 1) & 1)) == (uint32_t)(definition->channel_count == 0) &&
             (uint32_t)(!((type_flags >> 3) & 1)) == (uint32_t)(definition->format == 0))) {
            if (sound_channels[channel_index].sound_index == 0xffffffff) {
                return channel_index;
            }
            // sound_compare_priority(EAX = a, ECX = b, stack = d) answers "does b outrank a":
            // b's class priority is higher, or equal and a is farther than d
            if (sound_compare_priority(sound_channels[channel_index].sound_index, sound_handle, candidate_distance_squared) &&
                (best_channel == -1 ||
                 sound_compare_priority(sound_channels[channel_index].sound_index, best_handle, best_distance_squared))) {
                sound *occupant = (sound *)((uint8_t *)sound_data->data +
                    (sound_channels[channel_index].sound_index & 0xffff) * sizeof(sound));
                best_handle = sound_channels[channel_index].sound_index;
                best_channel = channel_index;
                best_distance_squared = sound_location_distance_squared(occupant->listener_index, &occupant->location);
            }
        }
    }
    return best_channel;
}

#if 0
Original Ghidra decompilation (0x54c440):

short FUN_0054c440(uint param_1)

{
  short sVar1;
  char cVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  short extraout_DX;
  short sVar6;
  float10 fVar7;
  float10 fVar8;
  float local_4;

  iVar3 = (param_1 & 0xffff) * 0xb0;
  iVar4 = iVar3 + *(int *)(DAT_007252c0 + 0x34);
  iVar3 = *(int *)((*(uint *)(iVar3 + 8 + *(int *)(DAT_007252c0 + 0x34)) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  sVar1 = -1;
  fVar7 = (float10)FUN_0054bbd0((int)*(short *)(iVar4 + 6));
  sVar6 = 0;
  if (DAT_007252b4 < 1) {
    return extraout_DX;
  }
  do {
    uVar5 = (uint)(short)(&DAT_00724a64)[sVar6 * 0xc];
    if (((((&DAT_00724a64)[sVar6 * 0xc] & 2) != 0) ||
        ((~uVar5 & 1) == (uint)(*(short *)(iVar4 + 0x14) == 0))) &&
       (((ushort)(uVar5 >> 2) & 1) == *(ushort *)(iVar3 + 6) &&
        ((~(uVar5 >> 1) & 1) == (uint)(*(short *)(iVar3 + 0x6c) == 0) &&
        (~(uVar5 >> 3) & 1) == (uint)(*(short *)(iVar3 + 0x6e) == 0)))) {
      if ((&DAT_00724a60)[sVar6 * 6] == 0xffffffff) {
        return sVar6;
      }
      cVar2 = FUN_0054c6b0((float)fVar7);
      if ((cVar2 != '\0') && ((sVar1 == -1 || (cVar2 = FUN_0054c6b0(local_4), cVar2 != '\0')))) {
        fVar8 = (float10)FUN_0054bbd0((int)*(short *)(((&DAT_00724a60)[sVar6 * 6] & 0xffff) * 0xb0 +
                                                      *(int *)(DAT_007252c0 + 0x34) + 6));
        local_4 = (float)fVar8;
        sVar1 = sVar6;
      }
    }
    sVar6 = sVar6 + 1;
  } while (sVar6 < DAT_007252b4);
  return sVar1;
}

Disassembly (0x54c440..0x54c5da, capstone; phase-4 review):

0x54c440: sub esp, 0x18
0x54c443: mov eax, dword ptr [esp + 0x1c]
0x54c447: mov ecx, dword ptr [0x7252c0]
0x54c44d: and eax, 0xffff
0x54c452: imul eax, eax, 0xb0
0x54c458: push ebp
0x54c459: mov ebp, dword ptr [ecx + 0x34]
0x54c45c: mov ecx, dword ptr [eax + ebp + 8]
0x54c460: add eax, ebp
0x54c462: and ecx, 0xffff
0x54c468: push esi
0x54c469: mov esi, dword ptr [0x87bc14]
0x54c46f: shl ecx, 5
0x54c472: mov ebp, dword ptr [ecx + esi + 0x14]
0x54c476: lea ecx, [eax + 0x14]
0x54c479: movsx eax, word ptr [eax + 6]
0x54c47d: push edi
0x54c47e: or edx, 0xffffffff
0x54c481: push eax
0x54c482: mov dword ptr [esp + 0x14], edx
0x54c486: mov dword ptr [esp + 0x18], edx
0x54c48a: mov dword ptr [esp + 0x1c], ecx
0x54c48e: call 0x54bbd0
0x54c493: xor edi, edi
0x54c495: add esp, 4
0x54c498: fstp dword ptr [esp + 0x1c]
0x54c49c: cmp word ptr [0x7252b4], di
0x54c4a3: jle 0x54c5d0
0x54c4a9: push ebx
0x54c4aa: lea ebx, [ebx]
0x54c4b0: movsx eax, di
0x54c4b3: lea esi, [eax + eax*2]
0x54c4b6: mov cx, word ptr [esi*8 + 0x724a64]
0x54c4be: lea esi, [esi*8 + 0x724a60]
0x54c4c5: movsx eax, cx
0x54c4c8: mov edx, eax
0x54c4ca: shr edx, 3
0x54c4cd: xor ebx, ebx
0x54c4cf: not edx
0x54c4d1: and edx, 1
0x54c4d4: cmp word ptr [ebp + 0x6e], bx
0x54c4d8: mov byte ptr [esp + 0x13], 1
0x54c4dd: sete bl
0x54c4e0: cmp edx, ebx
0x54c4e2: je 0x54c4e9
0x54c4e4: mov byte ptr [esp + 0x13], 0
0x54c4e9: mov edx, eax
0x54c4eb: shr edx, 1
0x54c4ed: xor ebx, ebx
0x54c4ef: not edx
0x54c4f1: and edx, 1
0x54c4f4: cmp word ptr [ebp + 0x6c], bx
0x54c4f8: sete bl
0x54c4fb: cmp edx, ebx
0x54c4fd: je 0x54c503
0x54c4ff: xor dl, dl
0x54c501: jmp 0x54c507
0x54c503: mov dl, byte ptr [esp + 0x13]
0x54c507: mov ebx, eax
0x54c509: shr ebx, 2
0x54c50c: and ebx, 1
0x54c50f: cmp bx, word ptr [ebp + 6]
0x54c513: je 0x54c517
0x54c515: xor dl, dl
0x54c517: test cl, 2
0x54c51a: jne 0x54c531
0x54c51c: mov ebx, dword ptr [esp + 0x1c]
0x54c520: xor ecx, ecx
0x54c522: cmp word ptr [ebx], cx
0x54c525: not eax
0x54c527: sete cl
0x54c52a: and eax, 1
0x54c52d: cmp eax, ecx
0x54c52f: jne 0x54c5aa
0x54c531: test dl, dl
0x54c533: je 0x54c5aa
0x54c535: mov eax, dword ptr [esi]
0x54c537: cmp eax, -1
0x54c53a: je 0x54c5c5
0x54c540: mov edx, dword ptr [esp + 0x20]
0x54c544: mov ecx, dword ptr [esp + 0x2c]
0x54c548: push edx
0x54c549: call 0x54c6b0
0x54c54e: add esp, 4
0x54c551: test al, al
0x54c553: je 0x54c5aa
0x54c555: cmp word ptr [esp + 0x14], -1
0x54c55b: je 0x54c574
0x54c55d: mov eax, dword ptr [esp + 0x24]
0x54c561: mov ecx, dword ptr [esp + 0x18]
0x54c565: push eax
0x54c566: mov eax, dword ptr [esi]
0x54c568: call 0x54c6b0
0x54c56d: add esp, 4
0x54c570: test al, al
0x54c572: je 0x54c5aa
0x54c574: mov ecx, dword ptr [esi]
0x54c576: mov edx, dword ptr [0x7252c0]
0x54c57c: mov esi, dword ptr [edx + 0x34]
0x54c57f: mov eax, ecx
0x54c581: and eax, 0xffff
0x54c586: imul eax, eax, 0xb0
0x54c58c: add eax, esi
0x54c58e: mov dword ptr [esp + 0x18], ecx
0x54c592: lea ecx, [eax + 0x14]
0x54c595: movsx eax, word ptr [eax + 6]
0x54c599: push eax
0x54c59a: mov dword ptr [esp + 0x18], edi
0x54c59e: call 0x54bbd0
0x54c5a3: add esp, 4
0x54c5a6: fstp dword ptr [esp + 0x24]
0x54c5aa: inc edi
0x54c5ab: cmp di, word ptr [0x7252b4]
0x54c5b2: jl 0x54c4b0
0x54c5b8: mov ax, word ptr [esp + 0x14]
0x54c5bd: pop ebx
0x54c5be: pop edi
0x54c5bf: pop esi
0x54c5c0: pop ebp
0x54c5c1: add esp, 0x18
0x54c5c4: ret 
0x54c5c5: pop ebx
0x54c5c6: mov ax, di
0x54c5c9: pop edi
0x54c5ca: pop esi
0x54c5cb: pop ebp
0x54c5cc: add esp, 0x18
0x54c5cf: ret 
0x54c5d0: pop edi
0x54c5d1: pop esi
0x54c5d2: mov ax, dx
0x54c5d5: pop ebp
0x54c5d6: add esp, 0x18
0x54c5d9: ret 
#endif
