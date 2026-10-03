// sound_eax30_effect_set_environment_index  (Ghidra: missed_551180; 0 callers in this module --
// reached only through the EAX3 vtable's set_environment_index slot, 0x00671d28+0x18)
// address 0x551180, size 49 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: confirmed by reading .rdata at 0x00671d28 (EAX3 vtable): +0x18 == 0x00551180.
//   Identical structure to sound_eax20_effect_set_environment_index.c (0x54ff30, this batch)
//   except the GUID (sound_eax30_listener_property_guid, 0x0064e310, reused from
//   sound_eax30_effect_apply_listener.c, this module) and the property id (2, EAX 3.0's
//   environment property, vs EAX 2.0's 0xb). The property set is read from `this+0x1c`
//   (sound_eax_effect_object.channel_property_sets[0]).
// register convention: __thiscall (ECX -> this), stack -> environment.
// blam-cc: ECX -> this_object, stack -> environment
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found. The CommitDeferredSettings call is unconditional.
// FIXED (register inputs, objdump): ECX carries this_object (read at 0x551195, mov eax,[ecx+0x1c]);
// the notes said "ECX -> this" but the parameter is named this_object, so it did not parse.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax30_listener_property_guid[16]; // 0x0064e310
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this_object, stack -> environment
// Sets the EAX 3.0 listener's environment (id 2) property, then unconditionally commits deferred
// 3D listener settings.
void __thiscall sound_eax30_effect_set_environment_index(sound_eax_effect_object *this_object, int32_t environment)
{
    void *property_set = this_object->channel_property_sets[0]; // +0x1c: slot 0 carries the listener properties
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)property_set)[0x10 / 4];

    set(property_set, sound_eax30_listener_property_guid, 2, 0, 0, &environment, 4);
    ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
}

#if 0
Original Ghidra decompilation (0x551180):

void missed_551180(undefined4 param_1)

{
  int in_ECX;

  (**(code **)(**(int **)(in_ECX + 0x1c) + 0x10))
            (*(int **)(in_ECX + 0x1c),&DAT_0064e310,2,0,0,&param_1,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return;
}

Disassembly (0x551180..0x5511ae; phase-4 review):

0x551180: mov eax, dword ptr [esp + 4]
0x551184: push 4
0x551186: lea edx, [esp + 8]
0x55118a: push edx
0x55118b: push 0
0x55118d: push 0
0x55118f: push 2
0x551191: mov dword ptr [esp + 0x18], eax
0x551195: mov eax, dword ptr [ecx + 0x1c]
0x551198: mov ecx, dword ptr [eax]
0x55119a: push 0x64e310
0x55119f: push eax
0x5511a0: call dword ptr [ecx + 0x10]
0x5511a3: mov eax, dword ptr [0x746114]
0x5511a8: mov ecx, dword ptr [eax]
0x5511aa: push eax
0x5511ab: call dword ptr [ecx + 0x44]
0x5511ae: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
