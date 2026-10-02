// sound_update_looping_gain  (Ghidra: FUN_0054deb0, still unnamed there)
// address 0x54deb0, size 848 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/sound_functions.md "Computes and applies the current gain and playback
// state for a looping playing sound instance, triggering detail sounds and starting/continuing
// its driver voice."; the sound_channel_parameters stack block (0x20 bytes: minimum_distance,
// maximum_distance, pitch, gain, inner/outer_cone_angle, outer_cone_gain, eax_value) matches
// types/sound.h exactly; driver vtable calls at +0x1c (channel_continue) and (inside
// sound_channel_set_next_permutation, +0x18/channel_play) match the established sound_driver
// layout. This is one of the largest, most register-heavy functions in this module; it was
// re-checked instruction by instruction in the phase-4 review (see below).
//
// register convention: stack -> (channel_index, external_gain_multiplier), both recognized.
// Phase-4 review (disassembly appended below): the EDI permutation arguments and the crossfade
// (EBX = new loop sound, fade out = this channel's sound, curve 1, 0.5 s) are confirmed. Fixed:
// the pitch-range test uses the unbent pitch (the draft passed the bent, rate-scaled one); the
// first permutation pick passes the current permutation index in CX (the draft passed -1, which
// is only the retry's value); the "no permutation left" test compared an int16 with 0xffff and
// could never fire. The 0x54e200 fragment Ghidra split off (the state-machine tail) is part of
// this function.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *sound_data;         // 0x007252c0, "sounds" 0x200 x 0xb0
extern tag_instance *tag_instances;    // 0x0087bc14
extern data_array *looping_sound_data; // 0x00724a50, "looping sounds" 0x80 x 0xe4
extern sound_class_definition sound_class_definitions[k_maximum_sound_classes]; // 0x0069eae0
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60
extern float sound_music_gain;         // 0x007252a8
extern uint8_t sound_idle_update_active; // 0x00725203
extern sound_channel_parameters_proc sound_channel_parameters_proc_ptr; // 0x006e36cc
extern sound_driver *current_sound_driver; // 0x00725208, header calls this "sound_driver"

extern float sound_compute_class_gain(SoundClass_t sound_class); // this module, 0x54b100
extern void sound_instance_stop(datum_index sound_handle); // this module, 0x54b180
extern void sound_channel_set_next_permutation(int16_t channel_index, SoundPermutation *permutation,
    int16_t unknown, int16_t sound_class, uint8_t streaming); // this module, 0x54cd30
extern int16_t sound_channel_release_detail_buffers(int16_t channel_index); // this module, 0x54d020
extern datum_index sound_looping_create_detail_sound(datum_index owner, datum_index definition_index,
    int16_t track_index, int16_t play_state); // this module, 0x54d9f0
extern void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle); // this module, 0x54af60
extern float sound_clamp_gain_by_ratio(float gain, float compare, float ratio); // this module, 0x54e660
extern void sound_instance_apply_pending_definition_switch(datum_index sound_handle); // this module, 0x54ddc0
extern int16_t sound_permutation_pick_for_pitch(int16_t pitch_range_index, Sound *tag, float target_pitch); // 0x5454a0, other half of this module
extern int16_t sound_permutation_pick_random(int16_t pitch_range_index, int16_t explicit_permutation_index,
    Sound *tag); // 0x545590, other half of this module
extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, cache module

