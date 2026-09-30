// sound_permutation_pick_for_pitch  (Ghidra: sound_permutation_pick_for_pitch, already named)
// address 0x5454a0, size 228 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Selects the sound permutation whose pitch
//   range best matches the requested pitch, preferring the currently playing one if still
//   valid."; types/tags.h SoundPitchRange (bend_bounds 0x24, permutations 0x3c) and Sound.
//   pitch_ranges (0x98); the repeated `a < b != (a == b)` idiom is exactly `a <= b`.
// register convention: current pitch-range index in AX (in_AX), Sound tag pointer in ECX
//   (in_ECX), target pitch as the one stack parameter Ghidra recognizes directly (param_1).
// blam-cc: AX -> pitch_range_index, ECX -> tag, stack -> target_pitch
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

// blam-cc: AX -> pitch_range_index, ECX -> tag, stack -> target_pitch
// Returns `pitch_range_index` unchanged if it still has loaded permutations and its bend_bounds
// still cover `target_pitch`; otherwise returns the loaded pitch range whose bend_bounds contain
// `target_pitch`, or (failing that) the one whose bounds are numerically closest to it. -1 if no
// pitch range has loaded permutations at all.
int16_t sound_permutation_pick_for_pitch(int16_t pitch_range_index, Sound *tag, float target_pitch)
{
    int32_t count = (int32_t)tag->pitch_ranges.count;
    SoundPitchRange *ranges = (SoundPitchRange *)tag->pitch_ranges.pointer;
    int16_t best = -1;
    float best_ratio = 3.4028235e+38f;
    int32_t i;

    if (pitch_range_index != -1 && pitch_range_index < count) {
        SoundPitchRange *current = &ranges[pitch_range_index];
        if (current->bend_bounds[0] <= target_pitch && target_pitch <= current->bend_bounds[1] &&
            current->permutations.count != 0) {
            return pitch_range_index;
        }
    }

    for (i = 0; i < count; i++) {
        SoundPitchRange *range = &ranges[i];

        if (range->permutations.count != 0) {
            float ratio;

            if (range->bend_bounds[0] <= target_pitch && target_pitch <= range->bend_bounds[1]) {
                return (int16_t)i;
            }

            if (target_pitch <= range->bend_bounds[1]) {
                ratio = range->bend_bounds[0] / target_pitch;
            } else {
                ratio = target_pitch / range->bend_bounds[1];
            }

            if (ratio < best_ratio) {
                best = (int16_t)i;
                best_ratio = ratio;
            }
        }
    }

    return best;
}

#if 0
Original Ghidra decompilation (0x5454a0):

short sound_permutation_pick_for_pitch(float param_1)

{
  float fVar1;
  short in_AX;
  int iVar2;
  int in_ECX;
  short sVar3;
  short sVar4;
  float local_4;

  sVar4 = -1;
  if ((((in_AX != -1) && (iVar2 = (int)in_AX, iVar2 < *(int *)(in_ECX + 0x98))) &&
      (fVar1 = *(float *)(*(int *)(in_ECX + 0x9c) + 0x24 + iVar2 * 0x48),
      iVar2 = *(int *)(in_ECX + 0x9c) + iVar2 * 0x48, fVar1 < param_1 != (fVar1 == param_1))) &&
     ((fVar1 = *(float *)(iVar2 + 0x28), param_1 < fVar1 != (param_1 == fVar1) &&
      (*(int *)(iVar2 + 0x3c) != 0)))) {
    return in_AX;
  }
  sVar3 = 0;
  local_4 = 3.4028235e+38;
  if (0 < *(int *)(in_ECX + 0x98)) {
    iVar2 = 0;
    do {
      iVar2 = *(int *)(in_ECX + 0x9c) + iVar2 * 0x48;
      if (*(int *)(iVar2 + 0x3c) != 0) {
        if ((*(float *)(iVar2 + 0x24) < param_1 != (*(float *)(iVar2 + 0x24) == param_1)) &&
           (param_1 < *(float *)(iVar2 + 0x28) != (param_1 == *(float *)(iVar2 + 0x28)))) {
          return sVar3;
        }
        if (param_1 <= *(float *)(iVar2 + 0x28)) {
          fVar1 = *(float *)(iVar2 + 0x24) / param_1;
        }
        else {
          fVar1 = param_1 / *(float *)(iVar2 + 0x28);
        }
        if (fVar1 < local_4) {
          sVar4 = sVar3;
          local_4 = fVar1;
        }
      }
      sVar3 = sVar3 + 1;
      iVar2 = (int)sVar3;
    } while (iVar2 < *(int *)(in_ECX + 0x98));
  }
  return sVar4;
}

