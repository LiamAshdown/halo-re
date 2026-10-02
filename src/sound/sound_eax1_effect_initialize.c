// sound_eax1_effect_initialize  (Ghidra: FUN_0054ec60; earlier draft name
//   sound_eax1_effect_initialize)
// address 0x54ec60, size 238 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: slot 1 (initialize) of the EAX1 sound_effect_object_vtable at 0x00671d4c; called by
//   sound_effects_object_detect_mode (0x551270) and sound_effects_object_reinitialize
//   (0x5514d0) as initialize(this, channel, index), `ret 8`. It asks the channel's 3D buffer
//   for IKsPropertySet (0x0064e20c) and, in the EAX 1.0 listener property set (0x0064e2d0),
//   probes DSPROPERTY_EAX_VOLUME (2), _DECAYTIME (3) and _DAMPING (4), recording each that
//   supports both get and set (KSPROPERTY_SUPPORT_GET | _SET = 3) as bits 0x4 / 0x8 / 0x10.
// Phase-4 review (disassembly appended below): confirmed; the second stack argument is unused.
// register convention: ECX -> this_object, stack -> (channel, unused).

#include "tags.h"
#include "memory.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern const uint8_t sound_eax_property_set_guid[16];      // 0x0064e20c, IID_IKsPropertySet
extern const uint8_t sound_eax_listener_property_guid[16]; // 0x0064e2d0, EAX 1.0 listener property set

