// sound_eax30_effect_set_room_gain  (Ghidra: missed_5511c0; 0 callers in this module -- reached
// only through the EAX3 vtable's set_room_gain slot, 0x00671d28+0x20)
// address 0x5511c0, size 117 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: confirmed by reading .rdata at 0x00671d28 (EAX3 vtable): +0x20 == 0x005511c0.
//   Byte-for-byte identical arithmetic to sound_eax20_effect_set_room_gain.c (0x54ff70, this
//   batch) -- same 0.0f/10000.0f constants (0x672ac0/0x672e10), same linear gain*12000-10000
//   mapping, same lack of a call to sound_gain_to_directsound_volume.c (0x54ee70, this module) --
//   differing only in the GUID (sound_eax30_listener_property_guid, 0x0064e310, reused from
//   sound_eax30_effect_apply_listener.c, this module) and the property id (0xe, EAX 3.0's room
//   gain property, vs EAX 2.0's 9). The property set is read from `this+0x1c`
//   (sound_eax_effect_object.channel_property_sets[0]).
// register convention: __thiscall (ECX -> this), stack -> gain.
// blam-cc: ECX -> this_object, stack -> gain
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. The CommitDeferredSettings call is unconditional.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax30_listener_property_guid[16]; // 0x0064e310
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this_object, stack -> gain
// FIXED (register inputs, objdump): note phrasing only -- the note said "this" but the parameter
// is named `this_object`, so the checker's alias matching found no mapping at all.
// Converts `gain` to a millibel-ish value with a linear (not log10) mapping -- gain*12000-10000,
// clamped to int32 by truncation, k_sound_minimum_volume for gain == 0.0 -- sets the EAX 3.0
// listener's room gain (id 0xe) property, then unconditionally commits deferred 3D listener
// settings.
void __thiscall sound_eax30_effect_set_room_gain(sound_eax_effect_object *this_object, float gain)
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
    set(property_set, sound_eax30_listener_property_guid, 0xe, 0, 0, &value, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
}

#if 0
Original Ghidra decompilation (0x5511c0):

void missed_5511c0(float param_1)

{
  undefined4 uVar1;
  int in_ECX;

  uVar1 = 0xffffd8f0;
  if (param_1 != 0.0) {
    uVar1 = __ftol();
  }
  param_1 = (float)uVar1;
  (**(code **)(**(int **)(in_ECX + 0x1c) + 0x10))
            (*(int **)(in_ECX + 0x1c),&DAT_0064e310,0xe,0,0,&param_1,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return;
}

Disassembly (0x5511c0..0x551232; phase-4 review):

0x5511c0: push ecx
0x5511c1: fld dword ptr [0x672ac0]
0x5511c7: push esi
0x5511c8: fld dword ptr [esp + 0xc]
0x5511cc: mov esi, ecx
0x5511ce: fucompp
0x5511d0: fnstsw ax
0x5511d2: test ah, 0x44
0x5511d5: mov eax, 0xffffd8f0
0x5511da: jnp 0x551206
0x5511dc: cdq
0x5511dd: mov ecx, eax
0x5511df: xor ecx, edx
0x5511e1: sub ecx, edx
0x5511e3: mov eax, 0x7d0
0x5511e8: cdq
0x5511e9: xor eax, edx
0x5511eb: sub eax, edx
0x5511ed: add ecx, eax
0x5511ef: mov dword ptr [esp + 4], ecx
0x5511f3: fild dword ptr [esp + 4]
0x5511f7: fmul dword ptr [esp + 0xc]
0x5511fb: fsub dword ptr [0x672e10]
0x551201: call 0x6391b4
0x551206: push 4
0x551208: lea ecx, [esp + 0x10]
0x55120c: push ecx
0x55120d: push 0
0x55120f: push 0
0x551211: push 0xe
0x551213: mov dword ptr [esp + 0x20], eax
0x551217: mov eax, dword ptr [esi + 0x1c]
0x55121a: mov edx, dword ptr [eax]
0x55121c: push 0x64e310
0x551221: push eax
0x551222: call dword ptr [edx + 0x10]
0x551225: mov eax, dword ptr [0x746114]
0x55122a: mov edx, dword ptr [eax]
0x55122c: push eax
0x55122d: call dword ptr [edx + 0x44]
0x551230: pop esi
0x551231: pop ecx
0x551232: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
