// sound_looping_check_audibility_gate  (Ghidra: FUN_0054e740, still unnamed there)
// address 0x54e740, size 178 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: out/phase4/sound_functions.md "Checks whether a looping sound's permutations would
// actually be audible before allowing a state change to proceed."; gated by
// sound_looping_audibility_check (types/sound.h, 0x00724a54). Its only caller,
// sound_looping_set_state (0x549fa0), passes the SoundLooping tag in EAX and the
// sound_location pointer on the stack; `*(short *)arg == 1` is location->type ==
// _sound_location_absolute.
// register convention: EAX -> definition_index, stack -> location.
// Phase-4 review against the disassembly (appended below): the function really has no side
// effect and really reads tracks[0] / detail_sounds[0] on every iteration (the loaded pointer
// is loop-invariant in the binary, not a decompiler artefact). It is the residue of a stripped
// debug warning (an absolute-position looping sound whose loop has no minimum distance); it is
// kept literally, including the loops that can only spin or fall through.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t sound_looping_audibility_check; // 0x00724a54
extern tag_instance *tag_instances;            // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

// blam-cc: EAX -> definition_index, stack -> location
void sound_looping_check_audibility_gate(datum_index definition_index, sound_location *location)
{
    SoundLooping *definition;
    int16_t i;

    if (!sound_looping_audibility_check || location->type != _sound_location_absolute) {
        return;
    }

    definition = (SoundLooping *)tag_instances[definition_index & 0xffff].data;

    if ((int32_t)definition->tracks.count > 0) {
        SoundLoopingTrack *first_track = (SoundLoopingTrack *)definition->tracks.pointer;

        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            if (*(uint32_t *)&first_track->loop.tag_id != 0xffffffff) {
                Sound *loop_sound = (Sound *)tag_instances[first_track->loop.tag_id.index].data;

                if (loop_sound->minimum_distance != 0.0f) {
                    return;
                }
                if (sound_class_definitions[loop_sound->sound_class].default_minimum_distance != 0.0f) {
                    return;
                }
                break;
            }
        }
    }

    if ((int32_t)definition->detail_sounds.count > 0) {
        SoundLoopingDetail *first_detail = (SoundLoopingDetail *)definition->detail_sounds.pointer;

        for (i = 0; i < (int32_t)definition->detail_sounds.count; i++) {
            if (*(uint32_t *)&first_detail->sound.tag_id != 0xffffffff) {
                return;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x54e740):

void FUN_0054e740(short *param_1)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  short sVar3;

  if ((DAT_00724a54 != '\0') && (*param_1 == 1)) {
    iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar3 = 0;
    if (0 < *(int *)(iVar1 + 0x3c)) {
      do {
        if (*(int *)(*(int *)(iVar1 + 0x40) + 0x4c) != -1) {
          iVar2 = *(int *)((*(uint *)(*(int *)(iVar1 + 0x40) + 0x4c) & 0xffff) * 0x20 + 0x14 +
                          DAT_0087bc14);
          if (*(float *)(iVar2 + 8) != 0.0) {
            return;
          }
          if (*(float *)(&DAT_0069eaf8 + *(short *)(iVar2 + 4) * 0x2c) != 0.0) {
            return;
          }
          break;
        }
        sVar3 = sVar3 + 1;
      } while ((int)sVar3 < *(int *)(iVar1 + 0x3c));
    }
    sVar3 = 0;
    if (0 < *(int *)(iVar1 + 0x48)) {
      do {
        if (*(int *)(*(int *)(iVar1 + 0x4c) + 0xc) != -1) {
          return;
        }
        sVar3 = sVar3 + 1;
      } while ((int)sVar3 < *(int *)(iVar1 + 0x48));
    }
  }
  return;
}

Disassembly (0x54e740..0x54e7f2, capstone; phase-4 review):

0x54e740: mov cl, byte ptr [0x724a54]
0x54e746: test cl, cl
0x54e748: je 0x54e7f1
0x54e74e: mov ecx, dword ptr [esp + 4]
0x54e752: cmp word ptr [ecx], 1
0x54e756: jne 0x54e7f1
0x54e75c: push ebx
0x54e75d: push ebp
0x54e75e: and eax, 0xffff
0x54e763: push esi
0x54e764: mov esi, dword ptr [0x87bc14]
0x54e76a: shl eax, 5
0x54e76d: mov ecx, dword ptr [eax + esi + 0x14]
0x54e771: mov eax, dword ptr [ecx + 0x3c]
0x54e774: xor edx, edx
0x54e776: test eax, eax
0x54e778: push edi
0x54e779: jle 0x54e7ce
0x54e77b: mov edi, dword ptr [ecx + 0x40]
0x54e77e: mov ebx, dword ptr [edi + 0x4c]
0x54e781: cmp ebx, -1
0x54e784: jne 0x54e790
0x54e786: inc edx
0x54e787: movsx ebp, dx
0x54e78a: cmp ebp, eax
0x54e78c: jl 0x54e781
0x54e78e: jmp 0x54e7ce
0x54e790: mov edx, dword ptr [edi + 0x4c]
0x54e793: fld dword ptr [0x672ac0]
0x54e799: and edx, 0xffff
0x54e79f: shl edx, 5
0x54e7a2: mov edx, dword ptr [edx + esi + 0x14]
0x54e7a6: fld dword ptr [edx + 8]
0x54e7a9: fucompp 
0x54e7ab: fnstsw ax
0x54e7ad: test ah, 0x44
0x54e7b0: jp 0x54e7ed
0x54e7b2: movsx eax, word ptr [edx + 4]
0x54e7b6: fld dword ptr [0x672ac0]
0x54e7bc: imul eax, eax, 0x2c
0x54e7bf: fld dword ptr [eax + 0x69eaf8]
0x54e7c5: fucompp 
0x54e7c7: fnstsw ax
0x54e7c9: test ah, 0x44
0x54e7cc: jp 0x54e7ed
0x54e7ce: mov edx, dword ptr [ecx + 0x48]
0x54e7d1: xor eax, eax
0x54e7d3: test edx, edx
0x54e7d5: jle 0x54e7ed
0x54e7d7: mov ecx, dword ptr [ecx + 0x4c]
0x54e7da: mov ecx, dword ptr [ecx + 0xc]
0x54e7dd: lea ecx, [ecx]
0x54e7e0: cmp ecx, -1
0x54e7e3: jne 0x54e7ed
0x54e7e5: inc eax
0x54e7e6: movsx esi, ax
0x54e7e9: cmp esi, edx
0x54e7eb: jl 0x54e7e0
0x54e7ed: pop edi
0x54e7ee: pop esi
0x54e7ef: pop ebp
0x54e7f0: pop ebx
0x54e7f1: ret 
#endif
