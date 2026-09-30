// sound_permutation_pick_random  (Ghidra: sound_permutation_pick_random, already named)
// address 0x545590, size 384 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Randomly selects a sound permutation to play
//   next, weighting against recently used permutations and honoring language-specific
//   exceptions."; types/tags.h SoundPermutation (skip_fraction 0x20, next_permutation_index 0x2a)
//   and SoundPitchRange (actual_permutation_count 0x2c, permutation_flags 0x34,
//   last/discarded_permutation_index 0x38/0x3a, permutations 0x3c); the sound_class exception
//   test (0x12/0x13, 0x20..0x23, < 0x2c) matches types/tags.h SoundClass
//   (unit_footsteps/unit_dialog, the four music/ambient classes, everything below the scripted
//   classes); the trailing sound_permutation_limit branch (0 vs 1 vs other) matches
//   types/sound.h's own comment on that global exactly ("0 keeps only permutation 0, 1 picks from
//   the first half").
// register convention: pitch-range index in AX (in_AX), an explicit "chained" permutation index
//   in CX (in_CX), Sound tag pointer as the one stack parameter Ghidra recognizes directly
//   (param_1).
// blam-cc: AX -> pitch_range_index, CX -> explicit_permutation_index, stack -> tag
// Every caller only consumes AX, so this rewrite returns int16_t.
// Phase-4 review (disassembly appended below): confirmed branch by branch; the half-range pick
//   is effect_random_int_between(ECX = 0, count / 2) (0x44c800, effects module).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"
#include "fn_sound.h"

extern random_seed effect_random_seed;      // 0x00719cd4
extern int16_t sound_permutation_limit;     // 0x007252b8, UNSURE: see types/sound.h globals

extern int effect_random_int_between(int16_t minimum, int16_t maximum); // 0x44c800, effects module, blam-cc: ECX -> minimum, stack -> maximum

// blam-cc: AX -> pitch_range_index, CX -> explicit_permutation_index, stack -> tag
// Picks the next permutation to play for a pitch range: a previously discarded index takes
// priority; then (for tags with Sound flags bit 0x02) an explicit chained permutation's
// next_permutation_index; otherwise a random permutation weighted away from recently-picked ones
// via a per-pitch-range "tried" bitmask and each candidate's skip_fraction, finally forced to
// permutation 0 (or bounded to the first half) for ordinary sound classes when
// sound_permutation_limit restricts variety.
int16_t sound_permutation_pick_random(int16_t pitch_range_index, int16_t explicit_permutation_index, Sound *tag)
{
    SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + pitch_range_index;
    SoundPermutation *permutations;
    int16_t chosen;
    int16_t attempts;
    int16_t sound_class;
    uint32_t candidate;

    if (range->discarded_permutation_index != (uint16_t)0xffff) {
        chosen = (int16_t)range->discarded_permutation_index;
        range->last_permutation_index = (uint16_t)chosen;
        range->discarded_permutation_index = 0xffff;
        return chosen;
    }

    if ((tag->flags & 2) != 0 && explicit_permutation_index != -1) {
        permutations = (SoundPermutation *)range->permutations.pointer;
        return (int16_t)permutations[explicit_permutation_index].next_permutation_index;
    }

    effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
    candidate = (uint32_t)((effect_random_seed >> 16) * (int32_t)range->actual_permutation_count) >> 16;
    attempts = 0;
    permutations = (SoundPermutation *)range->permutations.pointer;

    for (;;) {
        uint32_t candidate_bit;

        chosen = (int16_t)candidate;

        if ((~range->permutation_flags & ((1u << (range->actual_permutation_count & 0x1f)) - 1)) == 0) {
            range->permutation_flags = 0;
            if (range->actual_permutation_count > 1) {
                range->permutation_flags = 1u << (range->last_permutation_index & 0x1f);
            }
        }

        candidate_bit = 1u << (candidate & 0x1f);

        if ((range->permutation_flags & candidate_bit) == 0) {
            range->permutation_flags |= candidate_bit;

            if (attempts == 0x10) {
                break;
            }

            effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
            attempts++;

            if (permutations[chosen].skip_fraction <= (float)(effect_random_seed >> 16) * 1.5259022e-05f) {
                break;
            }
        }

        candidate++;
        if ((int16_t)candidate == (int16_t)range->actual_permutation_count) {
            candidate = 0;
        }
    }

    range->last_permutation_index = (uint16_t)chosen;
    sound_class = tag->sound_class;

    if (sound_class != 0x12 && sound_class != 0x13 &&
        (sound_class < 0x20 || sound_class > 0x23) && sound_class < 0x2c) {
        int32_t limit = sound_permutation_limit;

        if (limit != 0) {
            limit -= 1;
            if (limit != 0) {
                return chosen;
            }
            if (range->actual_permutation_count > 1) {
                return (int16_t)effect_random_int_between(0, (int16_t)((int16_t)range->actual_permutation_count / 2));
            }
        }
        chosen = 0;
    }

    return chosen;
}

