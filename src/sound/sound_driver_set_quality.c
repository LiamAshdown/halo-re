// sound_driver_set_quality  (Ghidra: FUN_005480f0)
// address 0x5480f0, size 122 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/sound_types_notes.md misattribution note: "0x5480f0 FUN_005480f0: the
//   driver set_quality slot (0x38), not a general environment setter."; types/sound.h sound_driver
//   lists this address at slot 0x38 (`set_quality(int32_t unknown, uint8_t eax_enabled, int32_t
//   quality)`); directsound_quality (0x00746128) and sound_effect_object.mode (0x04,
//   sound_effect_object_mode) match by offset; reuses sound_effects_object_reinitialize
//   (0x5514d0, already named) and FUN_00548200 (0x548200, this batch).
// register convention: matches the sound_driver.set_quality signature directly: three plain stack
//   parameters (param_1 unused here beyond forwarding, eax_enabled, quality).
// blam-cc: stack -> (unknown, eax_enabled, quality)

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern int32_t directsound_quality;         // 0x00746128
extern uint8_t directsound_eax_enabled;      // 0x00746121
extern uint8_t directsound_eax_available;    // 0x00746120
extern sound_effect_object *global_sound_effect_object; // 0x00721f24

extern void sound_effects_object_reinitialize(int enable); // 0x5514d0
extern void sound_driver_set_eax_enabled(uint8_t eax_enabled, uint8_t force); // 0x548200, blam-cc: stack, CL

// blam-cc: stack -> (unknown, eax_enabled, quality)
// Clamps and stores the sound quality level [0, 2] (default 1), reinitializes the EAX effects
// object if the requested eax_enabled state differs from whether EAX is actually active, then
// forwards the first argument to sound_driver_set_eax_enabled, forcing the device reopen (CL = 1)
// when the quality changed or the effects object was reinitialized.
// Phase-4 review (disassembly below): the draft always passed force = 0.
void sound_driver_set_quality(int32_t unknown, uint8_t eax_enabled, int32_t quality)
{
    uint8_t eax_currently_active;
    uint8_t force = 0;

    if (quality < 0 || quality > 2) {
        quality = 1;
    }
    if (directsound_quality != quality) {
        directsound_quality = quality;
        force = 1;
    }

    if (directsound_eax_enabled == 0 || directsound_eax_available == 0 || global_sound_effect_object == 0 ||
        (global_sound_effect_object->mode != _sound_effect_object_eax1 &&
         global_sound_effect_object->mode != _sound_effect_object_eax2 &&
         global_sound_effect_object->mode != _sound_effect_object_eax3)) {
        eax_currently_active = 0;
    } else {
        eax_currently_active = 1;
    }

    if (eax_enabled != eax_currently_active) {
        sound_effects_object_reinitialize(eax_enabled);
        force = 1;
    }

    sound_driver_set_eax_enabled((uint8_t)unknown, force);
}

#if 0
Original Ghidra decompilation (0x5480f0):

void FUN_005480f0(undefined4 param_1,byte param_2,int param_3)

{
  int iVar1;
  byte bVar2;

  if ((param_3 < 0) || (2 < param_3)) {
    param_3 = 1;
  }
  if (DAT_00746128 != param_3) {
    DAT_00746128 = param_3;
  }
  if ((((DAT_00746121 == '\0') || (DAT_00746120 == '\0')) || (DAT_00721f24 == 0)) ||
     (((iVar1 = *(int *)(DAT_00721f24 + 4), iVar1 != 0 && (iVar1 != 1)) && (iVar1 != 2)))) {
    bVar2 = 0;
  }
  else {
    bVar2 = 1;
  }
  if (param_2 != bVar2) {
    sound_effects_object_reinitialize((uint)param_2);
  }
  FUN_00548200(param_1);
  return;
}

Disassembly (0x5480f0..0x54816a, capstone; phase-4 review):

0x5480f0: mov eax, dword ptr [esp + 0xc]
0x5480f4: xor cl, cl
0x5480f6: test eax, eax
0x5480f8: jl 0x5480ff
0x5480fa: cmp eax, 2
0x5480fd: jle 0x548104
0x5480ff: mov eax, 1
0x548104: cmp dword ptr [0x746128], eax
0x54810a: je 0x548113
0x54810c: mov dword ptr [0x746128], eax
0x548111: mov cl, 1
0x548113: mov al, byte ptr [0x746121]
0x548118: test al, al
0x54811a: je 0x548146
0x54811c: mov al, byte ptr [0x746120]
0x548121: test al, al
0x548123: je 0x548146
0x548125: mov eax, dword ptr [0x721f24]
0x54812a: test eax, eax
0x54812c: je 0x548146
0x54812e: mov eax, dword ptr [eax + 4]
0x548131: test eax, eax
0x548133: je 0x54813f
0x548135: cmp eax, 1
0x548138: je 0x54813f
0x54813a: cmp eax, 2
0x54813d: jne 0x548146
0x54813f: mov eax, 1
0x548144: jmp 0x548148
0x548146: xor eax, eax
0x548148: mov dl, byte ptr [esp + 8]
0x54814c: cmp dl, al
0x54814e: je 0x54815e
0x548150: movzx eax, dl
0x548153: push eax
0x548154: call 0x5514d0
0x548159: add esp, 4
0x54815c: mov cl, 1
0x54815e: mov edx, dword ptr [esp + 4]
0x548162: push edx
0x548163: call 0x548200
0x548168: pop ecx
0x548169: ret 
#endif
