// sound_definition_has_audible_permutations  (Ghidra: FUN_0054af10, still unnamed there)
// address 0x54af10, size 67 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Returns whether an object has loaded sound
// permutations for a sound class that is not currently muted." (the "object" in that summary is
// the Sound tag, not a game object: the only input is a tag id). tag_instances[index].data
// (types/cache.h tag_instance, 0x14) is read as Sound *; pitch_ranges (Sound 0x98, TagReflexive)
// and its first entry's permutations (SoundPitchRange 0x3c, TagReflexive) are checked for a
// nonzero count; the final term indexes sound_class_definitions[sound_class].muted
// (0x0069eae0 + 0x28 == 0x0069eb08, types/sound.h sound_class_definition).
// register convention: EAX -> sound_tag_id.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern tag_instance *tag_instances;                    // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

// blam-cc: EAX -> sound_tag_id
// True if the sound tag has at least one pitch range whose permutations are loaded, and its
// sound class is not currently muted.
uint32_t sound_definition_has_audible_permutations(TagID sound_tag_id)
{
    Sound *sound;
    SoundPitchRange *pitch_range;

    sound = (Sound *)tag_instances[sound_tag_id.index].data;
    if (sound->pitch_ranges.count != 0) {
        pitch_range = (SoundPitchRange *)sound->pitch_ranges.pointer;
        if (pitch_range->permutations.count != 0 &&
            sound_class_definitions[sound->sound_class].muted == 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x54af10):

undefined4 FUN_0054af10(void)

{
  int iVar1;
  uint in_EAX;

  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(int *)(iVar1 + 0x98) != 0) && (*(int *)(*(int *)(iVar1 + 0x9c) + 0x3c) != 0)) &&
     ((&DAT_0069eb08)[*(short *)(iVar1 + 4) * 0x2c] == '\0')) {
    return 1;
  }
  return 0;
}

Disassembly (0x54af10..0x54af53, capstone; phase-4 review):

0x54af10: mov ecx, dword ptr [0x87bc14]
0x54af16: and eax, 0xffff
0x54af1b: shl eax, 5
0x54af1e: mov eax, dword ptr [eax + ecx + 0x14]
0x54af22: mov ecx, dword ptr [eax + 0x98]
0x54af28: test ecx, ecx
0x54af2a: je 0x54af50
0x54af2c: mov edx, dword ptr [eax + 0x9c]
0x54af32: mov ecx, dword ptr [edx + 0x3c]
0x54af35: test ecx, ecx
0x54af37: je 0x54af50
0x54af39: movsx eax, word ptr [eax + 4]
0x54af3d: imul eax, eax, 0x2c
0x54af40: mov cl, byte ptr [eax + 0x69eb08]
0x54af46: test cl, cl
0x54af48: jne 0x54af50
0x54af4a: mov eax, 1
0x54af4f: ret 
0x54af50: xor eax, eax
0x54af52: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
