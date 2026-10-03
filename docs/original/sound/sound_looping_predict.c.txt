// sound_looping_predict  (Ghidra: FUN_00544000)  [earlier name sound_looping_preload_tracks]
// address 0x544000, size 130 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/sound_types_notes.md misattribution note for this exact address: "the
//   arithmetic is SoundLooping.tracks (count 0x3c, pointer 0x40, stride 0xa0, loop.tag_id at
//   +0x4c) and Sound.pitch_ranges, so these are looping sound definition helpers (music class
//   test, preload)" -- superseding out/phase4/sound_functions.md's own "object attached parts"
//   summary. types/tags.h SoundLooping.tracks (TagReflexive 0x3c) and SoundLoopingTrack.loop
//   (TagDependency, tag_id at +0x4c) confirm the offsets; Sound.pitch_ranges (0x98) and
//   SoundPitchRange.permutations (0x3c) confirm the inner check.
// register convention: SoundLooping tag handle in EAX (in_EAX).
// blam-cc: EAX -> looping_definition

// Phase-4 review rename: the hs command sound_looping_predict <looping_sound>, called only from
// the hs primitive table (0x47fe4e). Review fix: sound_cache_touch gets (1, 0, BL 0, EDI = the
// first permutation of the loop sound), the draft passed NULL.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, src/cache/sound_cache_touch.c, blam-cc: stack, stack, BL, EDI

// blam-cc: EAX -> looping_definition
// For every track of a SoundLooping definition whose "loop" sound has exactly one pitch range
// with loaded permutations, touches the sound cache to keep it resident.
void sound_looping_predict(datum_index looping_definition)
{
    if (looping_definition != k_datum_index_none) {
        SoundLooping *definition = (SoundLooping *)tag_instances[looping_definition & 0xffff].data;
        int32_t i;

        for (i = 0; i < (int32_t)definition->tracks.count; i++) {
            SoundLoopingTrack *track = (SoundLoopingTrack *)definition->tracks.pointer + i;
            uint32_t loop_sound = *(uint32_t *)&track->loop.tag_id;

            if (loop_sound != 0xffffffff) {
                Sound *loop_tag = (Sound *)tag_instances[loop_sound & 0xffff].data;

                if (loop_tag->pitch_ranges.count == 1 &&
                    ((SoundPitchRange *)loop_tag->pitch_ranges.pointer)->permutations.count != 0) {
                    // EDI = the first permutation of the single pitch range
                    sound_cache_touch(1, 0, 0, (SoundPermutation *)((SoundPitchRange *)loop_tag->pitch_ranges.pointer)->
                        permutations.pointer);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x544000):

void FUN_00544000(void)

{
  int iVar1;
  uint uVar2;
  uint in_EAX;
  int iVar3;
  short sVar4;

  if (in_EAX != 0xffffffff) {
    iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    sVar4 = 0;
    if (0 < *(int *)(iVar1 + 0x3c)) {
      iVar3 = 0;
      do {
        uVar2 = *(uint *)(iVar3 * 0xa0 + *(int *)(iVar1 + 0x40) + 0x4c);
        if (((uVar2 != 0xffffffff) &&
            (iVar3 = *(int *)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14),
            *(int *)(iVar3 + 0x98) == 1)) && (*(int *)(*(int *)(iVar3 + 0x9c) + 0x3c) != 0)) {
          sound_cache_touch(1,0);
        }
        sVar4 = sVar4 + 1;
        iVar3 = (int)sVar4;
      } while (iVar3 < *(int *)(iVar1 + 0x3c));
    }
  }
  return;
}

Disassembly (0x544000..0x544082, capstone; phase-4 review):

0x544000: cmp eax, -1
0x544003: je 0x544081
0x544005: mov ecx, dword ptr [0x87bc14]
0x54400b: and eax, 0xffff
0x544010: push ebp
0x544011: shl eax, 5
0x544014: push esi
0x544015: mov esi, dword ptr [eax + ecx + 0x14]
0x544019: mov eax, dword ptr [esi + 0x3c]
0x54401c: xor ebp, ebp
0x54401e: test eax, eax
0x544020: jle 0x54407f
0x544022: push ebx
0x544023: xor eax, eax
0x544025: push edi
0x544026: mov edx, dword ptr [esi + 0x40]
0x544029: lea eax, [eax + eax*4]
0x54402c: shl eax, 5
0x54402f: add eax, edx
0x544031: mov eax, dword ptr [eax + 0x4c]
0x544034: cmp eax, -1
0x544037: je 0x544072
0x544039: mov edx, dword ptr [0x87bc14]
0x54403f: and eax, 0xffff
0x544044: shl eax, 5
0x544047: mov eax, dword ptr [eax + edx + 0x14]
0x54404b: cmp dword ptr [eax + 0x98], 1
0x544052: jne 0x544072
0x544054: mov eax, dword ptr [eax + 0x9c]
0x54405a: mov ecx, dword ptr [eax + 0x3c]
0x54405d: test ecx, ecx
0x54405f: je 0x544072
0x544061: mov edi, dword ptr [eax + 0x40]
0x544064: push 0
0x544066: push 1
0x544068: xor bl, bl
0x54406a: call 0x443e10
0x54406f: add esp, 8
0x544072: mov ecx, dword ptr [esi + 0x3c]
0x544075: inc ebp
0x544076: movsx eax, bp
0x544079: cmp eax, ecx
0x54407b: jl 0x544026
0x54407d: pop edi
0x54407e: pop ebx
0x54407f: pop esi
0x544080: pop ebp
0x544081: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