// blam-cc: stack -> (channel_index, external_gain_multiplier)
// Computes the current gain/pitch/cone parameters for the sound playing on `channel_index`
// (scaled by `external_gain_multiplier`), pushes them to the driver, and drives the looping
// track's playback state machine: starting the channel the first time it is assigned, crossfading
// in a successor detail sound when the pitch range needs to change mid-loop, and otherwise
// picking a new permutation (or applying a queued definition switch) once the current one is
// exhausted.
void sound_update_looping_gain(int16_t channel_index, float external_gain_multiplier)
{
    datum_index sound_handle;
    sound *instance;
    Sound *definition;
    looping_sound *owner_state;
    SoundLooping *looping_definition;
    SoundLoopingTrack *track;
    float scale;
    float pitch;
    float zero_gain, class_gain, gain;
    sound_channel_parameters params;
    SoundPitchRange *pitch_range;
    SoundPermutation *permutation;
    int16_t streaming_flag = 0;

    sound_handle = sound_channels[channel_index].sound_index;
    instance = (sound *)((uint8_t *)sound_data->data + (sound_handle & 0xffff) * sizeof(sound));
    scale = instance->location.scale;
    definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;
    owner_state = (looping_sound *)((uint8_t *)looping_sound_data->data +
        (instance->owner_index & 0xffff) * sizeof(looping_sound));
    looping_definition = (SoundLooping *)tag_instances[owner_state->definition_index & 0xffff].data;
    track = (SoundLoopingTrack *)looping_definition->tracks.pointer + instance->track_index;

    pitch = ((definition->one_pitch_modifier - definition->zero_pitch_modifier) * scale +
        definition->zero_pitch_modifier) * instance->pitch;

    params.minimum_distance = definition->minimum_distance;
    if (params.minimum_distance == 0.0f) {
        params.minimum_distance = sound_class_definitions[definition->sound_class].default_minimum_distance;
    }
    params.maximum_distance = 3.4028235e+38f;
    params.pitch = pitch;
    params.inner_cone_angle = definition->inner_cone_angle;
    params.outer_cone_angle = definition->outer_cone_angle;
    params.outer_cone_gain = definition->outer_cone_gain;
    params.eax_value = sound_class_definitions[definition->sound_class].eax_value;

    zero_gain = definition->zero_gain_modifier;
    class_gain = sound_compute_class_gain(definition->sound_class);
    gain = (definition->one_gain_modifier - zero_gain) * scale + zero_gain;
    gain = gain * class_gain * track->gain * definition->random_gain_modifier * instance->location.gain *
        external_gain_multiplier;
    params.gain = gain;

    if (instance->channel_index == -1) {
        if (definition->sound_class == soundclass_music && sound_music_gain == 0.0f) {
            return;
        }
        pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        params.gain = params.gain * permutation->gain;
        params.pitch = pitch * pitch_range->playback_rate;
        sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
        sound_channel_set_next_permutation(channel_index, permutation, instance->first_person,
            definition->sound_class, 0);
        instance->channel_index = channel_index;
        current_sound_driver->channel_continue(channel_index, 0, definition->sound_class);
        return;
    }

    if (definition->sound_class == soundclass_music && sound_music_gain == 0.0f) {
        sound_instance_stop(sound_handle);
        return;
    }

    pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
    {
        float bent_pitch = sound_clamp_gain_by_ratio(pitch, sound_channels[channel_index].current_pitch *
            pitch_range->natural_pitch, definition->maximum_bend_per_second);
        params.pitch = bent_pitch * pitch_range->playback_rate;
    }

    if (instance->play_state == _sound_play_loop &&
        (instance->fade_start_time == instance->fade_end_time || instance->fade_end_gain != 0.0f) &&
        sound_permutation_pick_for_pitch(instance->pitch_range_index, definition, pitch) != instance->pitch_range_index &&
        sound_channels[channel_index].sound_index == owner_state->track_sounds[instance->track_index] &&
        !sound_idle_update_active) {
        datum_index detail_handle = sound_looping_create_detail_sound(instance->owner_index,
            instance->definition_index, instance->track_index, _sound_play_loop);
        if (detail_handle != 0xffffffff) {
            sound_schedule_gain_fade(detail_handle, _sound_fade_power, 0.5f, sound_channels[channel_index].sound_index);
            owner_state->track_sounds[instance->track_index] = detail_handle;
        }
    }

    if (instance->play_state != _sound_play_loop_end &&
        (instance->play_state != _sound_play_loop_start || !(track->flags & 1)) &&
        (sound_channel_release_detail_buffers(channel_index) != 2 ||
         (instance->flags & _sound_permutation_pending_bit) != 0 ||
         (sound_channels[channel_index].current_permutation != 0 &&
          sound_channels[channel_index].current_permutation->next_permutation_index == 0xffff &&
          instance->pending_definition_index != 0xffffffff))) {

        if (instance->pending_definition_index == 0xffffffff ||
            (sound_channels[channel_index].current_permutation != 0 &&
             sound_channels[channel_index].current_permutation->next_permutation_index != 0xffff)) {
            if (!(instance->flags & _sound_permutation_pending_bit)) {
                int16_t new_permutation = sound_permutation_pick_random(instance->pitch_range_index,
                    instance->permutation_index, definition);
                streaming_flag = 1;
                if (new_permutation == -1) {
                    if (!(looping_definition->flags & 2)) { // !not_a_loop
                        new_permutation = sound_permutation_pick_random(instance->pitch_range_index, -1, definition);
                        if (new_permutation != -1) {
                            instance->flags |= _sound_permutation_pending_bit;
                            instance->permutation_index = new_permutation;
                        }
                    } else {
                        instance->play_state = _sound_play_loop_end;
                        owner_state->finished = 1;
                    }
                } else {
                    instance->flags |= _sound_permutation_pending_bit;
                    instance->permutation_index = new_permutation;
                }
            }
        } else {
            sound_instance_apply_pending_definition_switch(sound_handle);
            definition = (Sound *)tag_instances[instance->definition_index & 0xffff].data;
            pitch_range = (SoundPitchRange *)definition->pitch_ranges.pointer + instance->pitch_range_index;
        }

        permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
        if (instance->play_state != _sound_play_loop_end &&
            sound_cache_touch(1, 1, 0, permutation)) {
            instance->flags &= ~_sound_permutation_pending_bit;
            sound_channel_set_next_permutation(channel_index, permutation, instance->first_person,
                definition->sound_class, 1);
            if (instance->pending_definition_index == 0xffffffff && permutation->next_permutation_index == 0xffff) {
                if (instance->play_state == _sound_play_loop_start) {
                    instance->play_state = _sound_play_loop;
                } else if (instance->play_state == _sound_play_loop_stopping) {
                    instance->play_state = _sound_play_loop_end;
                }
            }
        }
    }

    permutation = (SoundPermutation *)pitch_range->permutations.pointer + instance->permutation_index;
    params.gain = params.gain * permutation->gain;
    sound_channel_parameters_proc_ptr(channel_index, &params, 0, definition->sound_class);
    current_sound_driver->channel_continue(channel_index, streaming_flag, definition->sound_class);
}

