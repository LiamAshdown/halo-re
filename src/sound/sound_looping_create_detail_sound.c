// sound_looping_create_detail_sound  (Ghidra: FUN_0054d9f0, still unnamed there)
// address 0x54d9f0, size 541 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Creates a new one-shot 'detail sound' playing
// instance associated with a looping sound's cluster/permutation."; in practice it creates the
// sound for one looping track part (start / loop / end, play_state 1 / 2 / 4) for
// sound_looping_set_state (0x549fa0) and the loop crossfade in sound_update_looping_gain
// (0x54deb0). The pitch-range pick uses the zero_/one_pitch_modifier lerp at the owner's
// location.scale times a random_range_real pitch, as sound_compute_random_pitch does.
// register convention: stack -> (owner, definition_index, track_index, play_state).
// Phase-4 review (disassembly appended below): location_proc is 0x54dc10,
// sound_looping_track_location_proc (it copies the owner's whole location; it is not a thunk to
// 0x54dc70 as types/sound.h first assumed); sound_location_check_audibility gets the owner's
// location in EAX; sound_permutation_pick_for_pitch gets AX = -1 (the draft passed 0);
// sound_cache_touch gets (1, 0, BL 0, EDI = the chosen permutation) (the draft passed NULL).

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4
extern tag_instance *tag_instances;    // 0x0087bc14
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern data_array *sound_data;         // 0x007252c0, "sounds" 0x200 x 0xb0
extern int32_t sound_time;             // 0x0072520c

extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, cache module
extern real random_range_real(real minimum, real maximum); // 0x444af0, math module
extern datum_index datum_new(data_array *array); // 0x4d0480, memory module


// Creates a new one-shot detail sound for `definition_index`, owned by looping_sound `owner`,
// starting at `track_index`/`play_state`. Returns k_datum_index_none if the definition has no
// loaded, unmuted permutations, or is not currently audible from the owner's location.
datum_index sound_looping_create_detail_sound(datum_index owner, datum_index definition_index,
    int16_t track_index, int16_t play_state)
{
    looping_sound *state;
    Sound *definition;
    float owner_scale;
    float max_distance;
    int16_t listener_index;
    datum_index handle;
    sound *instance;
    float pitch;
    float pitch_modifier;

    state = (looping_sound *)((uint8_t *)looping_sound_data->data + (owner & 0xffff) * sizeof(looping_sound));
    owner_scale = state->location.scale;
    definition = (Sound *)tag_instances[definition_index & 0xffff].data;

    if (definition->pitch_ranges.count == 0) {
        return 0xffffffff;
    }
    if (((SoundPitchRange *)definition->pitch_ranges.pointer)->permutations.count == 0 ||
        sound_class_definitions[definition->sound_class].muted != 0) {
        return 0xffffffff;
    }

    max_distance = definition->maximum_distance;
    if (max_distance == 0.0f) {
        max_distance = sound_class_definitions[definition->sound_class].default_maximum_distance;
    }
    listener_index = sound_location_check_audibility(&state->location, max_distance);
    if (listener_index == -1) {
        return 0xffffffff;
    }

    handle = datum_new(sound_data);
    if (handle == 0xffffffff) {
        return handle;
    }

    instance = (sound *)((uint8_t *)sound_data->data + (handle & 0xffff) * sizeof(sound));
    instance->definition_index = definition_index;
    instance->channel_index = -1;
    instance->listener_index = listener_index;
    instance->flags = 0;
    pitch = random_range_real(definition->random_pitch_bounds[0], definition->random_pitch_bounds[1]);
    instance->pitch = pitch;
    instance->owner_index = owner;
    instance->location = state->location;
    instance->track_index = track_index;
    instance->start_time = sound_time;
    instance->play_state = play_state;
    instance->location_proc = sound_looping_track_location_proc;
    instance->fade_end_time = 0;
    instance->fade_start_time = 0;
    instance->pending_definition_index = 0xffffffff;

    pitch_modifier = (definition->one_pitch_modifier - definition->zero_pitch_modifier) * owner_scale +
        definition->zero_pitch_modifier;
    instance->pitch_range_index = sound_permutation_pick_for_pitch(-1, definition, pitch_modifier * pitch);
    instance->permutation_index = sound_permutation_pick_random(instance->pitch_range_index, -1, definition);

    {
        SoundPitchRange *range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        sound_cache_touch(1, 0, 0, (SoundPermutation *)range->permutations.pointer + instance->permutation_index);
    }
    state->active_sound_count += 1;

    return handle;
}

