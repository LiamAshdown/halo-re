// sound_eax1_effect_apply_listener  (Ghidra: FUN_0054ed50; earlier draft name
//   sound_eax1_effect_apply_listener)
// address 0x54ed50, size 148 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: slot 7 (apply_listener) of the EAX1 sound_effect_object_vtable at 0x00671d4c
//   (called by sound_listener_update 0x547070 with the SoundEnvironment); `data` is that
//   SoundEnvironment (types/tags.h): +0x14 decay_time, +0x18 decay_hf_ratio, +0x24
//   reverb_intensity. They are written to the EAX 1.0 listener properties DECAYTIME (3),
//   DAMPING (4, decay_hf_ratio * 0.4761905 * 2, the float at 0x00672af0) and VOLUME (2) through
//   IKsPropertySet::Set (vtable +0x10), each only when initialize found it supported.
// Phase-4 review: the draft read this as a per-channel obstruction update with unknown field
//   offsets; the vtable slot and the SoundEnvironment layout settle it.
// register convention: ECX -> this_object, stack -> environment (ret 4).

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax_listener_property_guid[16]; // 0x0064e2d0, EAX 1.0 listener property set
extern uint8_t directsound_deferred_dirty; // 0x00746132

// blam-cc: ECX -> this_object, stack -> environment
void sound_eax1_effect_apply_listener(sound_effect_object *this_object, SoundEnvironment *environment)
{
    float value;

    if ((this_object->supported_properties & 0x08) != 0) {
        value = environment->decay_time;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[0x10 / 4])(this_object->property_set,
            sound_eax_listener_property_guid, 3, 0, 0, &value, 4);
    }
    if ((this_object->supported_properties & 0x10) != 0) {
        value = environment->decay_hf_ratio * 0.4761905f;
        value = value + value;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[0x10 / 4])(this_object->property_set,
            sound_eax_listener_property_guid, 4, 0, 0, &value, 4);
    }
    if ((this_object->supported_properties & 0x04) != 0) {
        value = environment->reverb_intensity;
        ((sound_property_set_fn)(*(void ***)this_object->property_set)[0x10 / 4])(this_object->property_set,
            sound_eax_listener_property_guid, 2, 0, 0, &value, 4);
    }
    directsound_deferred_dirty = 1;
}

#if 0
Original Ghidra decompilation (0x54ed50):

void FUN_0054ed50(float param_1)

{
  int iVar1;
  int in_ECX;

  iVar1 = (int)param_1;
  if ((*(byte *)(in_ECX + 8) & 8) != 0) {
    param_1 = *(float *)((int)param_1 + 0x14);
    (**(code **)(**(int **)(in_ECX + 0x18) + 0x10))
              (*(int **)(in_ECX + 0x18),&DAT_0064e2d0,3,0,0,&param_1,4);
  }
  if ((*(byte *)(in_ECX + 8) & 0x10) != 0) {
    param_1 = *(float *)(iVar1 + 0x18) * 0.4761905;
    param_1 = param_1 + param_1;
    (**(code **)(**(int **)(in_ECX + 0x18) + 0x10))
              (*(int **)(in_ECX + 0x18),&DAT_0064e2d0,4,0,0,&param_1,4);
  }
  if ((*(byte *)(in_ECX + 8) & 4) != 0) {
    param_1 = *(float *)(iVar1 + 0x24);
    (**(code **)(**(int **)(in_ECX + 0x18) + 0x10))
              (*(int **)(in_ECX + 0x18),&DAT_0064e2d0,2,0,0,&param_1,4);
  }
  DAT_00746132 = 1;
  return;
}

Disassembly (0x54ed50..0x54ede4, capstone; phase-4 review):

0x54ed50: push esi
0x54ed51: mov esi, ecx
0x54ed53: test byte ptr [esi + 8], 8
0x54ed57: push edi
0x54ed58: mov edi, dword ptr [esp + 0xc]
0x54ed5c: je 0x54ed80
0x54ed5e: mov eax, dword ptr [edi + 0x14]
0x54ed61: push 4
0x54ed63: lea edx, [esp + 0x10]
0x54ed67: push edx
0x54ed68: push 0
0x54ed6a: push 0
0x54ed6c: push 3
0x54ed6e: mov dword ptr [esp + 0x20], eax
0x54ed72: mov eax, dword ptr [esi + 0x18]
0x54ed75: mov ecx, dword ptr [eax]
0x54ed77: push 0x64e2d0
0x54ed7c: push eax
0x54ed7d: call dword ptr [ecx + 0x10]
0x54ed80: test byte ptr [esi + 8], 0x10
0x54ed84: je 0x54edb0
0x54ed86: fld dword ptr [edi + 0x18]
0x54ed89: mov eax, dword ptr [esi + 0x18]
0x54ed8c: fmul dword ptr [0x672af0]
0x54ed92: push 4
0x54ed94: lea edx, [esp + 0x10]
0x54ed98: push edx
0x54ed99: push 0
0x54ed9b: push 0
0x54ed9d: fadd st(0), st(0)
0x54ed9f: push 4
0x54eda1: push 0x64e2d0
0x54eda6: fstp dword ptr [esp + 0x24]
0x54edaa: mov ecx, dword ptr [eax]
0x54edac: push eax
0x54edad: call dword ptr [ecx + 0x10]
0x54edb0: test byte ptr [esi + 8], 4
0x54edb4: je 0x54edd8
0x54edb6: mov eax, dword ptr [edi + 0x24]
0x54edb9: mov esi, dword ptr [esi + 0x18]
0x54edbc: push 4
0x54edbe: lea edx, [esp + 0x10]
0x54edc2: push edx
0x54edc3: push 0
0x54edc5: push 0
0x54edc7: push 2
0x54edc9: push 0x64e2d0
0x54edce: mov dword ptr [esp + 0x24], eax
0x54edd2: mov ecx, dword ptr [esi]
0x54edd4: push esi
0x54edd5: call dword ptr [ecx + 0x10]
0x54edd8: pop edi
0x54edd9: mov byte ptr [0x746132], 1
0x54ede0: pop esi
0x54ede1: ret 4
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