#if 0
Original Ghidra decompilation (0x54deb0): see out/phase2/sound/01.md and
scratchpad/sound_packs/0x54deb0.md for the full 848-byte listing.

Disassembly (0x54deb0..0x54e3ba, capstone; phase-4 review):

0x54deb0: sub esp, 0x44
0x54deb3: movsx eax, word ptr [esp + 0x48]
0x54deb8: push ebx
0x54deb9: lea eax, [eax + eax*2]
0x54debc: lea eax, [eax*8 + 0x724a60]
0x54dec3: mov dword ptr [esp + 8], eax
0x54dec7: mov eax, dword ptr [eax]
0x54dec9: mov edx, dword ptr [0x724a50]
0x54decf: push ebp
0x54ded0: push esi
0x54ded1: mov esi, eax
0x54ded3: and esi, 0xffff
0x54ded9: imul esi, esi, 0xb0
0x54dedf: mov dword ptr [esp + 0x1c], eax
0x54dee3: mov eax, dword ptr [0x7252c0]
0x54dee8: mov ebx, dword ptr [eax + 0x34]
0x54deeb: mov ecx, dword ptr [esi + ebx + 8]
0x54deef: mov eax, dword ptr [0x87bc14]
0x54def4: add esi, ebx
0x54def6: mov ebx, dword ptr [edx + 0x34]
0x54def9: mov edx, dword ptr [esi + 0x18]
0x54defc: and ecx, 0xffff
0x54df02: shl ecx, 5
0x54df05: mov ebp, dword ptr [ecx + eax + 0x14]
0x54df09: push edi
0x54df0a: mov edi, dword ptr [esi + 0xc]
0x54df0d: fld dword ptr [ebp + 0x44]
0x54df10: mov dword ptr [esp + 0x18], edx
0x54df14: fld dword ptr [ebp + 0x5c]
0x54df17: and edi, 0xffff
0x54df1d: fsub st(1)
0x54df1f: imul edi, edi, 0xe4
0x54df25: mov ecx, dword ptr [edi + ebx + 4]
0x54df29: add edi, ebx
0x54df2b: fmul dword ptr [esp + 0x18]
0x54df2f: movsx ebx, word ptr [esi + 0x94]
0x54df36: and ecx, 0xffff
0x54df3c: fadd st(1)
0x54df3e: shl ecx, 5
0x54df41: mov eax, dword ptr [ecx + eax + 0x14]
0x54df45: mov ecx, dword ptr [eax + 0x40]
0x54df48: mov dword ptr [esp + 0x24], ebx
0x54df4c: fmul dword ptr [esi + 0x88]
0x54df52: lea ebx, [ebx + ebx*4]
0x54df55: mov dword ptr [esp + 0x30], eax
0x54df59: shl ebx, 5
0x54df5c: fstp dword ptr [esp + 0x10]
0x54df60: add ebx, ecx
0x54df62: mov byte ptr [esp + 0x28], 0
0x54df67: fstp st(0)
0x54df69: mov dword ptr [esp + 0x2c], ebx
0x54df6d: fld dword ptr [ebp + 8]
0x54df70: fld dword ptr [0x672ac0]
0x54df76: fld st(1)
0x54df78: fucompp 
0x54df7a: fnstsw ax
0x54df7c: test ah, 0x44
0x54df7f: jp 0x54df90
0x54df81: movsx eax, word ptr [ebp + 4]
0x54df85: fstp st(0)
0x54df87: imul eax, eax, 0x2c
0x54df8a: fld dword ptr [eax + 0x69eaf8]
0x54df90: fstp dword ptr [esp + 0x34]
0x54df94: mov dword ptr [esp + 0x38], 0x7f7fffff
0x54df9c: mov ecx, dword ptr [ebp + 0x1c]
0x54df9f: mov dword ptr [esp + 0x44], ecx
0x54dfa3: mov edx, dword ptr [ebp + 0x20]
0x54dfa6: mov dword ptr [esp + 0x48], edx
0x54dfaa: mov eax, dword ptr [ebp + 0x24]
0x54dfad: mov dword ptr [esp + 0x4c], eax
0x54dfb1: movsx ecx, word ptr [ebp + 4]
0x54dfb5: imul ecx, ecx, 0x2c
0x54dfb8: mov edx, dword ptr [ecx + 0x69eaf0]
0x54dfbe: mov dword ptr [esp + 0x50], edx
0x54dfc2: mov eax, dword ptr [ebp + 0x40]
0x54dfc5: mov dword ptr [esp + 0x1c], eax
0x54dfc9: mov ax, word ptr [ebp + 4]
0x54dfcd: call 0x54b100
0x54dfd2: fld dword ptr [ebp + 0x58]
0x54dfd5: fsub dword ptr [esp + 0x1c]
0x54dfd9: fmul dword ptr [esp + 0x18]
0x54dfdd: fadd dword ptr [esp + 0x1c]
0x54dfe1: fmulp st(1)
0x54dfe3: fmul dword ptr [ebx + 4]
0x54dfe6: fmul dword ptr [ebp + 0x28]
0x54dfe9: fmul dword ptr [esi + 0x1c]
0x54dfec: fmul dword ptr [esp + 0x5c]
0x54dff0: fstp dword ptr [esp + 0x40]
0x54dff4: mov cx, word ptr [esi + 0x8c]
0x54dffb: cmp cx, -1
0x54dfff: jne 0x54e09e
0x54e005: cmp word ptr [ebp + 4], 0x20
0x54e00a: jne 0x54e025
0x54e00c: fld dword ptr [0x672ac0]
0x54e012: fld dword ptr [0x7252a8]
0x54e018: fucompp 
0x54e01a: fnstsw ax
0x54e01c: test ah, 0x44
0x54e01f: jnp 0x54e3b2
0x54e025: movsx edi, word ptr [esi + 0x90]
0x54e02c: fld dword ptr [esp + 0x40]
0x54e030: movsx eax, word ptr [esi + 0x8e]
0x54e037: imul edi, edi, 0x7c
0x54e03a: mov edx, dword ptr [ebp + 0x9c]
0x54e040: lea ecx, [eax + eax*8]
0x54e043: lea eax, [edx + ecx*8]
0x54e046: mov edx, dword ptr [eax + 0x40]
0x54e049: mov ebx, dword ptr [esp + 0x58]
0x54e04d: add edi, edx
0x54e04f: lea ecx, [esp + 0x34]
0x54e053: fmul dword ptr [edi + 0x24]
0x54e056: fstp dword ptr [esp + 0x40]
0x54e05a: fld dword ptr [esp + 0x10]
0x54e05e: fmul dword ptr [eax + 0x30]
0x54e061: xor eax, eax
0x54e063: fstp dword ptr [esp + 0x3c]
0x54e067: mov ax, word ptr [ebp + 4]
0x54e06b: push eax
0x54e06c: push 0
0x54e06e: push ecx
0x54e06f: push ebx
0x54e070: call dword ptr [0x6e36cc]
0x54e076: xor edx, edx
0x54e078: mov dx, word ptr [ebp + 4]
0x54e07c: xor eax, eax
0x54e07e: mov al, byte ptr [esi + 0xac]
0x54e084: push 0
0x54e086: mov ecx, ebx
0x54e088: push edx
0x54e089: push eax
0x54e08a: call 0x54cd30
0x54e08f: add esp, 0x1c
0x54e092: mov word ptr [esi + 0x8c], bx
0x54e099: jmp 0x54e399
0x54e09e: cmp word ptr [ebp + 4], 0x20
0x54e0a3: jne 0x54e0cf
0x54e0a5: fld dword ptr [0x672ac0]
0x54e0ab: fld dword ptr [0x7252a8]
0x54e0b1: fucompp 
0x54e0b3: fnstsw ax
0x54e0b5: test ah, 0x44
0x54e0b8: jp 0x54e0cf
0x54e0ba: mov ecx, dword ptr [esp + 0x20]
0x54e0be: push ecx
0x54e0bf: call 0x54b180
0x54e0c4: add esp, 4
0x54e0c7: pop edi
0x54e0c8: pop esi
0x54e0c9: pop ebp
0x54e0ca: pop ebx
0x54e0cb: add esp, 0x44
0x54e0ce: ret 
0x54e0cf: movsx eax, word ptr [esi + 0x8e]
0x54e0d6: lea edx, [eax + eax*8]
0x54e0d9: mov eax, dword ptr [ebp + 0x9c]
0x54e0df: lea edx, [eax + edx*8]
0x54e0e2: mov eax, dword ptr [ebp + 0x2c]
0x54e0e5: push eax
0x54e0e6: movsx eax, cx
0x54e0e9: lea ecx, [eax + eax*2]
0x54e0ec: mov eax, dword ptr [esp + 0x14]
0x54e0f0: push ecx
0x54e0f1: fld dword ptr [ecx*8 + 0x724a6c]
0x54e0f8: mov dword ptr [esp + 0x64], edx
0x54e0fc: fmul dword ptr [edx + 0x20]
0x54e0ff: fstp dword ptr [esp]
0x54e102: push eax
0x54e103: call 0x54e660
0x54e108: fst dword ptr [esp + 0x1c]
0x54e10c: fmul dword ptr [edx + 0x30]
0x54e10f: add esp, 0xc
0x54e112: fstp dword ptr [esp + 0x3c]
0x54e116: cmp word ptr [esi + 2], 2
0x54e11b: jne 0x54e1d1
0x54e121: mov ecx, dword ptr [esi + 0xa4]
0x54e127: cmp ecx, dword ptr [esi + 0xa8]
0x54e12d: je 0x54e148
0x54e12f: fld dword ptr [0x672ac0]
0x54e135: fld dword ptr [esi + 0xa0]
0x54e13b: fucompp 
0x54e13d: fnstsw ax
0x54e13f: test ah, 0x44
0x54e142: jnp 0x54e1d1
0x54e148: mov edx, dword ptr [esp + 0x10]
0x54e14c: xor eax, eax
0x54e14e: mov ax, word ptr [esi + 0x8e]
0x54e155: push edx
0x54e156: mov ecx, ebp
0x54e158: call 0x5454a0
0x54e15d: add esp, 4
0x54e160: cmp ax, word ptr [esi + 0x8e]
0x54e167: je 0x54e1d1
0x54e169: mov ecx, dword ptr [esp + 0x24]
0x54e16d: mov eax, dword ptr [esp + 0x20]
0x54e171: cmp eax, dword ptr [edi + ecx*4 + 0xd4]
0x54e178: jne 0x54e1d1
0x54e17a: mov al, byte ptr [0x725203]
0x54e17f: test al, al
0x54e181: jne 0x54e1d1
0x54e183: mov eax, dword ptr [esi + 8]
0x54e186: mov ecx, dword ptr [esi + 0xc]
0x54e189: xor edx, edx
0x54e18b: mov dx, word ptr [esi + 0x94]
0x54e192: push 2
0x54e194: push edx
0x54e195: push eax
0x54e196: push ecx
0x54e197: call 0x54d9f0
0x54e19c: add esp, 0x10
0x54e19f: cmp eax, -1
0x54e1a2: mov dword ptr [esp + 0x20], eax
0x54e1a6: je 0x54e1d1
0x54e1a8: mov edx, dword ptr [esp + 0x14]
0x54e1ac: mov ecx, dword ptr [edx]
0x54e1ae: push ecx
0x54e1af: push 0x3f000000
0x54e1b4: push 1
0x54e1b6: mov ebx, eax
0x54e1b8: call 0x54af60
0x54e1bd: mov edx, dword ptr [esp + 0x30]
0x54e1c1: mov eax, ebx
0x54e1c3: mov ebx, dword ptr [esp + 0x38]
0x54e1c7: add esp, 0xc
0x54e1ca: mov dword ptr [edi + edx*4 + 0xd4], eax
0x54e1d1: mov ax, word ptr [esi + 2]
0x54e1d5: cmp ax, 4
0x54e1d9: je 0x54e35c
0x54e1df: cmp ax, 1
0x54e1e3: jne 0x54e1ee
0x54e1e5: test byte ptr [ebx], 1
0x54e1e8: jne 0x54e35c
0x54e1ee: mov cx, word ptr [esi + 0x8c]
0x54e1f5: call 0x54d020
0x54e1fa: cmp ax, 2
0x54e1fe: jne 0x54e228
0x54e200: test byte ptr [esi + 4], 8
0x54e204: jne 0x54e228
0x54e206: mov ecx, dword ptr [esp + 0x14]
0x54e20a: mov edx, dword ptr [ecx + 0x10]
0x54e20d: or ebx, 0xffffffff
0x54e210: cmp word ptr [edx + 0x2a], bx
0x54e214: jne 0x54e35c
0x54e21a: cmp dword ptr [esi + 0x98], ebx
0x54e220: je 0x54e35c
0x54e226: jmp 0x54e22f
0x54e228: mov ecx, dword ptr [esp + 0x14]
0x54e22c: or ebx, 0xffffffff
0x54e22f: cmp dword ptr [esi + 0x98], ebx
0x54e235: je 0x54e279
0x54e237: mov eax, dword ptr [ecx + 0x10]
0x54e23a: test eax, eax
0x54e23c: je 0x54e244
0x54e23e: cmp word ptr [eax + 0x2a], bx
0x54e242: jne 0x54e279
0x54e244: mov ebx, dword ptr [ecx]
0x54e246: call 0x54ddc0
0x54e24b: mov eax, dword ptr [esi + 8]
0x54e24e: mov ecx, dword ptr [0x87bc14]
0x54e254: and eax, 0xffff
0x54e259: shl eax, 5
0x54e25c: mov ebp, dword ptr [eax + ecx + 0x14]
0x54e260: movsx eax, word ptr [esi + 0x8e]
0x54e267: lea edx, [eax + eax*8]
0x54e26a: mov eax, dword ptr [ebp + 0x9c]
0x54e270: lea ecx, [eax + edx*8]
0x54e273: mov dword ptr [esp + 0x5c], ecx
0x54e277: jmp 0x54e2d3
0x54e279: test byte ptr [esi + 4], 8
0x54e27d: jne 0x54e2d3
0x54e27f: mov cx, word ptr [esi + 0x90]
0x54e286: xor eax, eax
0x54e288: mov ax, word ptr [esi + 0x8e]
0x54e28f: push ebp
0x54e290: call 0x545590
0x54e295: add esp, 4
0x54e298: cmp ax, bx
0x54e29b: mov byte ptr [esp + 0x28], 1
0x54e2a0: jne 0x54e2c8
0x54e2a2: mov edx, dword ptr [esp + 0x30]
0x54e2a6: test byte ptr [edx], 2
0x54e2a9: jne 0x54e344
0x54e2af: xor eax, eax
0x54e2b1: mov ax, word ptr [esi + 0x8e]
0x54e2b8: push ebp
0x54e2b9: mov ecx, ebx
0x54e2bb: call 0x545590
0x54e2c0: add esp, 4
0x54e2c3: cmp ax, bx
0x54e2c6: je 0x54e2d3
0x54e2c8: or byte ptr [esi + 4], 8
0x54e2cc: mov word ptr [esi + 0x90], ax
0x54e2d3: movsx edi, word ptr [esi + 0x90]
0x54e2da: mov eax, dword ptr [esp + 0x5c]
0x54e2de: imul edi, edi, 0x7c
0x54e2e1: add edi, dword ptr [eax + 0x40]
0x54e2e4: cmp word ptr [esi + 2], 4
0x54e2e9: je 0x54e35c
0x54e2eb: push 1
0x54e2ed: push 1
0x54e2ef: xor bl, bl
0x54e2f1: call 0x443e10
0x54e2f6: add esp, 8
0x54e2f9: test al, al
0x54e2fb: je 0x54e35c
0x54e2fd: and byte ptr [esi + 4], 0xf7
0x54e301: xor ecx, ecx
0x54e303: mov cx, word ptr [ebp + 4]
0x54e307: xor edx, edx
0x54e309: mov dl, byte ptr [esi + 0xac]
0x54e30f: push 1
0x54e311: push ecx
0x54e312: mov ecx, dword ptr [esp + 0x60]
0x54e316: push edx
0x54e317: call 0x54cd30
0x54e31c: mov ecx, dword ptr [esi + 0x98]
0x54e322: or eax, 0xffffffff
0x54e325: add esp, 0xc
0x54e328: cmp ecx, eax
0x54e32a: jne 0x54e35c
0x54e32c: cmp word ptr [edi + 0x2a], ax
0x54e330: jne 0x54e35c
0x54e332: mov ax, word ptr [esi + 2]
0x54e336: cmp ax, 1
0x54e33a: jne 0x54e350
0x54e33c: mov word ptr [esi + 2], 2
0x54e342: jmp 0x54e35c
0x54e344: mov word ptr [esi + 2], 4
0x54e34a: mov byte ptr [edi + 0x4e], 1
0x54e34e: jmp 0x54e2d3
0x54e350: cmp ax, 3
0x54e354: jne 0x54e35c
0x54e356: mov word ptr [esi + 2], 4
0x54e35c: movsx eax, word ptr [esi + 0x90]
0x54e363: fld dword ptr [esp + 0x40]
0x54e367: mov ecx, dword ptr [esp + 0x5c]
0x54e36b: imul eax, eax, 0x7c
0x54e36e: mov edx, dword ptr [ecx + 0x40]
0x54e371: lea ecx, [esp + 0x34]
0x54e375: fmul dword ptr [eax + edx + 0x24]
0x54e379: mov edx, dword ptr [esp + 0x58]
0x54e37d: xor eax, eax
0x54e37f: fstp dword ptr [esp + 0x40]
0x54e383: mov ax, word ptr [ebp + 4]
0x54e387: push eax
0x54e388: push 0
0x54e38a: push ecx
0x54e38b: push edx
0x54e38c: call dword ptr [0x6e36cc]
0x54e392: mov ebx, dword ptr [esp + 0x68]
0x54e396: add esp, 0x10
0x54e399: mov ecx, dword ptr [esp + 0x28]
0x54e39d: mov edx, dword ptr [0x725208]
0x54e3a3: xor eax, eax
0x54e3a5: mov ax, word ptr [ebp + 4]
0x54e3a9: push eax
0x54e3aa: push ecx
0x54e3ab: push ebx
0x54e3ac: call dword ptr [edx + 0x1c]
0x54e3af: add esp, 0xc
0x54e3b2: pop edi
0x54e3b3: pop esi
0x54e3b4: pop ebp
0x54e3b5: pop ebx
0x54e3b6: add esp, 0x44
0x54e3b9: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