#if 0
Original Ghidra decompilation (0x54d9f0):

uint FUN_0054d9f0(uint param_1,uint param_2,undefined2 param_3,undefined2 param_4)

{
  int *piVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  short sVar5;
  undefined2 uVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  undefined4 *puVar12;
  undefined8 uVar13;
  float fVar14;
  float local_10;

  iVar10 = (param_1 & 0xffff) * 0xe4 + *(int *)(DAT_00724a50 + 0x34);
  fVar2 = *(float *)(iVar10 + 0x10);
  piVar1 = (int *)((param_2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = *piVar1;
  if (*(int *)(iVar3 + 0x98) == 0) {
    return 0xffffffff;
  }
  if ((*(int *)(*(int *)(iVar3 + 0x9c) + 0x3c) != 0) &&
     ((&DAT_0069eb08)[*(short *)(iVar3 + 4) * 0x2c] == '\0')) {
    iVar3 = *piVar1;
    local_10 = *(float *)(iVar3 + 0xc);
    if (local_10 == 0.0) {
      local_10 = *(float *)(&DAT_0069eafc + *(short *)(iVar3 + 4) * 0x2c);
    }
    sVar5 = FUN_0054bb20(local_10);
    if (sVar5 != -1) {
      uVar13 = datum_new();
      uVar7 = (uint)uVar13;
      if (uVar7 != 0xffffffff) {
        iVar9 = (uVar7 & 0xffff) * 0xb0 + *(int *)((int)((ulonglong)uVar13 >> 0x20) + 0x34);
        *(uint *)(iVar9 + 8) = param_2;
        *(undefined2 *)(iVar9 + 0x8c) = 0xffff;
        *(short *)(iVar9 + 6) = sVar5;
        *(undefined2 *)(iVar9 + 4) = 0;
        fVar14 = random_range_real(*(float *)(iVar3 + 0x14),*(float *)(iVar3 + 0x18));
        *(float *)(iVar9 + 0x88) = fVar14;
        *(uint *)(iVar9 + 0xc) = param_1;
        puVar11 = (undefined4 *)(iVar10 + 0xc);
        puVar12 = (undefined4 *)(iVar9 + 0x14);
        for (iVar8 = 0x10; iVar8 != 0; iVar8 = iVar8 + -1) {
          *puVar12 = *puVar11;
          puVar11 = puVar11 + 1;
          puVar12 = puVar12 + 1;
        }
        *(undefined2 *)(iVar9 + 2) = param_4;
        uVar4 = DAT_0072520c;
        *(undefined2 *)(iVar9 + 0x94) = param_3;
        *(undefined4 *)(iVar9 + 0x84) = uVar4;
        *(undefined1 **)(iVar9 + 0x10) = &LAB_0054dc10;
        *(undefined4 *)(iVar9 + 0xa8) = 0;
        *(undefined4 *)(iVar9 + 0xa4) = 0;
        *(undefined4 *)(iVar9 + 0x98) = 0xffffffff;
        uVar6 = sound_permutation_pick_for_pitch
                          (((*(float *)(iVar3 + 0x5c) - *(float *)(iVar3 + 0x44)) * fVar2 +
                           *(float *)(iVar3 + 0x44)) * fVar14);
        *(undefined2 *)(iVar9 + 0x8e) = uVar6;
        uVar6 = sound_permutation_pick_random(iVar3);
        *(undefined2 *)(iVar9 + 0x90) = uVar6;
        sound_cache_touch(1,0);
        *(short *)(iVar10 + 0x50) = *(short *)(iVar10 + 0x50) + 1;
      }
      return uVar7;
    }
    return 0xffffffff;
  }
  return 0xffffffff;
}

Disassembly (0x54d9f0..0x54dc0d, capstone; phase-4 review):

0x54d9f0: sub esp, 0x10
0x54d9f3: mov eax, dword ptr [0x724a50]
0x54d9f8: mov ecx, dword ptr [eax + 0x34]
0x54d9fb: mov edx, dword ptr [esp + 0x18]
0x54d9ff: mov eax, dword ptr [0x87bc14]
0x54da04: push ebx
0x54da05: push ebp
0x54da06: mov ebp, dword ptr [esp + 0x1c]
0x54da0a: and ebp, 0xffff
0x54da10: imul ebp, ebp, 0xe4
0x54da16: add ebp, ecx
0x54da18: mov ecx, dword ptr [ebp + 0x10]
0x54da1b: and edx, 0xffff
0x54da21: shl edx, 5
0x54da24: mov dword ptr [esp + 0x10], ecx
0x54da28: lea ecx, [edx + eax + 0x14]
0x54da2c: mov eax, dword ptr [ecx]
0x54da2e: mov edx, dword ptr [eax + 0x98]
0x54da34: or ebx, 0xffffffff
0x54da37: test edx, edx
0x54da39: je 0x54dc05
0x54da3f: mov edx, dword ptr [eax + 0x9c]
0x54da45: push esi
0x54da46: mov esi, dword ptr [edx + 0x3c]
0x54da49: test esi, esi
0x54da4b: je 0x54dbfc
0x54da51: movsx eax, word ptr [eax + 4]
0x54da55: imul eax, eax, 0x2c
0x54da58: mov dl, byte ptr [eax + 0x69eb08]
0x54da5e: test dl, dl
0x54da60: jne 0x54dbfc
0x54da66: fld dword ptr [0x672ac0]
0x54da6c: push edi
0x54da6d: mov edi, dword ptr [ecx]
0x54da6f: mov ecx, dword ptr [edi + 0xc]
0x54da72: mov dword ptr [esp + 0x10], ecx
0x54da76: fld dword ptr [esp + 0x10]
0x54da7a: mov dword ptr [esp + 0x14], edi
0x54da7e: fucompp 
0x54da80: fnstsw ax
0x54da82: test ah, 0x44
0x54da85: jp 0x54da98
0x54da87: movsx edx, word ptr [edi + 4]
0x54da8b: imul edx, edx, 0x2c
0x54da8e: mov eax, dword ptr [edx + 0x69eafc]
0x54da94: mov dword ptr [esp + 0x10], eax
0x54da98: mov ecx, dword ptr [esp + 0x10]
0x54da9c: lea esi, [ebp + 0xc]
0x54da9f: push ecx
0x54daa0: mov eax, esi
0x54daa2: call 0x54bb20
0x54daa7: add esp, 4
0x54daaa: cmp ax, 0xffff
0x54daae: mov dword ptr [esp + 0x10], eax
0x54dab2: je 0x54dbf2
0x54dab8: mov edx, dword ptr [0x7252c0]
0x54dabe: call 0x4d0480
0x54dac3: cmp eax, -1
0x54dac6: mov dword ptr [esp + 0x1c], eax
0x54daca: je 0x54dbea
0x54dad0: mov ecx, dword ptr [edx + 0x34]
0x54dad3: mov edx, dword ptr [esp + 0x28]
0x54dad7: mov ebx, eax
0x54dad9: mov ax, word ptr [esp + 0x10]
0x54dade: and ebx, 0xffff
0x54dae4: imul ebx, ebx, 0xb0
0x54daea: add ebx, ecx
0x54daec: mov dword ptr [ebx + 8], edx
0x54daef: mov word ptr [ebx + 0x8c], 0xffff
0x54daf8: mov word ptr [ebx + 6], ax
0x54dafc: mov word ptr [ebx + 4], 0
0x54db02: mov ecx, dword ptr [edi + 0x18]
0x54db05: mov edx, dword ptr [edi + 0x14]
0x54db08: push ecx
0x54db09: push edx
0x54db0a: call 0x444af0
0x54db0f: mov eax, dword ptr [esp + 0x2c]
0x54db13: fst dword ptr [ebx + 0x88]
0x54db19: mov dword ptr [ebx + 0xc], eax
0x54db1c: mov ax, word ptr [esp + 0x34]
0x54db21: lea edi, [ebx + 0x14]
0x54db24: mov ecx, 0x10
0x54db29: rep movsd dword ptr es:[edi], dword ptr [esi]
0x54db2b: mov cx, word ptr [esp + 0x38]
0x54db30: mov esi, dword ptr [esp + 0x1c]
0x54db34: mov word ptr [ebx + 2], cx
0x54db38: mov edx, dword ptr [0x72520c]
0x54db3e: mov word ptr [ebx + 0x94], ax
0x54db45: xor edi, edi
0x54db47: mov eax, 0xffffffff
0x54db4c: mov dword ptr [ebx + 0x84], edx
0x54db52: mov dword ptr [ebx + 0x10], 0x54dc10
0x54db59: mov dword ptr [ebx + 0xa8], edi
0x54db5f: mov dword ptr [ebx + 0xa4], edi
0x54db65: mov dword ptr [ebx + 0x98], eax
0x54db6b: add esp, 4
0x54db6e: fld dword ptr [esi + 0x44]
0x54db71: mov ecx, esi
0x54db73: fld dword ptr [esi + 0x5c]
0x54db76: fsub st(1)
0x54db78: fmul dword ptr [esp + 0x1c]
0x54db7c: fadd st(1)
0x54db7e: fmul st(2)
0x54db80: fstp dword ptr [esp]
0x54db83: fstp st(0)
0x54db85: fstp st(0)
0x54db87: call 0x5454a0
0x54db8c: push esi
0x54db8d: or ecx, 0xffffffff
0x54db90: mov word ptr [ebx + 0x8e], ax
0x54db97: call 0x545590
0x54db9c: mov ecx, dword ptr [ebx + 8]
0x54db9f: mov edx, dword ptr [0x87bc14]
0x54dba5: push edi
0x54dba6: and ecx, 0xffff
0x54dbac: movsx edi, ax
0x54dbaf: mov word ptr [ebx + 0x90], ax
0x54dbb6: imul edi, edi, 0x7c
0x54dbb9: shl ecx, 5
0x54dbbc: mov edx, dword ptr [ecx + edx + 0x14]
0x54dbc0: movsx ecx, word ptr [ebx + 0x8e]
0x54dbc7: mov edx, dword ptr [edx + 0x9c]
0x54dbcd: lea ecx, [ecx + ecx*8]
0x54dbd0: mov esi, dword ptr [edx + ecx*8 + 0x40]
0x54dbd4: push 1
0x54dbd6: add edi, esi
0x54dbd8: xor bl, bl
0x54dbda: call 0x443e10
0x54dbdf: mov eax, dword ptr [esp + 0x2c]
0x54dbe3: add esp, 0x10
0x54dbe6: inc word ptr [ebp + 0x50]
0x54dbea: pop edi
0x54dbeb: pop esi
0x54dbec: pop ebp
0x54dbed: pop ebx
0x54dbee: add esp, 0x10
0x54dbf1: ret 
0x54dbf2: pop edi
0x54dbf3: pop esi
0x54dbf4: pop ebp
0x54dbf5: mov eax, ebx
0x54dbf7: pop ebx
0x54dbf8: add esp, 0x10
0x54dbfb: ret 
0x54dbfc: pop esi
0x54dbfd: pop ebp
0x54dbfe: mov eax, ebx
0x54dc00: pop ebx
0x54dc01: add esp, 0x10
0x54dc04: ret 
0x54dc05: pop ebp
0x54dc06: mov eax, ebx
0x54dc08: pop ebx
0x54dc09: add esp, 0x10
0x54dc0c: ret 
#endif
