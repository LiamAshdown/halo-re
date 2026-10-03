// sound_eax1_effect_set_room_gain  (Ghidra: missed_54ee30; 0 callers in this module -- reached
// only through the EAX1 vtable's set_room_gain slot, 0x00671d4c+0x20)
// address 0x54ee30, size 53 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: confirmed by reading .rdata at 0x00671d4c (EAX1 vtable): +0x20 == 0x0054ee30.
//   types/sound.h sound_effect_object_vtable.set_room_gain comment ("0x20 ... float gain");
//   sound_effect_object.property_set (0x18, types/sound.h) matches by offset; IKsPropertySet::Set
//   is vtable slot 0x10; sound_eax_listener_property_guid (0x0064e2d0) is reused from
//   sound_eax1_effect_apply_listener.c (this module); property id 2 is EAX 1.0's VOLUME property
//   (DSPROPERTY_EAX_VOLUME). Unlike the EAX2/EAX3 set_room_gain implementations (0x54ff70,
//   0x5511c0, this batch), this one forwards the caller's value untouched -- the caller
//   (sound_channel_set_parameters.c, this module) already computes the millibel value before
//   calling through the driver.
// register convention: __thiscall (ECX -> this), stack -> gain.
// blam-cc: ECX -> this_object, stack -> gain
// FIXED (register inputs, objdump): ECX carries this_object (read at 0x54ee45, mov eax,[ecx+0x18]);
// the notes said "ECX -> this" but the parameter is named this_object, so it did not parse.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block. As with sound_eax1_effect_set_environment_index.c (0x54edf0, this batch), the
// CommitDeferredSettings call is conditional on Set() succeeding, which Ghidra's own decompile
// did not show.

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax_listener_property_guid[16]; // 0x0064e2d0, EAX 1.0 listener property set
extern void *directsound_listener; // 0x00746114, IDirectSound3DListener *

// blam-cc: ECX -> this_object, stack -> gain
// Sets the EAX 1.0 listener's VOLUME (id 2) property to `gain` unchanged, then commits deferred
// 3D listener settings if the Set succeeded.
void __thiscall sound_eax1_effect_set_room_gain(sound_effect_object *this_object, float gain)
{
    sound_property_set_fn set = (sound_property_set_fn)(*(void ***)this_object->property_set)[0x10 / 4];
    int32_t result = set(this_object->property_set, sound_eax_listener_property_guid, 2, 0, 0, &gain, 4);

    if (result >= 0) {
        ((directsound_listener_commit_proc)(*(void ***)directsound_listener)[0x44 / 4])(directsound_listener);
    }
}

#if 0
Original Ghidra decompilation (0x54ee30):

undefined1 missed_54ee30(undefined4 param_1)

{
  undefined4 local_10;
  int in_ECX;

  local_10 = param_1;
  (**(code **)(**(int **)(in_ECX + 0x18) + 0x10))
            (*(int **)(in_ECX + 0x18),&DAT_0064e2d0,2,0,0,&local_10,4);
  (**(code **)(*DAT_00746114 + 0x44))(DAT_00746114);
  return 1;
}

Disassembly (0x54ee30..0x54ee62; phase-4 review):

0x54ee30: mov eax, dword ptr [esp + 4]
0x54ee34: push 4
0x54ee36: lea edx, [esp + 8]
0x54ee3a: push edx
0x54ee3b: push 0
0x54ee3d: push 0
0x54ee3f: push 2
0x54ee41: mov dword ptr [esp + 0x18], eax
0x54ee45: mov eax, dword ptr [ecx + 0x18]
0x54ee48: mov ecx, dword ptr [eax]
0x54ee4a: push 0x64e2d0
0x54ee4f: push eax
0x54ee50: call dword ptr [ecx + 0x10]
0x54ee53: test eax, eax
0x54ee55: jl 0x54ee62
0x54ee57: mov eax, dword ptr [0x746114]
0x54ee5c: mov ecx, dword ptr [eax]
0x54ee5e: push eax
0x54ee5f: call dword ptr [ecx + 0x44]
0x54ee62: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
