// sound_pick_channel_for_instance  (Ghidra: FUN_0054c2f0, still unnamed there)
// address 0x54c2f0, size 322 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Chooses (or steals) a pitch-range playback channel
// for a new sound instance to occupy."; caller (0x54c020, sound_assign_channels.c) treats this
// function's result as an int16_t channel index (`if (sVar4 == -1) goto stop`), so despite
// Ghidra's `void` signature here (the return value flows through untracked EAX), this rewrite
// returns int16_t and forwards each sub-call's result with an explicit `return`.
// register convention: sound handle as the recognized parameter (param_1).
// Phase-4 review (disassembly appended below): the value is returned in AX on every path. A
// sound that already has a channel returns it; the dialog branch returns the loop index (the
// channel the owner's other dialog sound is on) after copying that sound's location.type (+0x14)
// into the new sound (the draft copied listener_index and returned -1).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern int16_t sound_channel_count; // 0x007252b4
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60


// blam-cc: sound handle as the recognized parameter
// Chooses a playback channel for a sound that does not have one yet. Non-dialog sounds (and
// dialog sounds with no owner) first try to steal a channel from another sound sharing their
// owner or tag, once the relevant budget is exceeded; dialog sounds with an owner instead look
// for another currently-playing dialog sound from the same owner and take over its channel
// (copying its location type). Falls back to sound_find_lowest_priority_channel otherwise. A
// sound that already has a channel keeps it.
int16_t sound_pick_channel_for_instance(datum_index sound_handle)
{
    sound *instance;
    Sound *definition;
    sound_channel_candidate_list candidates;
    int16_t channel_index;

    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));

    if (instance->channel_index == -1) {
        definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

        if (sound_class_definitions[definition->sound_class].dialog == 0 || instance->owner_index == 0xffffffff) {
            sound_build_channel_candidates(&candidates, sound_handle);
            if (candidates.owner_match_limit <= candidates.owner_match_count) {
                return sound_pick_replaceable_channel(sound_handle, candidates.owner_matches, candidates.owner_match_count);
            }
            if (candidates.tag_match_limit <= candidates.tag_match_count) {
                return sound_pick_replaceable_channel(sound_handle, candidates.tag_matches, candidates.tag_match_count);
            }
        } else {
            for (channel_index = 0; channel_index < sound_channel_count; channel_index++) {
                if (sound_channels[channel_index].sound_index != 0xffffffff) {
                    sound *other = (sound *)((uint8_t *)sound_data->data +
                        (sound_channels[channel_index].sound_index & 0xffff) * sizeof(sound));
                    if (other->owner_index == instance->owner_index) {
                        Sound *other_definition = (Sound *)tag_instances[other->definition_index & 0xffff].data;
                        if (sound_class_definitions[other_definition->sound_class].dialog != 0) {
                            // take over that dialog's channel, with its location type
                            instance->location.type = other->location.type;
                            return channel_index;
                        }
                    }
                }
            }
        }

        return sound_find_lowest_priority_channel(sound_handle);
    }
    return instance->channel_index;
}

#if 0
Original Ghidra decompilation (0x54c2f0):

void FUN_0054c2f0(uint param_1)