#if 0
Original Ghidra decompilation (0x545590):

undefined4 sound_permutation_pick_random(byte *param_1)

{
  int iVar1;
  short in_AX;
  uint uVar2;
  undefined2 uVar5;
  int iVar3;
  undefined4 uVar4;
  short in_CX;
  short sVar6;
  short sVar7;
  uint uVar8;

  iVar1 = *(int *)(param_1 + 0x9c) + in_AX * 0x48;
  sVar7 = *(short *)(iVar1 + 0x3a);
  sVar6 = 0;
  if (sVar7 != -1) {
    *(short *)(iVar1 + 0x38) = sVar7;
    *(undefined2 *)(iVar1 + 0x3a) = 0xffff;
    return CONCAT22((short)((uint)(in_AX * 9) >> 0x10),sVar7);
  }
  if (((*param_1 & 2) != 0) && (in_CX != -1)) {
    return CONCAT22((short)((uint)(in_CX * 0x7c) >> 0x10),
                    *(undefined2 *)(in_CX * 0x7c + 0x2a + *(int *)(iVar1 + 0x40)));
  }
  DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
  uVar8 = (DAT_00719cd4 >> 0x10) * (int)*(short *)(iVar1 + 0x2c) >> 0x10;
LAB_00545611:
  sVar7 = (short)uVar8;
  if (((~*(uint *)(iVar1 + 0x34) & (1 << ((byte)*(short *)(iVar1 + 0x2c) & 0x1f)) - 1U) == 0) &&
     (*(undefined4 *)(iVar1 + 0x34) = 0, 1 < *(short *)(iVar1 + 0x2c))) {
    *(int *)(iVar1 + 0x34) = 1 << (*(byte *)(iVar1 + 0x38) & 0x1f);
  }
  uVar2 = 1 << ((byte)uVar8 & 0x1f);
  if ((*(uint *)(iVar1 + 0x34) & uVar2) == 0) {
    uVar2 = uVar2 | *(uint *)(iVar1 + 0x34);
    *(uint *)(iVar1 + 0x34) = uVar2;
    uVar5 = (undefined2)(uVar2 >> 0x10);
    if (sVar6 == 0x10) goto LAB_005456ab;
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar5 = (undefined2)((uint)*(int *)(iVar1 + 0x40) >> 0x10);
    sVar6 = sVar6 + 1;
    if (*(float *)(sVar7 * 0x7c + 0x20 + *(int *)(iVar1 + 0x40)) <=
        (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05) goto LAB_005456ab;
  }
  uVar8 = uVar8 + 1;
  if ((short)uVar8 == *(short *)(iVar1 + 0x2c)) {
    uVar8 = 0;
  }
  goto LAB_00545611;
LAB_005456ab:
  *(short *)(iVar1 + 0x38) = sVar7;
  sVar6 = *(short *)(param_1 + 4);
  iVar3 = CONCAT22(uVar5,sVar6);
  if ((((sVar6 != 0x12) && (sVar6 != 0x13)) && ((sVar6 < 0x20 || (0x23 < sVar6)))) && (sVar6 < 0x2c)
     ) {
    iVar3 = (int)DAT_007252b8;
    if (iVar3 != 0) {
      iVar3 = iVar3 + -1;
      if (iVar3 != 0) goto LAB_00545709;
      if (1 < *(short *)(iVar1 + 0x2c)) {
        uVar4 = FUN_0044c800((int)*(short *)(iVar1 + 0x2c) / 2);
        return uVar4;
      }
    }
    sVar7 = 0;
  }
LAB_00545709:
  return CONCAT22((short)((uint)iVar3 >> 0x10),sVar7);
}

Disassembly (0x545590..0x545710, capstone; phase-4 review):

0x545590: push ebx
0x545591: push ebp
0x545592: mov ebp, dword ptr [esp + 0xc]
0x545596: mov edx, dword ptr [ebp + 0x9c]
0x54559c: movsx eax, ax
0x54559f: lea eax, [eax + eax*8]
0x5455a2: push esi
0x5455a3: lea edx, [edx + eax*8]
0x5455a6: xor esi, esi
0x5455a8: mov si, word ptr [edx + 0x3a]
0x5455ac: xor ebx, ebx
0x5455ae: cmp si, -1
0x5455b2: je 0x5455c5
0x5455b4: mov word ptr [edx + 0x38], si
0x5455b8: mov ax, si
0x5455bb: pop esi
0x5455bc: pop ebp
0x5455bd: mov word ptr [edx + 0x3a], 0xffff
0x5455c3: pop ebx
0x5455c4: ret 
0x5455c5: test byte ptr [ebp], 2
0x5455c9: je 0x5455e6
0x5455cb: cmp cx, -1
0x5455cf: je 0x5455e6
0x5455d1: movsx eax, cx
0x5455d4: mov ecx, dword ptr [edx + 0x40]
0x5455d7: imul eax, eax, 0x7c
0x5455da: mov si, word ptr [eax + ecx + 0x2a]
0x5455df: mov ax, si
0x5455e2: pop esi
0x5455e3: pop ebp
0x5455e4: pop ebx
0x5455e5: ret 
0x5455e6: mov ecx, dword ptr [0x719cd4]
0x5455ec: mov ax, word ptr [edx + 0x2c]
0x5455f0: imul ecx, ecx, 0x19660d
0x5455f6: add ecx, 0x3c6ef35f
0x5455fc: mov esi, ecx
0x5455fe: movsx eax, ax
0x545601: shr esi, 0x10
0x545604: imul esi, eax
0x545607: mov dword ptr [0x719cd4], ecx
0x54560d: shr esi, 0x10
0x545610: push edi
0x545611: mov ax, word ptr [edx + 0x2c]
0x545615: mov cl, al
0x545617: mov edi, 1
0x54561c: shl edi, cl
0x54561e: mov ecx, dword ptr [edx + 0x34]
0x545621: not ecx
0x545623: dec edi
0x545624: test ecx, edi
0x545626: jne 0x545642
0x545628: cmp ax, 1
0x54562c: mov dword ptr [edx + 0x34], 0
0x545633: jle 0x545642
0x545635: mov cl, byte ptr [edx + 0x38]
0x545638: mov eax, 1
0x54563d: shl eax, cl
0x54563f: mov dword ptr [edx + 0x34], eax
0x545642: mov edi, dword ptr [edx + 0x34]
0x545645: movsx ecx, si
0x545648: mov eax, 1
0x54564d: shl eax, cl
0x54564f: test edi, eax
0x545651: jne 0x545699
0x545653: or eax, edi
0x545655: mov dword ptr [edx + 0x34], eax
0x545658: mov ax, bx
0x54565b: inc ebx
0x54565c: cmp ax, 0x10
0x545660: je 0x5456ab
0x545662: mov eax, dword ptr [0x719cd4]
0x545667: imul ecx, ecx, 0x7c
0x54566a: imul eax, eax, 0x19660d
0x545670: add eax, 0x3c6ef35f
0x545675: mov dword ptr [0x719cd4], eax
0x54567a: shr eax, 0x10
0x54567d: mov dword ptr [esp + 0x14], eax
0x545681: mov eax, dword ptr [edx + 0x40]
0x545684: fild dword ptr [esp + 0x14]
0x545688: fmul dword ptr [0x672b84]
0x54568e: fcomp dword ptr [ecx + eax + 0x20]
0x545692: fnstsw ax
0x545694: test ah, 1
0x545697: je 0x5456ab
0x545699: inc esi
0x54569a: cmp si, word ptr [edx + 0x2c]
0x54569e: jne 0x545611
0x5456a4: xor esi, esi
0x5456a6: jmp 0x545611
0x5456ab: mov word ptr [edx + 0x38], si
0x5456af: mov ax, word ptr [ebp + 4]
0x5456b3: cmp ax, 0x12
0x5456b7: pop edi
0x5456b8: je 0x545709
0x5456ba: cmp ax, 0x13
0x5456be: je 0x545709
0x5456c0: cmp ax, 0x20
0x5456c4: jl 0x5456cc
0x5456c6: cmp ax, 0x23
0x5456ca: jle 0x545709
0x5456cc: cmp ax, 0x2c
0x5456d0: jge 0x545709
0x5456d2: movsx eax, word ptr [0x7252b8]
0x5456d9: sub eax, 0
0x5456dc: je 0x545707
0x5456de: dec eax
0x5456df: jne 0x545709
0x5456e1: mov dx, word ptr [edx + 0x2c]
0x5456e5: cmp dx, 1
0x5456e9: jle 0x545707
0x5456eb: movsx eax, dx
0x5456ee: cdq 
0x5456ef: sub eax, edx
0x5456f1: sar eax, 1
0x5456f3: push eax
0x5456f4: xor ecx, ecx
0x5456f6: call 0x44c800
0x5456fb: add esp, 4
0x5456fe: mov esi, eax
0x545700: mov ax, si
0x545703: pop esi
0x545704: pop ebp
0x545705: pop ebx
0x545706: ret 
0x545707: xor esi, esi
0x545709: mov ax, si
0x54570c: pop esi
0x54570d: pop ebp
0x54570e: pop ebx
0x54570f: ret 
#endif
