// sound_eax20_effect_set_room_gain  (Ghidra: missed_54ff70; 0 callers in this module -- reached
// only through the EAX2 vtable's set_room_gain slot, 0x00671d04+0x20)
// address 0x54ff70, size 117 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: confirmed by reading .rdata at 0x00671d04 (EAX2 vtable): +0x20 == 0x0054ff70.
//   types/sound.h sound_effect_object_vtable.set_room_gain comment; the property set is read
//   from `this+0x1c` (sound_eax_effect_object.channel_property_sets[0], "slot 0 also carries the
//   listener properties"), exactly as sound_eax20_effect_set_environment_index.c (0x54ff30, this
//   batch); property id 9 is EAX 2.0's room gain property, on the listener GUID
//   (sound_eax20_listener_property_guid, 0x0064e2f0, this module); the constants 0.0f (0x672ac0)
//   and 10000.0f (0x672e10) were read from .rdata. Unlike sound_gain_to_directsound_volume.c
//   (0x54ee70, this module), this does not call that helper -- it inlines a *linear* mapping
//   (gain * 12000 - 10000, truncated), not a log10 one; confirmed by disassembly (no FYL2X here).
// register convention: __thiscall (ECX -> this), stack -> gain.
// blam-cc: ECX -> this, stack -> gain
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block. Ghidra's own decompile called an unresolved `__ftol()` with no visible operand;
// the disassembly shows the operand is `gain * 12000.0 - 10000.0` computed on the x87 stack right
// before the call (12000.0 itself is built at runtime as abs(-10000) + abs(2000) via two cdq/xor/
// sub idioms, always evaluating to the constant 12000). The CommitDeferredSettings call here is
// unconditional, unlike sound_eax1_effect_set_room_gain.c (0x54ee30, this batch).

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax20_listener_property_guid[16]; // 0x0064e2f0
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this, stack -> gain
// Converts `gain` to a millibel-ish value with a linear (not log10) mapping -- gain*12000-10000,
// clamped to int32 by truncation, 0 for gain == 0.0 mapping to k_sound_minimum_volume -- sets the
// EAX 2.0 listener's room gain (id 9) property, then unconditionally commits deferred 3D listener
// settings.
void __thiscall sound_eax20_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
{
    void *property_set = this_object->channel_property_sets[0]; // +0x1c: slot 0 carries the listener properties
    sound_property_set_fn set;
    int32_t value;

    if (gain != 0.0f) {
        value = (int32_t)((double)gain * 12000.0 - 10000.0);
    } else {
        value = k_sound_minimum_volume;
    }

    set = (sound_property_set_fn)(*(void ***)property_set)[0x10 / 4];
    set(property_set, sound_eax20_listener_property_guid, 9, 0, 0, &value, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
}

#if 0
Original Ghidra decompilation (0x54ff70):

void missed_54ff70(float param_1)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = 0xffffd8f0;
  if (param_1 != 0.0) {
    uVar1 = __ftol();
  }
  param_1 = (float)uVar1;
  (**(code **)(**(int **)(in_ECX + 0x1c) + 0x10))
            (*(int **)(in_ECX + 0x1c),&DAT_0064e2f0,9,0,0,&param_1,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return;
}

Disassembly (0x54ff70..0x54ffe1; phase-4 review):

0x54ff70: push ecx
0x54ff71: fld dword ptr [0x672ac0]
0x54ff77: push esi
0x54ff78: fld dword ptr [esp + 0xc]
0x54ff7c: mov esi, ecx
0x54ff7e: fucompp
0x54ff80: fnstsw ax
0x54ff82: test ah, 0x44
0x54ff85: mov eax, 0xffffd8f0
0x54ff8a: jnp 0x54ffb6
0x54ff8c: cdq
0x54ff8d: mov ecx, eax
0x54ff8f: xor ecx, edx
0x54ff91: sub ecx, edx
0x54ff93: mov eax, 0x7d0
0x54ff98: cdq
0x54ff99: xor eax, edx
0x54ff9b: sub eax, edx
0x54ff9d: add ecx, eax
0x54ff9f: mov dword ptr [esp + 4], ecx
0x54ffa3: fild dword ptr [esp + 4]
0x54ffa7: fmul dword ptr [esp + 0xc]
0x54ffab: fsub dword ptr [0x672e10]
0x54ffb1: call 0x6391b4
0x54ffb6: push 4
0x54ffb8: lea ecx, [esp + 0x10]
0x54ffbc: push ecx
0x54ffbd: push 0
0x54ffbf: push 0
0x54ffc1: push 9
0x54ffc3: mov dword ptr [esp + 0x20], eax
0x54ffc7: mov eax, dword ptr [esi + 0x1c]
0x54ffca: mov edx, dword ptr [eax]
0x54ffcc: push 0x64e2f0
0x54ffd1: push eax
0x54ffd2: call dword ptr [edx + 0x10]
0x54ffd5: mov eax, dword ptr [0x746114]
0x54ffda: mov edx, dword ptr [eax]
0x54ffdc: push eax
0x54ffdd: call dword ptr [edx + 0x44]
#endif
