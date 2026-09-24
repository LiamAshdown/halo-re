// sound_eax1_effect_set_environment_index  (Ghidra: missed_54edf0; 0 callers in this module --
// reached only through the EAX1 vtable's set_environment_index slot, 0x00671d4c+0x18)
// address 0x54edf0, size 53 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: confirmed by reading .rdata at 0x00671d4c (EAX1 vtable): +0x18 == 0x0054edf0.
//   types/sound.h sound_effect_object_vtable.set_environment_index comment ("0x18 ... int32_t
//   environment"); sound_effect_object.property_set (0x18, types/sound.h) matches by offset;
//   IKsPropertySet::Set is vtable slot 0x10 (per the module header's own vtable-slot note);
//   sound_eax_listener_property_guid (0x0064e2d0, EAX 1.0 listener property set) is reused from
//   sound_eax1_effect_apply_listener.c (this module); property id 1 is EAX 1.0's ENVIRONMENT
//   property (DSPROPERTY_EAX_ENVIRONMENT).
// register convention: __thiscall (ECX -> this_object), stack -> environment.
// blam-cc: ECX -> this_object, stack -> environment
// FIXED (register inputs, objdump): notes said "ECX -> this" but the parameter is named
// this_object, so the checker's alias match failed and ECX (read at 0x54ee05) looked unclaimed.
// Body already used this_object correctly; reworded only.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block. The CommitDeferredSettings call is conditional on Set() succeeding (`test eax,eax;
// jl` before it), which Ghidra's own decompile did not show; the decompile's `return 1` is also
// bogus (no `mov eax,1` exists in the assembly -- the driver's own prototype is void).

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern const uint8_t sound_eax_listener_property_guid[16]; // 0x0064e2d0, EAX 1.0 listener property set
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this_object, stack -> environment
// Sets the EAX 1.0 listener's ENVIRONMENT (id 1) property, then commits deferred 3D listener
// settings.
void __thiscall sound_eax1_effect_set_environment_index(sound_effect_object *this_object, int32_t environment)
{
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)this_object->property_set)[0x10 / 4];
    int32_t result = set(this_object->property_set, sound_eax_listener_property_guid, 1, 0, 0, &environment, 4);

    if (result >= 0) {
        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
    }
}

#if 0
Original Ghidra decompilation (0x54edf0):

undefined1 missed_54edf0(undefined4 param_1)

{
  undefined4 local_10;
  int in_ECX;

  local_10 = param_1;
  (**(code **)(**(int **)(in_ECX + 0x18) + 0x10))
            (*(int **)(in_ECX + 0x18),&DAT_0064e2d0,1,0,0,&local_10,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return 1;
}

Disassembly (0x54edf0..0x54ee22; phase-4 review):

0x54edf0: mov eax, dword ptr [esp + 4]
0x54edf4: push 4
0x54edf6: lea edx, [esp + 8]
0x54edfa: push edx
0x54edfb: push 0
0x54edfd: push 0
0x54edff: push 1
0x54ee01: mov dword ptr [esp + 0x18], eax
0x54ee05: mov eax, dword ptr [ecx + 0x18]
0x54ee08: mov ecx, dword ptr [eax]
0x54ee0a: push 0x64e2d0
0x54ee0f: push eax
0x54ee10: call dword ptr [ecx + 0x10]
0x54ee13: test eax, eax
0x54ee15: jl 0x54ee22
0x54ee17: mov eax, dword ptr [0x746114]
0x54ee1c: mov ecx, dword ptr [eax]
0x54ee1e: push eax
0x54ee1f: call dword ptr [ecx + 0x44]
0x54ee22: ret 4
#endif