{
  int iVar1;
  short sVar2;
  int iVar3;
  int iVar4;
  undefined4 local_48;
  short local_26;
  undefined4 local_24;
  short local_2;

  iVar3 = (param_1 & 0xffff) * 0xb0;
  iVar1 = *(int *)(DAT_007252c0 + 0x34);
  iVar4 = iVar3 + iVar1;
  if (*(short *)(iVar3 + 0x8c + iVar1) == -1) {
    if (((&DAT_0069eae8)
         [*(short *)(*(int *)((*(uint *)(iVar4 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 4) *
          0x2c] == '\0') || (*(int *)(iVar4 + 0xc) == -1)) {
      FUN_0054c1d0(param_1);
      if (local_2 <= (short)local_24) {
        FUN_0054c5e0((int)&local_24 + 2,local_24);
        return;
      }
      if (local_26 <= (short)local_48) {
        FUN_0054c5e0((int)&local_48 + 2,local_48);
        return;
      }
    }
    else {
      sVar2 = 0;
      if (0 < DAT_007252b4) {
        do {
          if ((((&DAT_00724a60)[sVar2 * 6] != 0xffffffff) &&
              (iVar3 = ((&DAT_00724a60)[sVar2 * 6] & 0xffff) * 0xb0 + iVar1,
              *(int *)(iVar3 + 0xc) == *(int *)(iVar4 + 0xc))) &&
             ((&DAT_0069eae8)
              [*(short *)(*(int *)(DAT_0087bc14 + 0x14 + (*(uint *)(iVar3 + 8) & 0xffff) * 0x20) + 4
                         ) * 0x2c] != '\0')) {
            *(undefined2 *)(iVar4 + 0x14) = *(undefined2 *)(iVar3 + 0x14);
            return;
          }
          sVar2 = sVar2 + 1;
        } while (sVar2 < DAT_007252b4);
      }
    }
    FUN_0054c440(param_1);
  }
  return;
}

Disassembly (0x54c2f0..0x54c432, capstone; phase-4 review):

0x54c2f0: mov eax, dword ptr [0x7252c0]
0x54c2f5: sub esp, 0x48
0x54c2f8: push ebp
0x54c2f9: mov ebp, dword ptr [esp + 0x50]
0x54c2fd: mov edx, ebp
0x54c2ff: and edx, 0xffff
0x54c305: imul edx, edx, 0xb0
0x54c30b: push esi
0x54c30c: mov esi, dword ptr [eax + 0x34]
0x54c30f: mov ax, word ptr [edx + esi + 0x8c]
0x54c317: add edx, esi
0x54c319: cmp ax, 0xffff
0x54c31d: jne 0x54c3d0
0x54c323: mov ecx, dword ptr [edx + 8]
0x54c326: and ecx, 0xffff
0x54c32c: shl ecx, 5
0x54c32f: push ebx
0x54c330: mov ebx, dword ptr [0x87bc14]
0x54c336: mov eax, dword ptr [ecx + ebx + 0x14]
0x54c33a: movsx ecx, word ptr [eax + 4]
0x54c33e: imul ecx, ecx, 0x2c
0x54c341: mov al, byte ptr [ecx + 0x69eae8]
0x54c347: test al, al
0x54c349: push edi
0x54c34a: je 0x54c3e6
0x54c350: mov edi, dword ptr [edx + 0xc]
0x54c353: cmp edi, -1
0x54c356: je 0x54c3e6
0x54c35c: xor eax, eax
0x54c35e: cmp word ptr [0x7252b4], ax
0x54c365: jle 0x54c3c5
0x54c367: jmp 0x54c370
0x54c369: lea esp, [esp]
0x54c370: movsx ecx, ax
0x54c373: lea ecx, [ecx + ecx*2]
0x54c376: lea ecx, [ecx*8 + 0x724a60]
0x54c37d: mov ecx, dword ptr [ecx]
0x54c37f: cmp ecx, -1
0x54c382: je 0x54c3bb
0x54c384: and ecx, 0xffff
0x54c38a: imul ecx, ecx, 0xb0
0x54c390: add ecx, esi
0x54c392: cmp dword ptr [ecx + 0xc], edi
0x54c395: jne 0x54c3bb
0x54c397: mov ebp, dword ptr [ecx + 8]
0x54c39a: and ebp, 0xffff
0x54c3a0: shl ebp, 5
0x54c3a3: mov ebp, dword ptr [ebx + ebp + 0x14]
0x54c3a7: movsx ebp, word ptr [ebp + 4]
0x54c3ab: imul ebp, ebp, 0x2c
0x54c3ae: cmp byte ptr [ebp + 0x69eae8], 0
0x54c3b5: jne 0x54c3d6
0x54c3b7: mov ebp, dword ptr [esp + 0x5c]
0x54c3bb: inc eax
0x54c3bc: cmp ax, word ptr [0x7252b4]
0x54c3c3: jl 0x54c370
0x54c3c5: push ebp
0x54c3c6: call 0x54c440
0x54c3cb: add esp, 4
0x54c3ce: pop edi
0x54c3cf: pop ebx
0x54c3d0: pop esi
0x54c3d1: pop ebp
0x54c3d2: add esp, 0x48
0x54c3d5: ret 
0x54c3d6: mov cx, word ptr [ecx + 0x14]
0x54c3da: pop edi
0x54c3db: pop ebx
0x54c3dc: pop esi
0x54c3dd: mov word ptr [edx + 0x14], cx
0x54c3e1: pop ebp
0x54c3e2: add esp, 0x48
0x54c3e5: ret 
0x54c3e6: push ebp
0x54c3e7: lea esi, [esp + 0x14]
0x54c3eb: call 0x54c1d0
0x54c3f0: mov eax, dword ptr [esp + 0x38]
0x54c3f4: add esp, 4
0x54c3f7: cmp ax, word ptr [esp + 0x56]
0x54c3fc: jl 0x54c416
0x54c3fe: push eax
0x54c3ff: lea edx, [esp + 0x3a]
0x54c403: push edx
0x54c404: mov eax, ebp
0x54c406: call 0x54c5e0
0x54c40b: add esp, 8
0x54c40e: pop edi
0x54c40f: pop ebx
0x54c410: pop esi
0x54c411: pop ebp
0x54c412: add esp, 0x48
0x54c415: ret 
0x54c416: mov eax, dword ptr [esp + 0x10]
0x54c41a: cmp ax, word ptr [esp + 0x32]
0x54c41f: jl 0x54c3c5
0x54c421: push eax
0x54c422: lea eax, [esp + 0x16]
0x54c426: push eax
0x54c427: mov eax, ebp
0x54c429: call 0x54c5e0
0x54c42e: add esp, 8
0x54c431: pop edi
#endif
