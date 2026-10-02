// sound_effects_object_shutdown  (Ghidra: sound_effects_object_shutdown, already named, __cdecl)
// address 0x551420, size 49 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Releases and frees the current global sound effects
// object, if one exists."; types/sound.h sound_effect_object_state (0x00746130),
// global_sound_effect_object (0x00721f24).
// register convention: __cdecl, no parameters.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern sound_effect_object *global_sound_effect_object; // 0x00721f24
extern int16_t sound_effect_object_state;               // 0x00746130

extern void free(void *ptr); // 0x6277e8, MSVC CRT

void __cdecl sound_effects_object_shutdown(void)
{
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        free(global_sound_effect_object);
        global_sound_effect_object = 0;
        sound_effect_object_state = 0;
    }
}

#if 0
Original Ghidra decompilation (0x551420):

void __cdecl sound_effects_object_shutdown(void)

{
  if (DAT_00721f24 != (undefined4 *)0x0) {
    (**(code **)*DAT_00721f24)();
    _free(DAT_00721f24);
    DAT_00721f24 = (undefined4 *)0x0;
    DAT_00746130 = 0;
  }
  return;
}

Disassembly (0x551420..0x551451, capstone; phase-4 review):

0x551420: mov ecx, dword ptr [0x721f24]
0x551426: test ecx, ecx
0x551428: je 0x551450
0x55142a: mov eax, dword ptr [ecx]
0x55142c: call dword ptr [eax]
0x55142e: mov ecx, dword ptr [0x721f24]
0x551434: push ecx
0x551435: call 0x6277e8
0x55143a: add esp, 4
0x55143d: mov dword ptr [0x721f24], 0
0x551447: mov word ptr [0x746130], 0
0x551450: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
