// sound_definition_maximum_distance  (Ghidra: FUN_00545460)
// address 0x545460, size 54 bytes
// name confidence: 0.3   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md's own summary ("Returns an object's sound gain/scale
//   override, or its sound class's default scale when the object has none set") is superseded by
//   direct field-offset evidence: tag+0xc is Sound.maximum_distance (types/tags.h), and the
//   fallback global 0x0069eafc is sound_class_definitions[0].default_maximum_distance (0x0069eae0
//   + 0x1c, types/sound.h), stride 0x2c per class -- not a gain/scale field at all. This is the
//   effective-maximum-distance resolver: a Sound tag's own maximum_distance when set, otherwise
//   its sound class's fallback default_maximum_distance.
// register convention: Sound tag handle in EAX (in_EAX).
// blam-cc: EAX -> sound_definition
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

// blam-cc: EAX -> sound_definition
// Effective maximum audible distance for a Sound tag: its own maximum_distance when nonzero,
// otherwise its sound class's default_maximum_distance.
float sound_definition_maximum_distance(datum_index sound_definition)
{
    Sound *tag = (Sound *)tag_instances[sound_definition & 0xffff].data;
    float distance = tag->maximum_distance;

    if (distance == 0.0f) {
        distance = sound_class_definitions[tag->sound_class].default_maximum_distance;
    }

    return distance;
}

#if 0
Original Ghidra decompilation (0x545460):

float10 FUN_00545460(void)

{
  int iVar1;
  uint in_EAX;
  float10 fVar2;

  iVar1 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  fVar2 = (float10)*(float *)(iVar1 + 0xc);
  if (fVar2 == (float10)0.0) {
    fVar2 = (float10)*(float *)(&DAT_0069eafc + *(short *)(iVar1 + 4) * 0x2c);
  }
  return fVar2;
}

Disassembly (0x545460..0x545496, capstone; phase-4 review):

0x545460: mov ecx, dword ptr [0x87bc14]
0x545466: and eax, 0xffff
0x54546b: shl eax, 5
0x54546e: mov ecx, dword ptr [eax + ecx + 0x14]
0x545472: fld dword ptr [ecx + 0xc]
0x545475: fld dword ptr [0x672ac0]
0x54547b: fld st(1)
0x54547d: fucompp 
0x54547f: fnstsw ax
0x545481: test ah, 0x44
0x545484: jp 0x545495
0x545486: movsx edx, word ptr [ecx + 4]
0x54548a: fstp st(0)
0x54548c: imul edx, edx, 0x2c
0x54548f: fld dword ptr [edx + 0x69eafc]
0x545495: ret 
#endif
