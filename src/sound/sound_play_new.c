// sound_play_new  (Ghidra: FUN_00549af0)
// address 0x549af0, size 1003 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: types/sound.h explicitly attributes this address as one of the two "sound"
//   constructors ("Constructors 0x549af0 and 0x54d9f0 write 0x02, 0x04, 0x06, 0x08, 0x0c,
//   0x10, 0x14..0x54, ..."), and every field write below lines up with that struct's offsets
//   exactly: definition_index 0x08 <- param_1, owner_index 0x0c <- param_3, location_proc
//   0x10 <- param_4, location 0x14 (0x40-byte copy) <- *param_2, callback_data 0x54 <- memcpy
//   from param_5/param_6, pitch 0x88 <- FUN_0054aec0(...), channel_index 0x8c <- -1,
//   pitch_range_index 0x8e <- sound_permutation_pick_for_pitch, permutation_index 0x90 <-
//   sound_permutation_pick_random, track_index 0x94 <- -1, first_person 0xac, listener_index
//   0x06 <- FUN_0054bb20's result (header: "0x54bb20 result, stride 0x44"), start_time 0x84 <-
//   sound_time (or sound_time + a computed distance-based delay), flags 0x04 bit 0x01
//   (_sound_delayed_start_bit) set on the delayed path -- matching the header's own note.
//   Sound tag field reads (sound_class 0x04, sample_rate 0x06, format 0x6e, channel_count 0x6c,
//   random_pitch_bounds 0x14/0x18, zero_/one_pitch_modifier 0x44/0x5c, zero_/one_skip_fraction
//   0x3c/0x54, skip_fraction 0x10, longest_permutation_length 0x84, promotion_sound.tag_id 0x7c)
//   match types/tags.h's Sound struct at every offset used.
// register convention: plain stack, seven arguments (Ghidra recognized all seven).
// blam-cc: stack -> (definition_index, location, owner_index, location_proc, callback_data,
//   callback_data_size, first_person_hint)
// Phase-4 review: every callee argument below was resolved from the capstone disassembly
//   (appended in the #if 0 block): sound_definition_maximum_distance / _has_audible_permutations
//   take the definition in EAX, sound_definition_check_promotion in ECX,
//   sound_location_check_audibility takes the location in EAX and the distance on the stack,
//   sound_location_distance the location in ECX and the listener index on the stack,
//   sound_permutation_pick_for_pitch (AX -1, ECX tag, pitch), sound_permutation_pick_random (AX
//   pitch range, CX -1, tag) and sound_cache_touch (stack 1, 0; BL 0; EDI permutation).
//   The start delay is distance * 8.964706 ms (0x0065e560, the speed of sound in world units)
//   truncated by __ftol; sounds more than 250 ms away start late with _sound_delayed_start_bit.
//   sound.flags is stored from DX, which the code zeroed just before the pitch call (the pitch
//   helper does not touch EDX), so it is 0.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern data_array *sound_data;      // 0x007252c0

extern game_time_globals *game_time; // 0x006f1d6c
extern int32_t ai_communication_quiet_until_tick; // 0x00725204
extern uint8_t sound_dialog_unspatialized;       // 0x007252bc
extern uint8_t sound_initialized;   // 0x00725200
extern uint8_t sound_enabled;       // 0x00725201
extern uint8_t sound_disabled;      // 0x007252b6
extern random_seed effect_random_seed; // 0x00719cd4, see src/sound/sound_compute_random_pitch.c
extern int32_t sound_time;          // 0x0072520c
extern uint8_t *cinematic_globals_ptr;   // 0x006f187c, UNSURE (see src/game/game_engine_update_local_player_control.c)

extern datum_index datum_new(data_array *array); // 0x4d0480, blam-cc: EDX -> array
extern uint8_t sound_cache_touch(uint8_t allocate_if_missing, uint8_t lock, uint8_t wait_until_loaded,
    void *permutation); // 0x443e10, src/cache/sound_cache_touch.c; blam-cc: stack, stack, BL, EDI
