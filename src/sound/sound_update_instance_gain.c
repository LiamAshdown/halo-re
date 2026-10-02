// sound_update_instance_gain  (Ghidra: FUN_0054c750, still unnamed there)
// address 0x54c750, size 426 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Computes and applies the current gain for a
// non-looping (one-shot) playing sound instance, starting or updating its driver voice."; the
// sound_channel_parameters block and driver channel_continue call match
// sound_update_looping_gain.c's (0x54deb0) established layout exactly. When the channel was
// already assigned (the second branch), only `gain` (local_14) is ever written into the stack
// parameters block before it is passed with update=1; this rewrite reads that as "update=1 means
// the proc only consults gain", consistent with sound_channel_parameters_proc's own `update`
// flag, rather than forwarding genuinely uninitialized stack data.
// register convention: stack -> (channel_index, external_gain_multiplier), both recognized.
// Phase-4 review (disassembly appended below): the 4th proc argument is EAX, which still holds
// the sound class loaded for sound_compute_class_gain (that function leaves EAX alone), so it is
// definition->sound_class; the running gain factor is Sound.random_gain_modifier (+0x28), not
// outer_cone_gain as the draft had it.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;         // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances;    // 0x0087bc14
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern sound_channel_parameters_proc sound_channel_parameters_proc_ptr; // 0x006e36cc
extern sound_driver *current_sound_driver; // 0x00725208, header calls this "sound_driver"

extern float sound_compute_class_gain(SoundClass_t sound_class); // this module, 0x54b100
extern void sound_channel_set_next_permutation(int16_t channel_index, SoundPermutation *permutation,
    int16_t unknown, int16_t sound_class, uint8_t streaming); // this module, 0x54cd30

// blam-cc: stack -> (channel_index, external_gain_multiplier)
// Computes and applies the current gain (and, the first time this channel is assigned, the full
// parameter set and driver voice start) for the one-shot sound playing on `channel_index`.
void sound_update_instance_gain(int16_t channel_index, float external_gain_multiplier)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    float zero_gain, class_gain, gain_factor;

    sound_handle = sound_channels[channel_index].sound_index;
    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;

    zero_gain = definition->zero_gain_modifier;
    class_gain = sound_compute_class_gain(definition->sound_class);
    gain_factor = (definition->one_gain_modifier - zero_gain) * instance->location.scale + zero_gain;
    gain_factor = gain_factor * class_gain * instance->location.gain * external_gain_multiplier;

    if (instance->channel_index == -1) {
        SoundPitchRange *pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        SoundPermutation *permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        sound_channel_parameters params;

        params.gain = permutation->gain * definition->random_gain_modifier * gain_factor;
        params.pitch = instance->pitch * pitch_range->playback_rate;
        params.minimum_distance = definition->minimum_distance;
        if (params.minimum_distance == 0.0f) {
            params.minimum_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
        }
        params.inner_cone_angle = definition->inner_cone_angle;
        params.outer_cone_angle = definition->outer_cone_angle;
        params.outer_cone_gain = definition->outer_cone_gain;
        params.eax_value = sound_class_definitions[definition->sound_class].eax_value;
        params.maximum_distance = 3.4028235e+38f;

        sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
        sound_channel_set_next_permutation(channel_index, permutation, instance->first_person,
            definition->sound_class, 0);
        instance->channel_index = channel_index;
        return;
    }

    {
        sound_channel_parameters params;
        SoundPermutation *current = sound_channels[channel_index].current_permutation;

        // only the gain is updated (update flag 1); the rest of params is never read
        if (current == 0) {
            params.gain = definition->random_gain_modifier * gain_factor;
        } else {
            params.gain = current->gain * definition->random_gain_modifier * gain_factor;
        }

        sound_channel_parameters_proc_ptr(channel_index, &params, 1, definition->sound_class);
        current_sound_driver->channel_continue(channel_index, 0, definition->sound_class);
    }
}

#if 0
Original Ghidra decompilation (0x54c750):

void FUN_0054c750(undefined4 param_1,float param_2)

