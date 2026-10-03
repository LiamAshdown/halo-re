// sound_looping_definition_has_music_loop  (Ghidra: FUN_00544c10)
// address 0x544c10, size 87 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md misattribution note for the paired address 0x544000:
//   "the arithmetic is SoundLooping.tracks ... so these are looping sound definition helpers
//   (music class test, preload)" -- this is the "music class test" half. Every track's loop sound
//   (SoundLoopingTrack.loop.tag_id, +0x4c) is checked against Sound.sound_class (+0x04) ==
//   soundclass_music (0x20, types/tags.h SoundClass). The CONCAT31 return dance Ghidra prints is
//   the compiler folding a byte-sized boolean into EAX alongside leftover high bytes; this rewrite
//   returns a plain 0/1.
// register convention: SoundLooping tag handle in EAX (in_EAX).
// blam-cc: EAX -> looping_definition
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

// blam-cc: EAX -> looping_definition
// True if any track of `looping_definition` has a "loop" sound belonging to the music sound
// class.
uint32_t sound_looping_definition_has_music_loop(datum_index looping_definition)
{
    SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
    int32_t i;

    for (i = 0; i < (int32_t)definition->tracks.count; i++) {
        SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;
        uint32_t loop_sound = *(uint32_t *)&track->loop.tag_id;

        if (loop_sound != 0xffffffff) {
            Sound *loop_tag = (Sound *)tag_instances[loop_sound & 0xffff].data;
            if (loop_tag->sound_class == soundclass_music) {
                return 1;
            }
        }
    }

    return 0;
}

#if 0
Original Ghidra decompilation (0x544c10):

uint FUN_00544c10(void)

{
  int iVar1;
  int iVar2;
  int iVar3;
  uint in_EAX;
  uint uVar4;
  short sVar5;

  uVar4 = (in_EAX & 0xffff) * 0x20;
  iVar1 = *(int *)(uVar4 + 0x14 + DAT_0087bc14);
  iVar2 = *(int *)(iVar1 + 0x3c);
  sVar5 = 0;
  if (0 < iVar2) {
    uVar4 = 0;
    do {
      uVar4 = *(uint *)(uVar4 * 0xa0 + *(int *)(iVar1 + 0x40) + 0x4c);
      if ((uVar4 != 0xffffffff) &&
         (iVar3 = *(int *)((uVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
         *(short *)(iVar3 + 4) == 0x20)) {
        return CONCAT31((int3)((uint)iVar3 >> 8),1);
      }
      sVar5 = sVar5 + 1;
      uVar4 = (uint)sVar5;
    } while ((int)uVar4 < iVar2);
  }
  return uVar4 & 0xffffff00;
}

Disassembly (0x544c10..0x544c67, capstone; phase-4 review):

0x544c10: push esi
0x544c11: and eax, 0xffff
0x544c16: push edi
0x544c17: mov edi, dword ptr [0x87bc14]
0x544c1d: shl eax, 5
0x544c20: mov ecx, dword ptr [eax + edi + 0x14]
0x544c24: mov edx, dword ptr [ecx + 0x3c]
0x544c27: xor esi, esi
0x544c29: test edx, edx
0x544c2b: jle 0x544c5d
0x544c2d: mov ecx, dword ptr [ecx + 0x40]
0x544c30: xor eax, eax
0x544c32: lea eax, [eax + eax*4]
0x544c35: shl eax, 5
0x544c38: add eax, ecx
0x544c3a: mov eax, dword ptr [eax + 0x4c]
0x544c3d: cmp eax, -1
0x544c40: je 0x544c55
0x544c42: and eax, 0xffff
0x544c47: shl eax, 5
0x544c4a: mov eax, dword ptr [eax + edi + 0x14]
0x544c4e: cmp word ptr [eax + 4], 0x20
0x544c53: je 0x544c62
0x544c55: inc esi
0x544c56: movsx eax, si
0x544c59: cmp eax, edx
0x544c5b: jl 0x544c32
0x544c5d: pop edi
0x544c5e: xor al, al
0x544c60: pop esi
0x544c61: ret 
0x544c62: pop edi
0x544c63: mov al, 1
0x544c65: pop esi
0x544c66: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