// blam-cc: ECX -> this_object, stack -> (channel, unused)
// Returns (and stores as listener_supported) whether any EAX 1.0 listener property is usable.
int32_t sound_eax1_effect_initialize(sound_effect_object *this_object, directsound_channel *channel, int32_t unused)
{
    uint32_t type_support;
    void **vtable;

    (void)unused;
    this_object->property_set = 0;
    this_object->supported_properties = 0;
    this_object->listener_supported = 0;
    this_object->channel_supported = 0;

    if (channel == 0 || channel->buffer_3d == 0) {
        return 0;
    }

    vtable = *(void ***)channel->buffer_3d;
    if (((int32_t (__stdcall *)(void *, const uint8_t *, void **))vtable[0])(channel->buffer_3d, sound_eax_property_set_guid,
            &this_object->property_set) < 0) { // QueryInterface
        this_object->property_set = 0;
    } else {
        void *property_set = this_object->property_set;
        sound_query_support_fn query_support =
            (sound_query_support_fn)(*(void ***)property_set)[0x14 / 4];

        type_support = 0;
        if (query_support(property_set, sound_eax_listener_property_guid, 2, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x04;
        }
        if (query_support(property_set, sound_eax_listener_property_guid, 3, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x08;
        }
        if (query_support(property_set, sound_eax_listener_property_guid, 4, &type_support) >= 0 &&
            (type_support & 3) == 3) {
            this_object->supported_properties |= 0x10;
        }
    }

    this_object->listener_supported = this_object->supported_properties != 0;
    return this_object->listener_supported;
}

#if 0
Original Ghidra decompilation (0x54ec60):

uint FUN_0054ec60(int param_1)

{
  undefined4 *puVar1;
  int iVar2;
  uint uVar3;
  int in_ECX;
  byte bVar4;
  int *piStack_28;
  undefined *puStack_24;
  undefined4 uStack_20;
  undefined1 *puStack_1c;
  undefined4 *puStack_18;
  undefined *puStack_14;
  undefined4 *puStack_10;
  int *piVar5;

  puVar1 = (undefined4 *)(in_ECX + 0x18);
  *puVar1 = 0;
  *(undefined4 *)(in_ECX + 8) = 0;
  *(undefined4 *)(in_ECX + 0x10) = 0;
  *(undefined4 *)(in_ECX + 0x14) = 0;
  if ((param_1 == 0) ||
     (puStack_18 = *(undefined4 **)(param_1 + 0x674), puStack_18 == (undefined4 *)0x0)) {
    return 0;
  }
  puStack_14 = &DAT_0064e20c;
  puStack_1c = (undefined1 *)0x54ec9c;
  puStack_10 = puVar1;
  iVar2 = (**(code **)*puStack_18)();
  if (iVar2 < 0) {
    *puVar1 = 0;
  }
  else {
    piStack_28 = (int *)*puVar1;
    puStack_1c = &stack0xfffffff8;
    uStack_20 = 2;
    puStack_24 = &DAT_0064e2d0;
    iVar2 = (**(code **)(*piStack_28 + 0x14))();
    if ((-1 < iVar2) && (((byte)puStack_18 & 3) == 3)) {
      *(uint *)(in_ECX + 8) = *(uint *)(in_ECX + 8) | 4;
    }
    piVar5 = (int *)*puVar1;
    iVar2 = (**(code **)(*piVar5 + 0x14))(piVar5,&DAT_0064e2d0,3,&puStack_18);
    bVar4 = (byte)piVar5;
    if ((-1 < iVar2) && (((byte)piStack_28 & 3) == 3)) {
      *(uint *)(in_ECX + 8) = *(uint *)(in_ECX + 8) | 8;
    }
    iVar2 = (**(code **)(*(int *)*puVar1 + 0x14))((int *)*puVar1,&DAT_0064e2d0,4,&piStack_28);
    if ((-1 < iVar2) && ((bVar4 & 3) == 3)) {
      *(uint *)(in_ECX + 8) = *(uint *)(in_ECX + 8) | 0x10;
      uVar3 = (uint)(*(int *)(in_ECX + 8) != 0);
      *(uint *)(in_ECX + 0x10) = uVar3;
      return uVar3;
    }
  }
  uVar3 = (uint)(*(int *)(in_ECX + 8) != 0);
  *(uint *)(in_ECX + 0x10) = uVar3;
  return uVar3;
}

Disassembly (0x54ec60..0x54ed4e, capstone; phase-4 review):

0x54ec60: mov eax, dword ptr [esp + 4]
0x54ec64: push ebx
0x54ec65: push esi
0x54ec66: xor ebx, ebx
0x54ec68: cmp eax, ebx
0x54ec6a: push edi
0x54ec6b: mov edi, ecx
0x54ec6d: lea esi, [edi + 0x18]
0x54ec70: mov dword ptr [esi], ebx
0x54ec72: mov dword ptr [edi + 8], ebx
0x54ec75: mov dword ptr [edi + 0x10], ebx
0x54ec78: mov dword ptr [edi + 0x14], ebx
0x54ec7b: je 0x54ed46
0x54ec81: mov ecx, dword ptr [eax + 0x674]
0x54ec87: cmp ecx, ebx
0x54ec89: je 0x54ed46
0x54ec8f: push esi
0x54ec90: mov eax, ecx
0x54ec92: mov ecx, dword ptr [eax]
0x54ec94: push 0x64e20c
0x54ec99: push eax
0x54ec9a: call dword ptr [ecx]
0x54ec9c: test eax, eax
0x54ec9e: jl 0x54ed31
0x54eca4: mov eax, dword ptr [esi]
0x54eca6: lea ecx, [esp + 0x10]
0x54ecaa: push ecx
0x54ecab: push 2
0x54ecad: push 0x64e2d0
0x54ecb2: mov dword ptr [esp + 0x1c], ebx
0x54ecb6: mov edx, dword ptr [eax]
0x54ecb8: push eax
0x54ecb9: call dword ptr [edx + 0x14]
0x54ecbc: test eax, eax
0x54ecbe: jl 0x54ecd0
0x54ecc0: mov edx, dword ptr [esp + 0x10]
0x54ecc4: and edx, 3
0x54ecc7: cmp dl, 3
0x54ecca: jne 0x54ecd0
0x54eccc: or dword ptr [edi + 8], 4
0x54ecd0: mov eax, dword ptr [esi]
0x54ecd2: mov ecx, dword ptr [eax]
0x54ecd4: lea edx, [esp + 0x10]
0x54ecd8: push edx
0x54ecd9: push 3
0x54ecdb: push 0x64e2d0
0x54ece0: push eax
0x54ece1: call dword ptr [ecx + 0x14]
0x54ece4: test eax, eax
0x54ece6: jl 0x54ecf7
0x54ece8: mov eax, dword ptr [esp + 0x10]
0x54ecec: and eax, 3
0x54ecef: cmp al, 3
0x54ecf1: jne 0x54ecf7
0x54ecf3: or dword ptr [edi + 8], 8
0x54ecf7: mov esi, dword ptr [esi]
0x54ecf9: mov ecx, dword ptr [esi]
0x54ecfb: lea edx, [esp + 0x10]
0x54ecff: push edx
0x54ed00: push 4
0x54ed02: push 0x64e2d0
0x54ed07: push esi
0x54ed08: call dword ptr [ecx + 0x14]
0x54ed0b: test eax, eax
0x54ed0d: jl 0x54ed33
0x54ed0f: mov eax, dword ptr [esp + 0x10]
0x54ed13: and eax, 3
0x54ed16: cmp al, 3
0x54ed18: jne 0x54ed33
0x54ed1a: or dword ptr [edi + 8], 0x10
0x54ed1e: mov ecx, dword ptr [edi + 8]
0x54ed21: xor eax, eax
0x54ed23: cmp ecx, ebx
0x54ed25: setne al
0x54ed28: mov dword ptr [edi + 0x10], eax
0x54ed2b: pop edi
0x54ed2c: pop esi
0x54ed2d: pop ebx
0x54ed2e: ret 8
0x54ed31: mov dword ptr [esi], ebx
0x54ed33: mov ecx, dword ptr [edi + 8]
0x54ed36: xor eax, eax
0x54ed38: cmp ecx, ebx
0x54ed3a: setne al
0x54ed3d: mov dword ptr [edi + 0x10], eax
0x54ed40: pop edi
0x54ed41: pop esi
0x54ed42: pop ebx
0x54ed43: ret 8
0x54ed46: pop edi
0x54ed47: pop esi
0x54ed48: xor eax, eax
0x54ed4a: pop ebx
0x54ed4b: ret 8
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