{
  int *piVar1;
  int iVar2;
  float fVar3;
  undefined4 uVar4;
  int iVar5;
  short sVar6;
  int iVar7;
  float10 extraout_ST0;
  float10 fVar8;
  float10 fVar9;
  float local_20 [3];
  float local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  sVar6 = (short)param_1;
  iVar7 = ((&DAT_00724a60)[sVar6 * 6] & 0xffff) * 0xb0 + *(int *)(DAT_007252c0 + 0x34);
  iVar5 = (*(uint *)(iVar7 + 8) & 0xffff) * 0x20;
  iVar2 = *(int *)(iVar5 + 0x14 + DAT_0087bc14);
  piVar1 = (int *)(iVar5 + 0x14 + DAT_0087bc14);
  fVar3 = *(float *)(iVar2 + 0x40);
  uVar4 = sound_compute_class_gain();
  fVar8 = (((float10)*(float *)(iVar2 + 0x58) - (float10)fVar3) * (float10)*(float *)(iVar7 + 0x18)
          + (float10)fVar3) * extraout_ST0 * (float10)*(float *)(iVar7 + 0x1c) * (float10)param_2;
  if (*(short *)(iVar7 + 0x8c) == -1) {
    iVar5 = *(int *)(iVar2 + 0x9c) + *(short *)(iVar7 + 0x8e) * 0x48;
    local_14 = (float)((float10)*(float *)(*(short *)(iVar7 + 0x90) * 0x7c + *(int *)(iVar5 + 0x40)
                                          + 0x24) * (float10)*(float *)(iVar2 + 0x28) * fVar8);
    local_20[2] = *(float *)(iVar7 + 0x88) * *(float *)(iVar5 + 0x30);
    iVar5 = *piVar1;
    local_20[0] = *(float *)(iVar5 + 8);
    if (local_20[0] == 0.0) {
      local_20[0] = *(float *)(&DAT_0069eaf8 + *(short *)(iVar5 + 4) * 0x2c);
    }
    local_c = *(undefined4 *)(iVar2 + 0x20);
    local_10 = *(undefined4 *)(iVar2 + 0x1c);
    local_8 = *(undefined4 *)(iVar2 + 0x24);
    local_4 = *(undefined4 *)(&DAT_0069eaf0 + *(short *)(iVar2 + 4) * 0x2c);
    local_20[1] = 3.4028235e+38;
    (*DAT_006e36cc)(param_1,local_20,0,*(short *)(iVar2 + 4));
    FUN_0054cd30(*(undefined1 *)(iVar7 + 0xac),*(undefined2 *)(iVar2 + 4),0);
    *(short *)(iVar7 + 0x8c) = sVar6;
    return;
  }
  if ((&DAT_00724a70)[sVar6 * 6] == 0) {
    fVar9 = (float10)*(float *)(iVar2 + 0x28);
  }
  else {
    fVar9 = (float10)*(float *)((&DAT_00724a70)[sVar6 * 6] + 0x24) *
            (float10)*(float *)(iVar2 + 0x28);
  }
  local_14 = (float)(fVar9 * fVar8);
  (*DAT_006e36cc)(param_1,local_20,1,uVar4);
  (**(code **)(DAT_00725208 + 0x1c))(param_1,0,*(undefined2 *)(iVar2 + 4));
  return;
}

Disassembly (0x54c750..0x54c8fa, capstone; phase-4 review):