Disassembly (0x5454a0..0x545584, capstone; phase-4 review):

0x5454a0: push ecx
0x5454a1: push ebx
0x5454a2: push esi
0x5454a3: mov esi, eax
0x5454a5: or ebx, 0xffffffff
0x5454a8: cmp si, -1
0x5454ac: je 0x5454f2
0x5454ae: mov edx, dword ptr [ecx + 0x98]
0x5454b4: movsx eax, si
0x5454b7: cmp eax, edx
0x5454b9: jge 0x5454f2
0x5454bb: mov edx, dword ptr [ecx + 0x9c]
0x5454c1: lea eax, [eax + eax*8]
0x5454c4: fld dword ptr [edx + eax*8 + 0x24]
0x5454c8: lea edx, [edx + eax*8]
0x5454cb: fcomp dword ptr [esp + 0x10]
0x5454cf: fnstsw ax
0x5454d1: test ah, 0x41
0x5454d4: jp 0x5454f2
0x5454d6: fld dword ptr [esp + 0x10]
0x5454da: fcomp dword ptr [edx + 0x28]
0x5454dd: fnstsw ax
0x5454df: test ah, 0x41
0x5454e2: jp 0x5454f2
0x5454e4: mov eax, dword ptr [edx + 0x3c]
0x5454e7: test eax, eax
0x5454e9: je 0x5454f2
0x5454eb: mov ax, si
0x5454ee: pop esi
0x5454ef: pop ebx
0x5454f0: pop ecx
0x5454f1: ret 
0x5454f2: push edi
0x5454f3: mov edi, dword ptr [ecx + 0x98]
0x5454f9: xor edx, edx
0x5454fb: test edi, edi
0x5454fd: mov dword ptr [esp + 0xc], 0x7f7fffff
0x545505: jle 0x545574
0x545507: mov esi, dword ptr [ecx + 0x9c]
0x54550d: xor eax, eax
0x54550f: nop 
0x545510: lea eax, [eax + eax*8]
0x545513: lea ecx, [esi + eax*8]
0x545516: mov eax, dword ptr [ecx + 0x3c]
0x545519: test eax, eax
0x54551b: je 0x54556c
0x54551d: fld dword ptr [ecx + 0x24]
0x545520: fcomp dword ptr [esp + 0x14]
0x545524: fnstsw ax
0x545526: test ah, 0x41
0x545529: jp 0x545539
0x54552b: fld dword ptr [esp + 0x14]
0x54552f: fcomp dword ptr [ecx + 0x28]
0x545532: fnstsw ax
0x545534: test ah, 0x41
0x545537: jnp 0x54557c
0x545539: fld dword ptr [ecx + 0x28]
0x54553c: fcomp dword ptr [esp + 0x14]
0x545540: fnstsw ax
0x545542: test ah, 5
0x545545: jp 0x545550
0x545547: fld dword ptr [esp + 0x14]
0x54554b: fdiv dword ptr [ecx + 0x28]
0x54554e: jmp 0x545557
0x545550: fld dword ptr [ecx + 0x24]
0x545553: fdiv dword ptr [esp + 0x14]
0x545557: fcom dword ptr [esp + 0xc]
0x54555b: fnstsw ax
0x54555d: test ah, 5
0x545560: jp 0x54556a
0x545562: fstp dword ptr [esp + 0xc]
0x545566: mov ebx, edx
0x545568: jmp 0x54556c
0x54556a: fstp st(0)
0x54556c: inc edx
0x54556d: movsx eax, dx
0x545570: cmp eax, edi
0x545572: jl 0x545510
0x545574: pop edi
0x545575: pop esi
0x545576: mov ax, bx
0x545579: pop ebx
0x54557a: pop ecx
0x54557b: ret 
0x54557c: pop edi
0x54557d: pop esi
0x54557e: mov ax, dx
0x545581: pop ebx
0x545582: pop ecx
0x545583: ret 
#endif
