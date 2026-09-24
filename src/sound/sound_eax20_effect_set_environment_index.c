// sound_eax20_effect_set_environment_index  (Ghidra: missed_54ff30; 0 callers in this module --
// reached only through the EAX2 vtable's set_environment_index slot, 0x00671d04+0x18)
// address 0x54ff30, size 49 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: confirmed by reading .rdata at 0x00671d04 (EAX2 vtable): +0x18 == 0x0054ff30.
//   types/sound.h sound_effect_object_vtable.set_environment_index comment; the property set is
//   read from `this+0x1c`, i.e. sound_eax_effect_object.channel_property_sets[0] ("slot 0 also
//   carries the listener properties", types/sound.h), exactly as sound_eax20_effect_apply_listener.c
//   (0x54fa80, this module) does -- not sound_effect_object.property_set (0x18), which EAX1's
//   siblings (0x54edf0/0x54ee30, this batch) use instead; property id 0xb is EAX 2.0's
//   environment property, on the listener GUID; reuses sound_eax20_listener_property_guid
//   (0x0064e2f0, this module).
// register convention: __thiscall (ECX -> this), stack -> environment.
// blam-cc: ECX -> this, stack -> environment
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. Unlike sound_eax1_effect_set_environment_index.c
// (0x54edf0, this batch), the CommitDeferredSettings call here is unconditional (no success
// check on the Set() result).

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax20_listener_property_guid[16]; // 0x0064e2f0
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this, stack -> environment
// Sets the EAX 2.0 listener's environment (id 0xb) property, then unconditionally commits
// deferred 3D listener settings.
void __thiscall sound_eax20_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    void *property_set = this_object->channel_property_sets[0]; // +0x1c: slot 0 carries the listener properties
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)property_set)[0x10 / 4];

    set(property_set, sound_eax20_listener_property_guid, 0xb, 0, 0, &environment, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
}

#if 0
Original Ghidra decompilation (0x54ff30):

void missed_54ff30(undefined4 param_1)

{
  int in_ECX;

  (**(code **)(**(int **)(in_ECX + 0x1c) + 0x10))
            (*(int **)(in_ECX + 0x1c),&DAT_0064e2f0,0xb,0,0,&param_1,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return;
}

Disassembly (0x54ff30..0x54ff5e; phase-4 review):

0x54ff30: mov eax, dword ptr [esp + 4]
0x54ff34: push 4
0x54ff36: lea edx, [esp + 8]
0x54ff3a: push edx
0x54ff3b: push 0
0x54ff3d: push 0
0x54ff3f: push 0xb
0x54ff41: mov dword ptr [esp + 0x18], eax
0x54ff45: mov eax, dword ptr [ecx + 0x1c]
0x54ff48: mov ecx, dword ptr [eax]
0x54ff4a: push 0x64e2f0
0x54ff4f: push eax
0x54ff50: call dword ptr [ecx + 0x10]
0x54ff53: mov eax, dword ptr [0x746114]
0x54ff58: mov ecx, dword ptr [eax]
0x54ff5a: push eax
0x54ff5b: call dword ptr [ecx + 0x44]
0x54ff5e: ret 4
#endif