0x54c750: sub esp, 0x24
0x54c753: push ebx
0x54c754: push ebp
0x54c755: mov ebp, dword ptr [esp + 0x30]
0x54c759: mov edx, dword ptr [0x87bc14]
0x54c75f: push esi
0x54c760: movsx eax, bp
0x54c763: push edi
0x54c764: lea edi, [eax + eax*2]
0x54c767: mov esi, dword ptr [edi*8 + 0x724a60]
0x54c76e: mov eax, dword ptr [0x7252c0]
0x54c773: mov ecx, dword ptr [eax + 0x34]
0x54c776: lea edi, [edi*8 + 0x724a60]
0x54c77d: and esi, 0xffff
0x54c783: imul esi, esi, 0xb0
0x54c789: add esi, ecx
0x54c78b: mov ecx, dword ptr [esi + 8]
0x54c78e: and ecx, 0xffff
0x54c794: shl ecx, 5
0x54c797: mov ebx, dword ptr [ecx + edx + 0x14]
0x54c79b: lea eax, [ecx + edx + 0x14]
0x54c79f: mov dword ptr [esp + 0x10], eax
0x54c7a3: mov eax, dword ptr [ebx + 0x40]
0x54c7a6: mov dword ptr [esp + 0x38], eax
0x54c7aa: xor eax, eax
0x54c7ac: mov ax, word ptr [ebx + 4]
0x54c7b0: call 0x54b100
0x54c7b5: cmp word ptr [esi + 0x8c], -1
0x54c7bd: fld dword ptr [ebx + 0x58]
0x54c7c0: fsub dword ptr [esp + 0x38]
0x54c7c4: fmul dword ptr [esi + 0x18]
0x54c7c7: fadd dword ptr [esp + 0x38]
0x54c7cb: fmulp st(1)
0x54c7cd: fmul dword ptr [esi + 0x1c]
0x54c7d0: fmul dword ptr [esp + 0x3c]
0x54c7d4: jne 0x54c8af
0x54c7da: movsx edi, word ptr [esi + 0x90]
0x54c7e1: movsx eax, word ptr [esi + 0x8e]
0x54c7e8: imul edi, edi, 0x7c
0x54c7eb: mov edx, dword ptr [ebx + 0x9c]
0x54c7f1: lea ecx, [eax + eax*8]
0x54c7f4: lea eax, [edx + ecx*8]
0x54c7f7: add edi, dword ptr [eax + 0x40]
0x54c7fa: fld dword ptr [edi + 0x24]
0x54c7fd: fmul dword ptr [ebx + 0x28]
0x54c800: fmul st(1)
0x54c802: fstp dword ptr [esp + 0x20]
0x54c806: fstp st(0)
0x54c808: fld dword ptr [esi + 0x88]
0x54c80e: fmul dword ptr [eax + 0x30]
0x54c811: mov eax, dword ptr [esp + 0x10]
0x54c815: fstp dword ptr [esp + 0x1c]
0x54c819: mov ecx, dword ptr [eax]
0x54c81b: fld dword ptr [ecx + 8]
0x54c81e: fld dword ptr [0x672ac0]
0x54c824: fld st(1)
0x54c826: fucompp 
0x54c828: fnstsw ax
0x54c82a: test ah, 0x44
0x54c82d: jp 0x54c83e
0x54c82f: movsx ecx, word ptr [ecx + 4]
0x54c833: fstp st(0)
0x54c835: imul ecx, ecx, 0x2c
0x54c838: fld dword ptr [ecx + 0x69eaf8]
0x54c83e: mov eax, dword ptr [ebx + 0x20]
0x54c841: fstp dword ptr [esp + 0x14]
0x54c845: mov edx, dword ptr [ebx + 0x1c]
0x54c848: mov ecx, dword ptr [ebx + 0x24]
0x54c84b: mov dword ptr [esp + 0x28], eax
0x54c84f: xor eax, eax
0x54c851: mov ax, word ptr [ebx + 4]
0x54c855: mov dword ptr [esp + 0x24], edx
0x54c859: movsx edx, ax
0x54c85c: imul edx, edx, 0x2c
0x54c85f: push eax
0x54c860: mov dword ptr [esp + 0x30], ecx
0x54c864: mov ecx, dword ptr [edx + 0x69eaf0]
0x54c86a: push 0
0x54c86c: lea edx, [esp + 0x1c]
0x54c870: push edx
0x54c871: push ebp
0x54c872: mov dword ptr [esp + 0x28], 0x7f7fffff
0x54c87a: mov dword ptr [esp + 0x40], ecx
0x54c87e: call dword ptr [0x6e36cc]
0x54c884: xor eax, eax
0x54c886: mov ax, word ptr [ebx + 4]
0x54c88a: xor ecx, ecx
0x54c88c: mov cl, byte ptr [esi + 0xac]
0x54c892: push 0
0x54c894: push eax
0x54c895: push ecx
0x54c896: mov ecx, ebp
0x54c898: call 0x54cd30
0x54c89d: add esp, 0x1c
0x54c8a0: pop edi
0x54c8a1: mov word ptr [esi + 0x8c], bp
0x54c8a8: pop esi
0x54c8a9: pop ebp
0x54c8aa: pop ebx
0x54c8ab: add esp, 0x24
0x54c8ae: ret 
0x54c8af: mov edi, dword ptr [edi + 0x10]
0x54c8b2: test edi, edi
0x54c8b4: je 0x54c8c6
0x54c8b6: fld dword ptr [edi + 0x24]
0x54c8b9: fmul dword ptr [ebx + 0x28]
0x54c8bc: fmul st(1)
0x54c8be: fstp dword ptr [esp + 0x20]
0x54c8c2: fstp st(0)
0x54c8c4: jmp 0x54c8cd
0x54c8c6: fmul dword ptr [ebx + 0x28]
0x54c8c9: fstp dword ptr [esp + 0x20]
0x54c8cd: push eax
0x54c8ce: push 1
0x54c8d0: lea edx, [esp + 0x1c]
0x54c8d4: push edx
0x54c8d5: push ebp
0x54c8d6: call dword ptr [0x6e36cc]
0x54c8dc: mov ecx, dword ptr [0x725208]
0x54c8e2: xor eax, eax
0x54c8e4: mov ax, word ptr [ebx + 4]
0x54c8e8: push eax
0x54c8e9: push 0
0x54c8eb: push ebp
0x54c8ec: call dword ptr [ecx + 0x1c]
0x54c8ef: add esp, 0x1c
0x54c8f2: pop edi
0x54c8f3: pop esi
0x54c8f4: pop ebp
0x54c8f5: pop ebx
0x54c8f6: add esp, 0x24
0x54c8f9: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
