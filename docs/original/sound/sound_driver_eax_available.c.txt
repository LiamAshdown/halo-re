// sound_driver_eax_available  (Ghidra: missed_5482a0; 0 callers in this module -- reached only
// through the DirectSound driver's eax_available vtable slot, sound_driver.eax_available 0x3c)
// address 0x5482a0, size 53 bytes
// name confidence: 0.6   rewrite confidence: 0.95
// evidence: out/phase4/sound_types_notes.md driver slot table "0x3c 0x5482a0" and
//   types/sound.h sound_driver.eax_available comment; global_sound_effect_object and its mode
//   field (0x04, sound_effect_object_mode, types/sound.h) match by offset and by the three
//   accepted values (eax1/eax2/eax3 == 0/1/2).
// register convention: plain __cdecl, no parameters.
// blam-cc: void -> uint8_t
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t directsound_eax_enabled;             // 0x00746121
extern uint8_t directsound_eax_available;           // 0x00746120
extern sound_effect_object *global_sound_effect_object; // 0x00721f24

// blam-cc: void -> uint8_t
// True when EAX hardware support was detected, EAX is currently enabled, an effects object
// exists, and its mode is one of EAX1/EAX2/EAX3.
uint8_t sound_driver_eax_available(void)
{
    if (directsound_eax_enabled != 0 && directsound_eax_available != 0 && global_sound_effect_object != 0) {
        int32_t mode = global_sound_effect_object->mode;

        if (mode == _sound_effect_object_eax1 || mode == _sound_effect_object_eax2 ||
            mode == _sound_effect_object_eax3) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x5482a0):

undefined4 missed_5482a0(void)

{
  int iVar1;

  if ((((DAT_00746121 != '\0') && (DAT_00746120 != '\0')) && (DAT_00721f24 != 0)) &&
     (((iVar1 = *(int *)(DAT_00721f24 + 4), iVar1 == 0 || (iVar1 == 1)) || (iVar1 == 2)))) {
    return 1;
  }
  return 0;
}

Disassembly (0x5482a0..0x5482d4; phase-4 review):

0x5482a0: mov al, byte ptr [0x746121]
0x5482a5: test al, al
0x5482a7: je 0x5482d2
0x5482a9: mov al, byte ptr [0x746120]
0x5482ae: test al, al
0x5482b0: je 0x5482d2
0x5482b2: mov eax, dword ptr [0x721f24]
0x5482b7: test eax, eax
0x5482b9: je 0x5482d2
0x5482bb: mov eax, dword ptr [eax + 4]
0x5482be: test eax, eax
0x5482c0: je 0x5482cc
0x5482c2: cmp eax, 1
0x5482c5: je 0x5482cc
0x5482c7: cmp eax, 2
0x5482ca: jne 0x5482d2
0x5482cc: mov eax, 1
0x5482d1: ret
0x5482d2: xor eax, eax
0x5482d4: ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
