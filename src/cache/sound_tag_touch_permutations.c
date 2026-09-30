// sound_tag_touch_permutations  (Ghidra: FUN_00444a60; renamed -- not Bungie-attested, chosen to
// match the cache_functions.md summary: "Forces every sound permutation referenced by a tag's
// pitch ranges into the sound cache")
// address 0x444a60, size 134 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: types/tags.h Sound (pitch_ranges TagReflexive at 0x98/0x9c, confirmed directly by
// this function) and SoundPitchRange (actual_permutation_count 0x2c, permutations TagReflexive
// at 0x3c/0x40, size 0x48) layouts; SoundPermutation size 0x7c (types/cache.h). Ghidra's own
// decompile already resolves in_EAX as the tag id and both loop bodies correctly; only the
// third, register-passed call argument was confirmed by raw disassembly (objdump -d -M intel
// --start-address=0x444a60 --stop-address=0x444af0 bin/halo.exe), which shows `xor bl,bl` /
// `lea edi,[ebx+j*0x7c]` immediately before every call 0x443e10 -- matching sound_cache_touch's
// own (EAX,ECX,EBX,EDI) register convention exactly (allocate_if_missing=1, lock=0,
// wait_until_loaded=0, permutation=&permutations[j]).
// register convention: packed TagID (index low 16, salt high 16) in EAX (in_EAX).

#include "tags.h"
#include "cache.h"
#include "fn_cache.h"

extern tag_instance *tag_instances; // 0x0087bc14

extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock,
    uint8_t wait_until_loaded, SoundPermutation *permutation); // blam-cc: wait_until_loaded in
    // EBX, permutation in EDI; this module, sound_cache_touch.c

// blam-cc: tag in EAX
// Walks every SoundPitchRange on a Sound tag and, for each, every one of its
// actual_permutation_count live permutations, forcing each into the sound cache via
// sound_cache_touch(allocate_if_missing = 1, lock = 0, wait_until_loaded = 0, permutation).
void sound_tag_touch_permutations(TagID tag)
{
    Sound *sound;
    int32_t pitch_range_index;
    SoundPitchRange *pitch_range;
    int16_t permutation_index;
    SoundPermutation *permutations;

    sound = (Sound *)tag_instances[tag.index].data;

    if (0 < (int32_t)sound->pitch_ranges.count) {
        pitch_range_index = 0;
        do {
            pitch_range = &((SoundPitchRange *)sound->pitch_ranges.pointer)[pitch_range_index];
            permutations = (SoundPermutation *)pitch_range->permutations.pointer;

            if (0 < pitch_range->actual_permutation_count) {
                permutation_index = 0;
                do {
                    sound_cache_touch(1, 0, 0, &permutations[permutation_index]);
                    permutation_index = permutation_index + 1;
                } while (permutation_index < (int16_t)pitch_range->actual_permutation_count);
            }

            pitch_range_index = pitch_range_index + 1;
        } while (pitch_range_index < (int32_t)sound->pitch_ranges.count);
    }
    return;
}

#if 0
Original Ghidra decompilation (0x444a60):

void FUN_00444a60(void)

{
  int iVar1;
  short sVar2;
  uint in_EAX;
  int iVar3;
  short sVar4;

  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = 0;
  sVar2 = 0;
  if (0 < *(int *)(iVar1 + 0x98)) {
    do {
      iVar3 = *(int *)(iVar1 + 0x9c) + iVar3 * 0x48;
      sVar4 = 0;
      if (0 < *(short *)(iVar3 + 0x2c)) {
        do {
          FUN_00443e10(1,0);
          sVar4 = sVar4 + 1;
        } while (sVar4 < *(short *)(iVar3 + 0x2c));
      }
      sVar2 = sVar2 + 1;
      iVar3 = (int)sVar2;
    } while (iVar3 < *(int *)(iVar1 + 0x98));
  }
  return;
}

Raw disassembly (0x444a60..0x444ae5) recovering the elided EDI/EBX register arguments to
FUN_00443e10 (sound_cache_touch):

00444aa4:  mov ebx,[ebp+0x40]       ; pitch_range->permutations.pointer
00444aa7:  movsx edi,si             ; j (permutation index)
00444aaa:  imul edi,edi,0x7c        ; j * sizeof(SoundPermutation)
00444aad:  push 0x0                 ; pushed first => second stack arg (lock)
00444aaf:  add edi,ebx              ; edi = &permutations[j]      (unaff_EDI)
00444ab1:  push 0x1                 ; pushed last  => first stack arg (allocate_if_missing)
00444ab3:  xor bl,bl                ; wait_until_loaded = 0        (unaff_BL)
00444ab5:  call 0x443e10            ; sound_cache_touch(1, 0, 0, &permutations[j])
#endif
