// sound_compare_priority  (Ghidra: FUN_0054c6b0, still unnamed there)
// address 0x54c6b0, size 146 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md "Compares two playback channels by sound-class
// priority (and distance as a tiebreaker) to decide which is more important."; the two
// tag-lookup chains both index sound_class_definitions[...].priority (0x0069eae0+0xa ==
// 0x0069eaea, types/sound.h).
// register convention: EAX -> sound_a, ECX -> sound_b, stack -> distance_a_squared.

// Phase-4 review (disassembly appended below): confirmed. Class priority is larger = more
//   important: returns 1 when sound_b's priority exceeds sound_a's, or they tie and sound_a
//   (EAX) is farther from its listener than `distance_a_squared`; i.e. "sound_b outranks
//   sound_a". The distance recompute uses sound_a's own location in ECX.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0

extern float sound_location_distance_squared(int16_t listener_index, sound_location *location); // this module, 0x54bbd0

// blam-cc: EAX -> sound_a, ECX -> sound_b, stack -> distance_a_squared
// True if sound_a's class priority is more important (lower) than sound_b's, or if they tie and
// distance_a_squared is less than a freshly recomputed distance for sound_a.
uint32_t sound_compare_priority(datum_index sound_a, datum_index sound_b, float distance_a_squared)
{
    sound *a;
    sound *b;
    Sound *definition_a;
    Sound *definition_b;
    int16_t priority_a;
    int16_t priority_b;

    a = (sound *)((uint8_t *)sound_data->data + (sound_a & 0xffff) * sizeof(sound));
    b = (sound *)((uint8_t *)sound_data->data + (sound_b & 0xffff) * sizeof(sound));
    definition_a = (Sound *)tag_instances[a->definition_index & 0xffff].data;
    definition_b = (Sound *)tag_instances[b->definition_index & 0xffff].data;
    priority_a = sound_class_definitions[definition_a->sound_class].priority;
    priority_b = sound_class_definitions[definition_b->sound_class].priority;

    if (priority_a < priority_b) {
        return 1;
    }
    if (priority_a == priority_b &&
        distance_a_squared < sound_location_distance_squared(a->listener_index, &a->location)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x54c6b0):

undefined4 FUN_0054c6b0(float param_1)

{
  uint in_EAX;
  int iVar1;
  uint in_ECX;
  float10 fVar2;

  iVar1 = (in_EAX & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
  if (*(short *)(&DAT_0069eaea +
                *(short *)(*(int *)((*(uint *)(iVar1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                          4) * 0x2c) <
      *(short *)(&DAT_0069eaea +
                *(short *)(*(int *)((*(uint *)((in_ECX & 0xffff) * 0xb0 + 8 +
                                              *(int *)(DAT_007252c0 + 0x34)) & 0xffff) * 0x20 + 0x14
                                   + DAT_0087bc14) + 4) * 0x2c)) {
    return 1;
  }
  if ((*(short *)(&DAT_0069eaea +
                 *(short *)(*(int *)((*(uint *)((in_ECX & 0xffff) * 0xb0 + 8 +
                                               *(int *)(DAT_007252c0 + 0x34)) & 0xffff) * 0x20 +
                                     0x14 + DAT_0087bc14) + 4) * 0x2c) ==
       *(short *)(&DAT_0069eaea +
                 *(short *)(*(int *)((*(uint *)(iVar1 + 8) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) +
                           4) * 0x2c)) &&
     (fVar2 = (float10)FUN_0054bbd0((int)*(short *)(iVar1 + 6)), (float10)param_1 < fVar2)) {
    return 1;
  }
  return 0;
}

Disassembly (0x54c6b0..0x54c742, capstone; phase-4 review):

0x54c6b0: mov edx, dword ptr [0x7252c0]
0x54c6b6: mov edx, dword ptr [edx + 0x34]
0x54c6b9: and ecx, 0xffff
0x54c6bf: imul ecx, ecx, 0xb0
0x54c6c5: and eax, 0xffff
0x54c6ca: imul eax, eax, 0xb0
0x54c6d0: add eax, edx
0x54c6d2: push esi
0x54c6d3: mov esi, dword ptr [ecx + edx + 8]
0x54c6d7: mov edx, dword ptr [eax + 8]
0x54c6da: mov ecx, dword ptr [0x87bc14]
0x54c6e0: and edx, 0xffff
0x54c6e6: and esi, 0xffff
0x54c6ec: shl edx, 5
0x54c6ef: mov edx, dword ptr [edx + ecx + 0x14]
0x54c6f3: movsx edx, word ptr [edx + 4]
0x54c6f7: shl esi, 5
0x54c6fa: imul edx, edx, 0x2c
0x54c6fd: mov esi, dword ptr [esi + ecx + 0x14]
0x54c701: movsx ecx, word ptr [esi + 4]
0x54c705: mov dx, word ptr [edx + 0x69eaea]
0x54c70c: imul ecx, ecx, 0x2c
0x54c70f: mov cx, word ptr [ecx + 0x69eaea]
0x54c716: cmp cx, dx
0x54c719: pop esi
0x54c71a: jg 0x54c73c
0x54c71c: jne 0x54c739
0x54c71e: lea ecx, [eax + 0x14]
0x54c721: movsx eax, word ptr [eax + 6]
0x54c725: push eax
0x54c726: call 0x54bbd0
0x54c72b: add esp, 4
0x54c72e: fcomp dword ptr [esp + 4]
0x54c732: fnstsw ax
0x54c734: test ah, 0x41
0x54c737: je 0x54c73c
0x54c739: xor eax, eax
0x54c73b: ret 
0x54c73c: mov eax, 1
0x54c741: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
