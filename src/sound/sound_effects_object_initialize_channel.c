// sound_effects_object_initialize_channel  (Ghidra: FUN_00551460, still unnamed there)
// address 0x551460, size 19 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: the call target (this_object->vtable + 8 bytes) is vtable slot 2, which
// types/sound.h's sound_effect_object_vtable documents as initialize_channel(this, channel_index)
// -- the same slot sound_effects_object_apply_all_channels.c (0x551480) calls with an explicit
// channel index. This function calls it with no visible channel index at all; out/phase4/
// sound_functions.md's summary ("returns the result of invoking the active sound effects
// object's vtable+8 method, or 0 if no effects object is active") does not resolve that either.
// register convention: EDX -> channel_index (see the note at the function).

#include "tags.h"
#include "memory.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern sound_effect_object *global_sound_effect_object; // 0x00721f24

// blam-cc: EDX -> channel_index
// Phase-4 review (disassembly 0x551460..0x551472): the channel index arrives in EDX and is
// pushed as the argument of the effects object's initialize_channel (vtable +8, __thiscall).
int32_t sound_effects_object_initialize_channel(int16_t channel_index)
{
    if (global_sound_effect_object != 0) {
        return global_sound_effect_object->vtable->initialize_channel(global_sound_effect_object, channel_index);
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x551460):

undefined4 FUN_00551460(void)

{
  undefined4 uVar1;

  uVar1 = 0;
  if (DAT_00721f24 != (int *)0x0) {
    uVar1 = (**(code **)(*DAT_00721f24 + 8))();
  }
  return uVar1;
}

Disassembly (0x551460..0x551473, capstone; phase-4 review):

0x551460: mov ecx, dword ptr [0x721f24]
0x551466: xor eax, eax
0x551468: test ecx, ecx
0x55146a: je 0x551472
0x55146c: mov eax, dword ptr [ecx]
0x55146e: push edx
0x55146f: call dword ptr [eax + 8]
0x551472: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
