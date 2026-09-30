// sound_pick_replaceable_channel  (Ghidra: FUN_0054c5e0, still unnamed there)
// address 0x54c5e0, size 207 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md "Picks a candidate playback channel that has played
// long enough and is close enough in distance to be safely replaced."; minimum_replace_time
// (0x0069eae0+4 == 0x0069eae4, types/sound.h sound_class_definition) matches "iVar2" here.
// register convention: EAX -> sound_handle, stack -> (candidate_channels, count).
// Phase-4 review (disassembly appended below): EDX still holds the candidate's Sound tag
//   pointer (from the distance call's setup) when the class is read, so recomputing it is exact;
//   each distance call passes the location of the same sound whose listener_index it pushes
//   (the draft measured the occupant's distance from the candidate's location). Returns AX:
//   the first candidate channel whose sound has played at least minimum_replace_time ms and is
//   not more than 1.0 (squared units) closer than the new sound.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t sound_time;          // 0x0072520c
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60


// blam-cc: EAX -> sound_handle, stack -> (candidate_channels, count)
// Among `candidate_channels[0..count)` (channel indices), returns the first whose current sound
// has played at least the candidate's class's minimum_replace_time and whose distance is within
// 1.0 (squared) of the candidate's own distance, or -1 if none qualify.
int16_t sound_pick_replaceable_channel(datum_index sound_handle, int16_t *candidate_channels, int16_t count)
{
    sound *candidate;
    Sound *definition;
    float candidate_distance_squared;
    int32_t minimum_replace_time;
    int16_t i;
    sound *occupant;
    float occupant_distance_squared;

    candidate = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)tag_instances[candidate->definition_index & 0xffff].data;
    candidate_distance_squared = sound_location_distance_squared(candidate->listener_index, &candidate->location);
    minimum_replace_time = sound_class_definitions[definition->sound_class].minimum_replace_time;

    for (i = 0; i < count; i++) {
        occupant = (sound *)((uint8_t *)sound_data->data +
            (sound_channels[candidate_channels[i]].sound_index & 0xffff) * sizeof(sound));
        if (minimum_replace_time <= sound_time - occupant->start_time) {
            occupant_distance_squared = sound_location_distance_squared(occupant->listener_index, &occupant->location);
            if (candidate_distance_squared - occupant_distance_squared < 1.0f) {
                return candidate_channels[i];
            }
        }
    }
    return -1;
}

#if 0
Original Ghidra decompilation (0x54c5e0):

undefined2 FUN_0054c5e0(int param_1,short param_2)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  int iVar4;
  undefined2 extraout_DX;
  int extraout_EDX;
  short sVar5;
  float10 fVar6;
  float10 fVar7;

  iVar1 = *(int *)(DAT_007252c0 + 0x34);
  fVar6 = (float10)FUN_0054bbd0((int)*(short *)((in_EAX & 0xffff) * 0xb0 + iVar1 + 6));
  iVar3 = DAT_0072520c;
  sVar5 = 0;
  if (0 < param_2) {
    iVar2 = *(int *)(&DAT_0069eae4 + *(short *)(extraout_EDX + 4) * 0x2c);
    do {
      iVar4 = ((&DAT_00724a60)[*(short *)(param_1 + sVar5 * 2) * 6] & 0xffff) * 0xb0 + iVar1;
      if (iVar2 <= iVar3 - *(int *)(iVar4 + 0x84)) {
        fVar7 = (float10)FUN_0054bbd0((int)*(short *)(iVar4 + 6));
        if ((float10)(float)fVar6 - fVar7 < (float10)1.0) {
          return extraout_DX;
        }
      }
      sVar5 = sVar5 + 1;
    } while (sVar5 < param_2);
  }
  return 0xffff;
}

Disassembly (0x54c5e0..0x54c6af, capstone; phase-4 review):

0x54c5e0: push ecx
0x54c5e1: mov ecx, dword ptr [0x7252c0]
0x54c5e7: and eax, 0xffff
0x54c5ec: imul eax, eax, 0xb0
0x54c5f2: push ebx
0x54c5f3: push ebp
0x54c5f4: push esi
0x54c5f5: push edi
0x54c5f6: mov edi, dword ptr [ecx + 0x34]
0x54c5f9: mov edx, dword ptr [eax + edi + 8]
0x54c5fd: mov ecx, dword ptr [0x87bc14]
0x54c603: add eax, edi
0x54c605: and edx, 0xffff
0x54c60b: shl edx, 5
0x54c60e: mov edx, dword ptr [edx + ecx + 0x14]
0x54c612: lea ecx, [eax + 0x14]
0x54c615: movsx eax, word ptr [eax + 6]
0x54c619: push eax
0x54c61a: call 0x54bbd0
0x54c61f: xor esi, esi
0x54c621: add esp, 4
0x54c624: fstp dword ptr [esp + 0x10]
0x54c628: cmp word ptr [esp + 0x1c], si
0x54c62d: jle 0x54c69c
0x54c62f: movsx ecx, word ptr [edx + 4]
0x54c633: mov ebp, dword ptr [0x72520c]
0x54c639: imul ecx, ecx, 0x2c
0x54c63c: mov ebx, dword ptr [ecx + 0x69eae4]
0x54c642: mov eax, dword ptr [esp + 0x18]
0x54c646: movsx edx, si
0x54c649: mov dx, word ptr [eax + edx*2]
0x54c64d: movsx eax, dx
0x54c650: lea ecx, [eax + eax*2]
0x54c653: mov eax, dword ptr [ecx*8 + 0x724a60]
0x54c65a: and eax, 0xffff
0x54c65f: imul eax, eax, 0xb0
0x54c665: add eax, edi
0x54c667: mov ecx, ebp
0x54c669: sub ecx, dword ptr [eax + 0x84]
0x54c66f: cmp ecx, ebx
0x54c671: jl 0x54c694
0x54c673: lea ecx, [eax + 0x14]
0x54c676: movsx eax, word ptr [eax + 6]
0x54c67a: push eax
0x54c67b: call 0x54bbd0
0x54c680: add esp, 4
0x54c683: fsubr dword ptr [esp + 0x10]
0x54c687: fcomp dword ptr [0x672ac4]
0x54c68d: fnstsw ax
0x54c68f: test ah, 5
0x54c692: jnp 0x54c6a6
0x54c694: inc esi
0x54c695: cmp si, word ptr [esp + 0x1c]
0x54c69a: jl 0x54c642
0x54c69c: pop edi
0x54c69d: pop esi
0x54c69e: pop ebp
0x54c69f: or ax, 0xffff
0x54c6a3: pop ebx
0x54c6a4: pop ecx
0x54c6a5: ret 
0x54c6a6: pop edi
0x54c6a7: pop esi
0x54c6a8: pop ebp
0x54c6a9: mov ax, dx
0x54c6ac: pop ebx
0x54c6ad: pop ecx
0x54c6ae: ret 
#endif
