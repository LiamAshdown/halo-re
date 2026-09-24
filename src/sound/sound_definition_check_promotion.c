// sound_definition_check_promotion  (Ghidra: FUN_0054b050, still unnamed there)
// address 0x54b050, size 176 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Rate-limits how often a sound on an object may be
// (re)triggered, returning whether to play, substitute a promotion sound, or refuse."; fields
// match the Sound tag's promotion_sound/promotion_count/longest_permutation_length/
// promotion_counter/promotion_time (types/tags.h Sound, 0x70/0x80/0x84/0x88/0x8c) -- the tag's
// own promotion_counter and promotion_time double as runtime rate-limit state, decayed by real
// elapsed sound_time each call.
// register convention: ECX -> sound_tag_id.
//
// disassembly (scratchpad/disasm/disasm.py) shows both "return ... & 0xffff0000" branches Ghidra
// prints are really `mov ax, bp` with bp == 0 and the caller only consulting AX: the true return
// type is a 16-bit code (0 play, 1 play the promotion sound, 2 refuse), with EAX's upper half
// left as unspecified leftover register content that Ghidra folded into a bitwise-AND artifact.
// This rewrite returns int16_t and drops that artifact (a strict "return 0").
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern int32_t sound_time;          // 0x0072520c

// blam-cc: ECX -> sound_tag_id
// Rate-limits (re)triggering `sound_tag_id`: accumulates a decaying counter of recently
// requested permutation time against the tag's promotion_count * longest_permutation_length
// budget. Returns 0 to play normally, 1 to play the promotion sound instead (and resets the
// counter), or 2 to refuse outright (no promotion sound configured; the counter is rolled back
// by this call's own increment so the next call sees the same budget again).
int16_t sound_definition_check_promotion(TagID sound_tag_id)
{
    Sound *sound;
    int32_t permutation_length;
    int32_t accumulated;
    int32_t threshold;

    sound = (Sound *)tag_instances[sound_tag_id.index].data;
    if (sound->promotion_count == 0) {
        return 0;
    }

    permutation_length = (int32_t)sound->longest_permutation_length;
    accumulated = (int32_t)sound->promotion_counter + (sound->promotion_time - sound_time);
    if (accumulated < 0) {
        accumulated = 0;
    }
    accumulated = accumulated + permutation_length;
    sound->promotion_time = sound_time;
    sound->promotion_counter = accumulated;

    threshold = sound->promotion_count * permutation_length;
    if (threshold < accumulated) {
        if (sound->promotion_sound.tag_id.index != 0xffff || sound->promotion_sound.tag_id.id != 0xffff) {
            sound->promotion_counter = 0;
            return 1;
        }
        sound->promotion_counter = accumulated - permutation_length;
        return 2;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x54b050):

uint FUN_0054b050(void)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  int iVar4;
  uint in_ECX;

  iVar1 = *(int *)((in_ECX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(short *)(iVar1 + 0x80) == 0) {
    return DAT_0087bc14 & 0xffff0000;
  }
  iVar2 = *(int *)(iVar1 + 0x84);
  uVar3 = *(int *)(iVar1 + 0x88) + (*(int *)(iVar1 + 0x8c) - DAT_0072520c);
  *(uint *)(iVar1 + 0x88) = uVar3;
  *(uint *)(iVar1 + 0x88) = uVar3 & ((int)uVar3 < 0) - 1;
  iVar4 = *(int *)(iVar1 + 0x88) + iVar2;
  *(int *)(iVar1 + 0x8c) = DAT_0072520c;
  *(int *)(iVar1 + 0x88) = iVar4;
  uVar3 = *(short *)(iVar1 + 0x80) * iVar2;
  if ((int)uVar3 < iVar4) {
    if (*(int *)(iVar1 + 0x7c) != -1) {
      *(undefined4 *)(iVar1 + 0x88) = 0;
      return 1;
    }
    *(int *)(iVar1 + 0x88) = iVar4 - iVar2;
    return 2;
  }
  return uVar3 & 0xffff0000;
}

Disassembly (0x54b050..0x54b0ff, capstone) confirming the AX-only return:

0x54b064: mov di, word ptr [ecx + 0x80]
0x54b06b: xor ebp, ebp
0x54b06d: cmp di, bp
0x54b070: je 0x54b0fa
...
0x54b0f3: pop esi
0x54b0f4: pop edi
0x54b0f5: mov ax, bp
0x54b0f8: pop ebp
0x54b0f9: ret
0x54b0fa: pop edi
0x54b0fb: mov ax, bp
0x54b0fe: pop ebp
0x54b0ff: ret

Disassembly (0x54b050..0x54b100, capstone; phase-4 review):

0x54b050: mov eax, dword ptr [0x87bc14]
0x54b055: and ecx, 0xffff
0x54b05b: shl ecx, 5
0x54b05e: mov ecx, dword ptr [ecx + eax + 0x14]
0x54b062: push ebp
0x54b063: push edi
0x54b064: mov di, word ptr [ecx + 0x80]
0x54b06b: xor ebp, ebp
0x54b06d: cmp di, bp
0x54b070: je 0x54b0fa
0x54b076: mov edx, dword ptr [ecx + 0x8c]
0x54b07c: mov eax, dword ptr [ecx + 0x88]
0x54b082: push esi
0x54b083: sub edx, dword ptr [0x72520c]
0x54b089: mov esi, dword ptr [ecx + 0x84]
0x54b08f: add eax, edx
0x54b091: mov dword ptr [ecx + 0x88], eax
0x54b097: mov edx, eax
0x54b099: mov eax, ebp
0x54b09b: sets al
0x54b09e: dec eax
0x54b09f: and edx, eax
0x54b0a1: mov dword ptr [ecx + 0x88], edx
0x54b0a7: mov edx, dword ptr [0x72520c]
0x54b0ad: mov eax, dword ptr [ecx + 0x88]
0x54b0b3: add eax, esi
0x54b0b5: mov dword ptr [ecx + 0x8c], edx
0x54b0bb: mov edx, eax
0x54b0bd: mov dword ptr [ecx + 0x88], eax
0x54b0c3: movsx eax, di
0x54b0c6: imul eax, esi
0x54b0c9: cmp edx, eax
0x54b0cb: jle 0x54b0f3
0x54b0cd: cmp dword ptr [ecx + 0x7c], -1
0x54b0d1: je 0x54b0e2
0x54b0d3: pop esi
0x54b0d4: pop edi
0x54b0d5: mov dword ptr [ecx + 0x88], ebp
0x54b0db: mov eax, 1
0x54b0e0: pop ebp
0x54b0e1: ret 
0x54b0e2: sub edx, esi
0x54b0e4: pop esi
0x54b0e5: pop edi
0x54b0e6: mov dword ptr [ecx + 0x88], edx
0x54b0ec: mov eax, 2
0x54b0f1: pop ebp
0x54b0f2: ret 
0x54b0f3: pop esi
0x54b0f4: pop edi
0x54b0f5: mov ax, bp
0x54b0f8: pop ebp
0x54b0f9: ret 
0x54b0fa: pop edi
0x54b0fb: mov ax, bp
0x54b0fe: pop ebp
0x54b0ff: ret 
#endif