extern float sound_definition_maximum_distance(datum_index sound_definition); // 0x545460, src/sound/sound_definition_maximum_distance.c
extern uint32_t sound_definition_has_audible_permutations(TagID sound_tag_id); // 0x54af10, blam-cc: EAX
    // src/sound/sound_definition_has_audible_permutations.c (outside this module's range)
extern int16_t sound_location_check_audibility(sound_location *location, float max_distance); // 0x54bb20, blam-cc: EAX, stack
extern int16_t sound_definition_check_promotion(TagID sound_tag_id); // 0x54b050, blam-cc: ECX
    // src/sound/sound_definition_check_promotion.c (outside this module's range)
extern float sound_location_distance(int16_t listener_index, sound_location *location); // 0x54bc50, blam-cc: stack, ECX
extern const float sound_delay_per_world_unit; // 0x0065e560, 8.964706 ms: sound travel time per world unit
extern float sound_compute_random_pitch(float pitch_bounds_min, float pitch_bounds_max,
    float zero_pitch_modifier, float one_pitch_modifier, float distance_scale); // 0x54aec0
    // src/sound/sound_compute_random_pitch.c (outside this module's range)
extern int16_t sound_permutation_pick_for_pitch(int16_t pitch_range_index, Sound *tag, float target_pitch); // 0x5454a0
extern int16_t sound_permutation_pick_random(int16_t pitch_range_index, int16_t explicit_permutation_index,
    Sound *tag); // 0x545590, src/sound/sound_permutation_pick_random.c

