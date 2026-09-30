// sound_driver_set_eax_enabled  (Ghidra: FUN_00548200)
// address 0x548200, size 148 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Enables or disables EAX environmental audio
//   and resets the cached EAX reverb environment parameters to their defaults."; the eight
//   constants written (0069f514..0069f524) are exactly types/sound.h's own literal for
//   sound_driver_parameters: "{0, {22,2,2,2}, {22,2,2,2}, 0}"; directsound_hardware_mode
//   (0x0074612c) reset to -1; forwards into FUN_005494a0 (0x5494a0, this batch).
// register convention: requested eax_enabled state as the recognized stack parameter (param_1),
//   a "force" flag in CL (in_CL).
// blam-cc: stack -> eax_enabled, CL -> force
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t directsound_initialized;   // 0x007252e0
extern uint8_t directsound_eax_enabled;   // 0x00746121
extern sound_driver_parameters driver_parameters; // 0x0069f514
extern int32_t directsound_hardware_mode; // 0x0074612c


// blam-cc: stack -> eax_enabled, CL -> force
// If the sound driver is initialized and either the requested EAX state differs from the current
// one or `force` is set, updates directsound_eax_enabled, resets sound_driver_parameters and
// directsound_hardware_mode to their defaults, and reopens the sound device with them
// (sound_reopen_device, reached by `jmp` after overwriting the stack argument with 0x0069f514).
void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force)
{
    if (directsound_initialized == 0) {
        return;
    }

    if (eax_enabled == 0) {
        if (directsound_eax_enabled == 0 && force == 0) {
            return;
        }
        directsound_eax_enabled = 0;
    } else {
        if (directsound_eax_enabled != 0 && force == 0) {
            return;
        }
        directsound_eax_enabled = 1;
        driver_parameters.driver_index = 0;
    }

    driver_parameters.channel_counts[0] = 0x16;
    driver_parameters.channel_counts[1] = 2;
    driver_parameters.channel_counts[2] = 2;
    driver_parameters.channel_counts[3] = 2;
    driver_parameters.slot_counts[0] = 0x16;
    driver_parameters.slot_counts[1] = 2;
    driver_parameters.slot_counts[2] = 2;
    driver_parameters.slot_counts[3] = 2;
    directsound_hardware_mode = -1;

    sound_reopen_device(&driver_parameters);
}

#if 0
Original Ghidra decompilation (0x548200):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00548200(char param_1)

{
  char in_CL;

  if (DAT_007252e0 == '\0') {
    return;
  }
  if (param_1 == '\0') {
    if ((DAT_00746121 == '\0') && (in_CL == '\0')) {
      return;
    }
    DAT_00746121 = 0;
  }
  else {
    if ((DAT_00746121 != '\0') && (in_CL == '\0')) {
      return;
    }
    DAT_00746121 = 1;
    DAT_0069f514 = 0;
  }
  DAT_0069f516 = 0x16;
  DAT_0069f518 = 2;
  DAT_0069f51a = 2;
  DAT_0069f51c = 2;
  DAT_0069f51e = 0x16;
  DAT_0069f520 = 2;
  _DAT_0069f522 = 2;
  _DAT_0069f524 = 2;
  DAT_0074612c = 0xffffffff;
  FUN_005494a0();
  return;
}

Disassembly (0x548200..0x548294, capstone; phase-4 review):

0x548200: mov dl, byte ptr [0x7252e0]
0x548206: xor eax, eax
0x548208: cmp dl, al
0x54820a: je 0x548293
0x548210: cmp byte ptr [esp + 4], al
0x548214: mov dl, byte ptr [0x746121]
0x54821a: je 0x548233
0x54821c: cmp dl, al
0x54821e: je 0x548224
0x548220: cmp cl, al
0x548222: je 0x548293
0x548224: mov byte ptr [0x746121], 1
0x54822b: mov word ptr [0x69f514], ax
0x548231: jmp 0x548240
0x548233: cmp dl, al
0x548235: jne 0x54823b
0x548237: cmp cl, al
0x548239: je 0x548293
0x54823b: mov byte ptr [0x746121], al
0x548240: mov eax, 2
0x548245: mov ecx, 0x16
0x54824a: mov word ptr [0x69f516], cx
0x548251: mov word ptr [0x69f518], ax
0x548257: mov word ptr [0x69f51a], ax
0x54825d: mov word ptr [0x69f51c], ax
0x548263: mov word ptr [0x69f51e], cx
0x54826a: mov word ptr [0x69f520], ax
0x548270: mov word ptr [0x69f522], ax
0x548276: mov word ptr [0x69f524], ax
0x54827c: mov dword ptr [0x74612c], 0xffffffff
0x548286: mov dword ptr [esp + 4], 0x69f514
0x54828e: jmp 0x5494a0
0x548293: ret 
#endif
