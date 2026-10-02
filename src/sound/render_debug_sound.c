// render_debug_sound  (Ghidra: render_debug_sound, already named)
// address 0x54e6d0, size 97 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Formats a debug string with a playing sound's tag
// name and two float parameters when sound debug display is enabled."; the two floats are
// instance->location.obstruction/occlusion (types/sound.h sound_location 0x38/0x3c, sound+0x14
// -> instance+0x4c/0x50), matching that header's own note "printed by render_debug_sound
// (sound+0x4c/+0x50)"; the string is tag_instances[...].path (types/cache.h tag_instance, 0x10).
// register convention: EAX -> sound_handle.
// UNSURE: the formatted buffer (local_200[512]) is never used after the sprintf -- Ghidra shows
// no draw/store of it, so this rewrite preserves that as literally dead work rather than
// inventing a destination for it.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t debug_sound;         // 0x00724a4d
extern data_array *sound_data;      // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances; // 0x0087bc14


// blam-cc: EAX -> sound_handle
// When sound debug display is enabled, formats "<tag path>|n<obstruction> <occlusion>" for
// `sound_handle` into a scratch buffer.
void render_debug_sound(datum_index sound_handle)
{
    sound *instance;
    char buffer[512];

    if (debug_sound) {
        instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
        sprintf(buffer, "%s|n%f %f", tag_instances[instance->definition_index & 0xffff].path,
            (double)instance->location.obstruction, (double)instance->location.occlusion);
    }
}

#if 0
Original Ghidra decompilation (0x54e6d0):

void render_debug_sound(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;
  char local_200 [512];

  if (DAT_00724a4d != '\0') {
    iVar1 = (in_EAX & 0xffff) * 0xb0;
    iVar2 = iVar1 + *(int *)(DAT_007252c0 + 0x34);
    _sprintf(local_200,"%s|n%f %f",
             *(undefined4 *)(*(short *)(iVar2 + 8) * 0x20 + 0x10 + DAT_0087bc14),
             (double)*(float *)(iVar2 + 0x4c),
             (double)*(float *)(iVar1 + 0x50 + *(int *)(DAT_007252c0 + 0x34)));
  }
  return;
}

Disassembly (0x54e6d0..0x54e731, capstone; phase-4 review):

0x54e6d0: mov cl, byte ptr [0x724a4d]
0x54e6d6: sub esp, 0x200
0x54e6dc: test cl, cl
0x54e6de: je 0x54e72a
0x54e6e0: mov ecx, dword ptr [0x7252c0]
0x54e6e6: mov edx, dword ptr [ecx + 0x34]
0x54e6e9: and eax, 0xffff
0x54e6ee: imul eax, eax, 0xb0
0x54e6f4: fld dword ptr [eax + edx + 0x50]
0x54e6f8: add eax, edx
0x54e6fa: movsx edx, word ptr [eax + 8]
0x54e6fe: sub esp, 0x10
0x54e701: shl edx, 5
0x54e704: fstp qword ptr [esp + 8]
0x54e708: fld dword ptr [eax + 0x4c]
0x54e70b: mov eax, dword ptr [0x87bc14]
0x54e710: mov ecx, dword ptr [edx + eax + 0x10]
0x54e714: fstp qword ptr [esp]
0x54e717: push ecx
0x54e718: lea edx, [esp + 0x14]
0x54e71c: push 0x671998
0x54e721: push edx
0x54e722: call 0x623693
0x54e727: add esp, 0x1c
0x54e72a: add esp, 0x200
0x54e730: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