// blam-cc: stack -> (definition_index, location, owner_index, location_proc, callback_data,
// callback_data_size, first_person_hint)
// Creates a new playing-sound datum for `definition_index`, applying the scripted-dialog
// suppression window, format/channel validation, a skip-fraction probability roll, pitch-range
// and permutation selection, and a distance-based start-time delay. Recurses through the
// definition's promotion_sound when the per-object retrigger throttle asks for a substitute.
datum_index sound_play_new(datum_index definition_index, sound_location *location, datum_index owner_index,
    sound_location_proc location_proc, void *callback_data, int32_t callback_data_size, uint32_t first_person_hint)
{
    Sound *tag = (Sound *)tag_instances[definition_index & 0xffff].data;
    datum_index handle = k_datum_index_none;

    if (tag->sound_class == soundclass_scripted_dialog_player ||
        tag->sound_class == soundclass_scripted_dialog_other ||
        tag->sound_class == soundclass_scripted_dialog_force_unspatialized) {
        int32_t suppress_until = ((int32_t)tag->longest_permutation_length * 30) / 1000 + 10 + game_time->game_time;
        if (ai_communication_quiet_until_tick < suppress_until) {
            ai_communication_quiet_until_tick = suppress_until;
        }
        if (sound_dialog_unspatialized != 0) {
            location->type = _sound_location_none;
        }
    }
    if (tag->sound_class == soundclass_scripted_dialog_force_unspatialized) {
        location->type = _sound_location_none;
    }

    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        if ((tag->format != soundformat_xbox_adpcm && tag->format != soundformat_ogg_vorbis &&
             tag->format != soundformat_16_bit_pcm) ||
            ((tag->channel_count != 0 || tag->sample_rate != 0) && tag->channel_count != 1)) {
            return k_datum_index_none;
        }

        if ((location->scale != 0.0f || tag->zero_gain_modifier != 0.0f)) {
            effect_random_seed = effect_random_seed * 0x19660d + 0x3c6ef35f;
            if (((tag->one_skip_fraction_modifier - tag->zero_skip_fraction_modifier) * location->scale +
                 tag->zero_skip_fraction_modifier) * tag->skip_fraction <
                (float)(effect_random_seed >> 16) * 1.5259022e-05f) {
                float maximum_distance = sound_definition_maximum_distance(definition_index);

                if (sound_definition_has_audible_permutations(*(TagID *)&definition_index) != 0) {
                    int16_t bucket = sound_location_check_audibility(location, maximum_distance);

                    if (bucket != -1) {
                        int16_t retrigger = sound_definition_check_promotion(*(TagID *)&definition_index);

                        if (retrigger != 0) {
                            if (retrigger != 1) {
                                return k_datum_index_none;
                            }
                            return sound_play_new(*(uint32_t *)&tag->promotion_sound.tag_id, location,
                                owner_index, location_proc, callback_data, callback_data_size, first_person_hint);
                        }

                        handle = datum_new(sound_data);
                        if (handle != k_datum_index_none) {
                            sound *self = &((sound *)sound_data->data)[(uint16_t)handle];
                            int32_t delay = (int32_t)(sound_location_distance(bucket, location) * sound_delay_per_world_unit);

                            self->first_person = 0;
                            if (((uint8_t)first_person_hint != 0 &&
                                 (tag->sound_class == soundclass_weapon_fire ||
                                  (tag->sound_class == soundclass_weapon_ready &&
                                   cinematic_globals_ptr[10] == 0 && cinematic_globals_ptr[9] == 0) ||
                                  tag->sound_class == soundclass_weapon_reload ||
                                  tag->sound_class == soundclass_weapon_empty ||
                                  tag->sound_class == soundclass_weapon_charge ||
                                  tag->sound_class == soundclass_weapon_overheat ||
                                  tag->sound_class == soundclass_weapon_idle)) ||
                                (cinematic_globals_ptr[9] != 0 && location->type != 0 &&
                                 (tag->sound_class == soundclass_scripted_dialog_player ||
                                  tag->sound_class == soundclass_scripted_dialog_other))) {
                                self->first_person = 1;
                            }

                            self->definition_index = definition_index;
                            self->channel_index = -1;
                            self->listener_index = bucket;
                            self->play_state = _sound_play_impulse;

                            self->pitch = sound_compute_random_pitch(tag->random_pitch_bounds[0], tag->random_pitch_bounds[1],
                                tag->zero_pitch_modifier, tag->one_pitch_modifier, location->scale);
                            self->flags = 0; // DX, zeroed before the pitch call, see file header
                            self->owner_index = owner_index;
                            self->location = *location;
                            self->location_proc = location_proc;

                            if (location_proc != 0) {
                                uint8_t *dst = self->callback_data;
                                uint8_t *src = (uint8_t *)callback_data;
                                int16_t count = (int16_t)callback_data_size;
                                int16_t i;
                                for (i = 0; i < count; i++) {
                                    dst[i] = src[i];
                                }
                            }

                            self->pitch_range_index = sound_permutation_pick_for_pitch(-1, tag, self->pitch);
                            self->permutation_index = sound_permutation_pick_random(self->pitch_range_index, -1, tag);
                            self->fade_end_time = 0;
                            self->fade_start_time = 0;
                            self->track_index = -1;

                            {
                                SoundPitchRange *range = (SoundPitchRange *)tag->pitch_ranges.pointer + self->pitch_range_index;
                                sound_cache_touch(1, 0, 0, (SoundPermutation *)range->permutations.pointer + self->permutation_index);
                            }

                            if (delay <= 250) {
                                self->start_time = sound_time;
                                return handle;
                            }
                            self->flags |= _sound_delayed_start_bit;
                            self->start_time = sound_time + delay;
                        }
                    }
                }
            }
        }
    }

    return handle;
}

#if 0
Original Ghidra decompilation (0x549af0):

uint FUN_00549af0(uint param_1,short *param_2,undefined4 param_3,int param_4,undefined4 *param_5,
                 undefined4 param_6,undefined4 param_7)

{
  int iVar1;
  char cVar2;
  short sVar3;
  undefined2 uVar4;
  undefined4 uVar5;
  int iVar6;
  int iVar7;
  uint uVar8;
  undefined2 extraout_DX;
  int iVar9;
  undefined4 *puVar10;
  float10 fVar11;
  undefined8 uVar12;
  uint local_8;

  iVar1 = *(int *)((param_1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  sVar3 = *(short *)(iVar1 + 4);
  local_8 = 0xffffffff;
  if (((sVar3 == 0x2c) || (sVar3 == 0x2e)) || (sVar3 == 0x2f)) {
    iVar7 = (*(int *)(iVar1 + 0x84) * 0x1e) / 1000 + 10 + *(int *)(DAT_006f1d6c + 0xc);
    if (DAT_00725204 < iVar7) {
      DAT_00725204 = iVar7;
    }
    if (DAT_007252bc != '\0') {
      *param_2 = 0;
    }
  }
  if (*(short *)(iVar1 + 4) == 0x2f) {
    *param_2 = 0;
  }
  if (((DAT_00725200 != '\0') && (DAT_00725201 != '\0')) && (DAT_007252b6 == '\0')) {
    sVar3 = *(short *)(iVar1 + 0x6e);
    if ((((sVar3 != 1) && (sVar3 != 3)) && (sVar3 != 0)) ||
       (((*(short *)(iVar1 + 0x6c) != 0 || (*(short *)(iVar1 + 6) != 0)) &&
        (*(short *)(iVar1 + 0x6c) != 1)))) {
      return 0xffffffff;
    }
    if (((*(float *)(param_2 + 2) != 0.0) || (*(float *)(iVar1 + 0x40) != 0.0)) &&
       (DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f,
       ((*(float *)(iVar1 + 0x54) - *(float *)(iVar1 + 0x3c)) * *(float *)(param_2 + 2) +
       *(float *)(iVar1 + 0x3c)) * *(float *)(iVar1 + 0x10) <
       (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05)) {
      fVar11 = (float10)FUN_00545460();
      cVar2 = FUN_0054af10();
      if (cVar2 != '\0') {
        uVar5 = FUN_0054bb20((float)fVar11);
        if ((short)uVar5 != -1) {
          sVar3 = FUN_0054b050();
          if (sVar3 != 0) {
            if (sVar3 != 1) {
              return 0xffffffff;
            }
            uVar8 = FUN_00549af0(*(undefined4 *)(iVar1 + 0x7c),param_2,param_3,param_4,param_5,
                                 param_6,param_7);
            return uVar8;
          }
          uVar12 = datum_new();
          local_8 = (uint)uVar12;
          if (local_8 != 0xffffffff) {
            iVar9 = (local_8 & 0xffff) * 0xb0 + *(int *)((int)((ulonglong)uVar12 >> 0x20) + 0x34);
            FUN_0054bc50(uVar5);
            iVar6 = __ftol();
            iVar7 = DAT_006f187c;
            *(undefined1 *)(iVar9 + 0xac) = 0;
            if ((((char)param_7 != '\0') &&
                (((sVar3 = *(short *)(iVar1 + 4), sVar3 == 4 ||
                  (((sVar3 == 5 && (*(char *)(iVar7 + 10) == '\0')) &&
                   (*(char *)(iVar7 + 9) == '\0')))) ||
                 ((((sVar3 == 6 || (sVar3 == 7)) || (sVar3 == 8)) || ((sVar3 == 9 || (sVar3 == 10)))
                  ))))) ||
               (((*(char *)(iVar7 + 9) != '\0' && (*param_2 != 0)) &&
                ((*(short *)(iVar1 + 4) == 0x2c || (*(short *)(iVar1 + 4) == 0x2e)))))) {
              *(undefined1 *)(iVar9 + 0xac) = 1;
            }
            *(uint *)(iVar9 + 8) = param_1;
            *(undefined2 *)(iVar9 + 0x8c) = 0xffff;
            *(short *)(iVar9 + 6) = (short)uVar5;
            *(undefined2 *)(iVar9 + 2) = 0;
            fVar11 = (float10)FUN_0054aec0(*(undefined4 *)(iVar1 + 0x14),
                                           *(undefined4 *)(iVar1 + 0x18),
                                           *(undefined4 *)(iVar1 + 0x44),
                                           *(undefined4 *)(iVar1 + 0x5c),
                                           *(undefined4 *)(param_2 + 2));
            *(float *)(iVar9 + 0x88) = (float)fVar11;
            *(undefined2 *)(iVar9 + 4) = extraout_DX;
            *(undefined4 *)(iVar9 + 0xc) = param_3;
            puVar10 = (undefined4 *)(iVar9 + 0x14);
            for (iVar7 = 0x10; iVar7 != 0; iVar7 = iVar7 + -1) {
              *puVar10 = *(undefined4 *)param_2;
              param_2 = param_2 + 2;
              puVar10 = puVar10 + 1;
            }
            *(int *)(iVar9 + 0x10) = param_4;
            if (param_4 != 0) {
              puVar10 = (undefined4 *)(iVar9 + 0x54);
              for (uVar8 = (uint)(int)(short)param_6 >> 2; uVar8 != 0; uVar8 = uVar8 - 1) {
                *puVar10 = *param_5;
                param_5 = param_5 + 1;
                puVar10 = puVar10 + 1;
              }
              for (uVar8 = (int)(short)param_6 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
                *(undefined1 *)puVar10 = *(undefined1 *)param_5;
                param_5 = (undefined4 *)((int)param_5 + 1);
                puVar10 = (undefined4 *)((int)puVar10 + 1);
              }
            }
            uVar4 = sound_permutation_pick_for_pitch(*(undefined4 *)(iVar9 + 0x88));
            *(undefined2 *)(iVar9 + 0x8e) = uVar4;
            uVar4 = sound_permutation_pick_random(iVar1);
            *(undefined4 *)(iVar9 + 0xa8) = 0;
            *(undefined4 *)(iVar9 + 0xa4) = 0;
            *(undefined2 *)(iVar9 + 0x90) = uVar4;
            *(undefined2 *)(iVar9 + 0x94) = 0xffff;
            sound_cache_touch(1,0);
            if (iVar6 < 0xfb) {
              *(int *)(iVar9 + 0x84) = DAT_0072520c;
              return local_8;
            }
            iVar6 = DAT_0072520c + iVar6;
            *(byte *)(iVar9 + 4) = *(byte *)(iVar9 + 4) | 1;
            *(int *)(iVar9 + 0x84) = iVar6;
          }
        }
      }
    }
  }
  return local_8;
}

Disassembly (0x549af0..0x549edb, capstone; phase-4 review):

0x549af0: sub esp, 8
0x549af3: mov ecx, dword ptr [0x87bc14]
0x549af9: push ebx
0x549afa: push ebp
0x549afb: mov ebp, dword ptr [esp + 0x14]
0x549aff: mov eax, ebp
0x549b01: and eax, 0xffff
0x549b06: shl eax, 5
0x549b09: mov ebx, dword ptr [eax + ecx + 0x14]
0x549b0d: mov ax, word ptr [ebx + 4]
0x549b11: cmp ax, 0x2c
0x549b15: push esi
0x549b16: mov esi, dword ptr [esp + 0x1c]
0x549b1a: mov edx, dword ptr [esi + 4]
0x549b1d: mov dword ptr [esp + 0xc], 0xffffffff
0x549b25: mov dword ptr [esp + 0x10], edx
0x549b29: je 0x549b37
0x549b2b: cmp ax, 0x2e
0x549b2f: je 0x549b37
0x549b31: cmp ax, 0x2f
0x549b35: jne 0x549b7b
0x549b37: mov ecx, dword ptr [ebx + 0x84]
0x549b3d: imul ecx, ecx, 0x1e
0x549b40: mov eax, 0x10624dd3
0x549b45: imul ecx
0x549b47: mov ecx, dword ptr [0x6f1d6c]
0x549b4d: sar edx, 6
0x549b50: mov eax, edx
0x549b52: shr eax, 0x1f
0x549b55: add eax, edx
0x549b57: mov edx, dword ptr [ecx + 0xc]
0x549b5a: mov ecx, dword ptr [0x725204]
0x549b60: lea eax, [eax + edx + 0xa]
0x549b64: cmp eax, ecx
0x549b66: jle 0x549b6d
0x549b68: mov dword ptr [0x725204], eax
0x549b6d: mov al, byte ptr [0x7252bc]
0x549b72: test al, al
0x549b74: je 0x549b7b
0x549b76: mov word ptr [esi], 0
0x549b7b: cmp word ptr [ebx + 4], 0x2f
0x549b80: jne 0x549b87
0x549b82: mov word ptr [esi], 0
0x549b87: mov al, byte ptr [0x725200]
0x549b8c: test al, al
0x549b8e: je 0x549e62
0x549b94: mov al, byte ptr [0x725201]
0x549b99: test al, al
0x549b9b: je 0x549e62
0x549ba1: mov al, byte ptr [0x7252b6]
0x549ba6: test al, al
0x549ba8: jne 0x549e62
0x549bae: mov ax, word ptr [ebx + 0x6e]
0x549bb2: cmp ax, 1
0x549bb6: je 0x549bc7
0x549bb8: cmp ax, 3
0x549bbc: je 0x549bc7
0x549bbe: test ax, ax
0x549bc1: jne 0x549ed1
0x549bc7: mov ax, word ptr [ebx + 0x6c]
0x549bcb: test ax, ax
0x549bce: jne 0x549bd6
0x549bd0: cmp word ptr [ebx + 6], ax
0x549bd4: je 0x549be0
0x549bd6: cmp ax, 1
0x549bda: jne 0x549ed1
0x549be0: fld dword ptr [0x672ac0]
0x549be6: fld dword ptr [esi + 4]
0x549be9: fucompp 
0x549beb: fnstsw ax
0x549bed: test ah, 0x44
0x549bf0: jp 0x549c08
0x549bf2: fld dword ptr [0x672ac0]
0x549bf8: fld dword ptr [ebx + 0x40]
0x549bfb: fucompp 
0x549bfd: fnstsw ax
0x549bff: test ah, 0x44
0x549c02: jnp 0x549e62
0x549c08: mov eax, dword ptr [0x719cd4]
0x549c0d: imul eax, eax, 0x19660d
0x549c13: add eax, 0x3c6ef35f
0x549c18: mov ecx, eax
0x549c1a: mov dword ptr [0x719cd4], eax
0x549c1f: shr ecx, 0x10
0x549c22: fld dword ptr [ebx + 0x3c]
0x549c25: mov dword ptr [esp + 0x1c], ecx
0x549c29: fild dword ptr [esp + 0x1c]
0x549c2d: fmul dword ptr [0x672b84]
0x549c33: fld dword ptr [ebx + 0x54]
0x549c36: fsub st(2)
0x549c38: fmul dword ptr [esp + 0x10]
0x549c3c: fadd st(2)
0x549c3e: fmul dword ptr [ebx + 0x10]
0x549c41: fcompp 
0x549c43: fnstsw ax
0x549c45: fstp st(0)
0x549c47: test ah, 5
0x549c4a: jp 0x549e62
0x549c50: mov eax, ebp
0x549c52: call 0x545460
0x549c57: fstp dword ptr [esp + 0x1c]
0x549c5b: mov eax, ebp
0x549c5d: call 0x54af10
0x549c62: test al, al
0x549c64: je 0x549e62
0x549c6a: mov edx, dword ptr [esp + 0x1c]
0x549c6e: push edi
0x549c6f: push edx
0x549c70: mov eax, esi
0x549c72: call 0x54bb20
0x549c77: add esp, 4
0x549c7a: mov edi, eax
0x549c7c: cmp di, -1
0x549c80: je 0x549e61
0x549c86: mov ecx, ebp
0x549c88: call 0x54b050
0x549c8d: test ax, ax
0x549c90: jne 0x549e85
0x549c96: mov edx, dword ptr [0x7252c0]
0x549c9c: call 0x4d0480
0x549ca1: cmp eax, -1
0x549ca4: mov dword ptr [esp + 0x10], eax
0x549ca8: je 0x549e61
0x549cae: mov ebp, eax
0x549cb0: mov eax, dword ptr [edx + 0x34]
0x549cb3: and ebp, 0xffff
0x549cb9: imul ebp, ebp, 0xb0
0x549cbf: push edi
0x549cc0: mov ecx, esi
0x549cc2: add ebp, eax
0x549cc4: call 0x54bc50
0x549cc9: add esp, 4
0x549ccc: fmul dword ptr [0x65e560]
0x549cd2: call 0x6391b4
0x549cd7: mov ecx, dword ptr [0x6f187c]
0x549cdd: mov dword ptr [esp + 0x20], eax
0x549ce1: mov al, byte ptr [esp + 0x34]
0x549ce5: test al, al
0x549ce7: mov byte ptr [ebp + 0xac], 0
0x549cee: je 0x549d2c
0x549cf0: mov ax, word ptr [ebx + 4]
0x549cf4: cmp ax, 4
0x549cf8: je 0x549d49
0x549cfa: cmp ax, 5
0x549cfe: jne 0x549d0e
0x549d00: mov dl, byte ptr [ecx + 0xa]
0x549d03: test dl, dl
0x549d05: jne 0x549d0e
0x549d07: mov dl, byte ptr [ecx + 9]
0x549d0a: test dl, dl
0x549d0c: je 0x549d49
0x549d0e: cmp ax, 6
0x549d12: je 0x549d49
0x549d14: cmp ax, 7
0x549d18: je 0x549d49
0x549d1a: cmp ax, 8
0x549d1e: je 0x549d49
0x549d20: cmp ax, 9
0x549d24: je 0x549d49
0x549d26: cmp ax, 0xa
0x549d2a: je 0x549d49
0x549d2c: mov al, byte ptr [ecx + 9]
0x549d2f: test al, al
0x549d31: je 0x549d50
0x549d33: cmp word ptr [esi], 0
0x549d37: je 0x549d50
0x549d39: mov ax, word ptr [ebx + 4]
0x549d3d: cmp ax, 0x2c
0x549d41: je 0x549d49
0x549d43: cmp ax, 0x2e
0x549d47: jne 0x549d50
0x549d49: mov byte ptr [ebp + 0xac], 1
0x549d50: mov eax, dword ptr [esp + 0x1c]
0x549d54: mov dword ptr [ebp + 8], eax
0x549d57: mov word ptr [ebp + 0x8c], 0xffff
0x549d60: mov word ptr [ebp + 6], di
0x549d64: xor edx, edx
0x549d66: mov word ptr [ebp + 2], dx
0x549d6a: mov ecx, dword ptr [esi + 4]
0x549d6d: mov eax, dword ptr [ebx + 0x5c]
0x549d70: push ecx
0x549d71: mov ecx, dword ptr [ebx + 0x44]
0x549d74: push eax
0x549d75: mov eax, dword ptr [ebx + 0x18]
0x549d78: push ecx
0x549d79: mov ecx, dword ptr [ebx + 0x14]
0x549d7c: push eax
0x549d7d: push ecx
0x549d7e: call 0x54aec0
0x549d83: fstp dword ptr [ebp + 0x88]
0x549d89: mov eax, dword ptr [esp + 0x3c]
0x549d8d: mov word ptr [ebp + 4], dx
0x549d91: mov edx, dword ptr [esp + 0x38]
0x549d95: lea edi, [ebp + 0x14]
0x549d98: mov ecx, 0x10
0x549d9d: add esp, 0x14
0x549da0: test eax, eax
0x549da2: mov dword ptr [ebp + 0xc], edx
0x549da5: rep movsd dword ptr es:[edi], dword ptr [esi]
0x549da7: mov dword ptr [ebp + 0x10], eax
0x549daa: je 0x549dc6
0x549dac: movsx ecx, word ptr [esp + 0x30]
0x549db1: mov esi, dword ptr [esp + 0x2c]
0x549db5: mov eax, ecx
0x549db7: shr ecx, 2
0x549dba: lea edi, [ebp + 0x54]
0x549dbd: rep movsd dword ptr es:[edi], dword ptr [esi]
0x549dbf: mov ecx, eax
0x549dc1: and ecx, 3
0x549dc4: rep movsb byte ptr es:[edi], byte ptr [esi]
0x549dc6: mov ecx, dword ptr [ebp + 0x88]
0x549dcc: push ecx
0x549dcd: or eax, 0xffffffff
0x549dd0: mov ecx, ebx
0x549dd2: call 0x5454a0
0x549dd7: push ebx
0x549dd8: or ecx, 0xffffffff
0x549ddb: mov word ptr [ebp + 0x8e], ax
0x549de2: call 0x545590
0x549de7: mov edx, dword ptr [ebp + 8]
0x549dea: xor ecx, ecx
0x549dec: mov dword ptr [ebp + 0xa8], ecx
0x549df2: mov dword ptr [ebp + 0xa4], ecx
0x549df8: push ecx
0x549df9: mov ecx, dword ptr [0x87bc14]
0x549dff: and edx, 0xffff
0x549e05: movsx edi, ax
0x549e08: mov word ptr [ebp + 0x90], ax
0x549e0f: imul edi, edi, 0x7c
0x549e12: mov word ptr [ebp + 0x94], 0xffff
0x549e1b: shl edx, 5
0x549e1e: mov edx, dword ptr [edx + ecx + 0x14]
0x549e22: movsx ecx, word ptr [ebp + 0x8e]
0x549e29: mov edx, dword ptr [edx + 0x9c]
0x549e2f: lea ecx, [ecx + ecx*8]
0x549e32: mov esi, dword ptr [edx + ecx*8 + 0x40]
0x549e36: push 1
0x549e38: add edi, esi
0x549e3a: xor bl, bl
0x549e3c: call 0x443e10
0x549e41: mov eax, dword ptr [esp + 0x30]
0x549e45: add esp, 0x10
0x549e48: cmp eax, 0xfa
0x549e4d: jle 0x549e6d
0x549e4f: mov ecx, dword ptr [0x72520c]
0x549e55: add ecx, eax
0x549e57: or byte ptr [ebp + 4], 1
0x549e5b: mov dword ptr [ebp + 0x84], ecx
0x549e61: pop edi
0x549e62: mov eax, dword ptr [esp + 0xc]
0x549e66: pop esi
0x549e67: pop ebp
0x549e68: pop ebx
0x549e69: add esp, 8
0x549e6c: ret 
0x549e6d: mov edx, dword ptr [0x72520c]
0x549e73: mov eax, dword ptr [esp + 0x10]
0x549e77: pop edi
0x549e78: pop esi
0x549e79: mov dword ptr [ebp + 0x84], edx
0x549e7f: pop ebp
0x549e80: pop ebx
0x549e81: add esp, 8
0x549e84: ret 
0x549e85: cmp ax, 1
0x549e89: jne 0x549ebd
0x549e8b: mov eax, dword ptr [esp + 0x34]
0x549e8f: mov ecx, dword ptr [esp + 0x30]
0x549e93: mov edx, dword ptr [esp + 0x2c]
0x549e97: push eax
0x549e98: mov eax, dword ptr [esp + 0x2c]
0x549e9c: push ecx
0x549e9d: mov ecx, dword ptr [esp + 0x2c]
0x549ea1: push edx
0x549ea2: mov edx, dword ptr [ebx + 0x7c]
0x549ea5: push eax
0x549ea6: push ecx
0x549ea7: push esi
0x549ea8: push edx
0x549ea9: call 0x549af0
0x549eae: add esp, 0x1c
0x549eb1: pop edi
0x549eb2: pop esi
0x549eb3: pop ebp
0x549eb4: mov dword ptr [esp + 4], eax
0x549eb8: pop ebx
0x549eb9: add esp, 8
0x549ebc: ret 
0x549ebd: pop edi
0x549ebe: pop esi
0x549ebf: mov dword ptr [esp + 8], 0xffffffff
0x549ec7: mov eax, dword ptr [esp + 8]
0x549ecb: pop ebp
0x549ecc: pop ebx
0x549ecd: add esp, 8
0x549ed0: ret 
0x549ed1: pop esi
0x549ed2: pop ebp
0x549ed3: or eax, 0xffffffff
0x549ed6: pop ebx
0x549ed7: add esp, 8
0x549eda: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
