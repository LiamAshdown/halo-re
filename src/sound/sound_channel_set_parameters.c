// sound_channel_set_parameters  (Ghidra: FUN_005475b0)
// address 0x5475b0, size 723 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Updates a channel's frequency, volume, and
//   (for positional channels) distance and cone parameters whenever they change."; matches
//   sound_driver.channel_set_parameters's own signature comment in types/sound.h exactly
//   ("(int16_t channel_index, sound_channel_parameters *parameters, uint8_t unknown)"); every
//   directsound_channel cache field (gain/volume/pitch/frequency/minimum_distance/
//   maximum_distance/inner_cone_angle/outer_cone_angle/cone_outside_gain/eax_value at
//   0x03c/0x070/0x040/0x06c/0x04c/0x050/0x058/0x05c/0x054/0x060) matches types/sound.h by offset;
//   IDirectSoundBuffer vtable slots 0x3c/0x44 are SetVolume/SetFrequency, IDirectSound3DBuffer
//   slots 0x40/0x44/0x34/0x3c are SetMaxDistance/SetMinDistance/SetConeAngles/
//   SetConeOutsideVolume; sound_linear_gain_to_attenuation (0x545710) for the cone outside gain.
// register convention: channel index and `update` are this function's own recognized stack
//   parameters; the sound_channel_parameters* is in EDI (unaff_EDI).
// blam-cc: stack -> (channel_index, update), EDI -> parameters
// Phase-4 review (disassembly appended below): the frequency is pitch * the channel's sample
//   rate (0x0065e4f8 by the 44 kHz bit) clamped to [188, 191983] (the draft passed the raw
//   pitch); SetConeAngles and SetConeOutsideVolume take DS3D_DEFERRED (1) as their last argument,
//   and the cone outside volume goes to the 3D buffer (vtable +0x3c of IDirectSound3DBuffer, the
//   draft called SetVolume on the 2D buffer); the EAX refresh is gated on the effects object's
//   channel_supported (+0x10), not listener_supported. Cone angles are radians * 57.29578
//   truncated; the volume is log10(fade * gain) * 2000 clamped to [-10000, 0], -10000 for 0.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern float directsound_fade; // 0x0074611c
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern uint8_t directsound_initialized; // 0x007252e0
extern int32_t directsound_hardware_mode; // 0x0074612c
extern uint8_t directsound_deferred_dirty; // 0x00746132
extern sound_effect_object *global_sound_effect_object; // 0x00721f24

extern double log10(double x); // see src/sound/sound_linear_gain_to_attenuation.c
extern int32_t k_sound_sample_rates[2]; // 0x0065e4f8, { 22050, 44100 }
extern int32_t sound_linear_gain_to_attenuation(float gain, int32_t maximum); // 0x545710

static float sound_linear_gain_to_attenuation_fabs(float x) { return (x < 0.0f) ? -x : x; }

// blam-cc: stack -> (channel_index, update), EDI -> parameters
// Commits a channel's volume (directsound_fade * gain, converted to a clamped DirectSound
// attenuation), and -- unless `update` is set -- its frequency and, for 3D channels, distance,
// cone-angle, cone-outside-gain and EAX value parameters, each only when changed.
void sound_channel_set_parameters(int16_t channel_index, sound_channel_parameters *parameters, uint8_t update)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    float effective_gain = directsound_fade * parameters->gain;

    if (sound_linear_gain_to_attenuation_fabs(effective_gain - channel->gain) >= 0.001f ||
        directsound_initialized == 0) {
        int32_t volume;

        if (effective_gain == 0.0f) {
            volume = -10000;
        } else {
            volume = (int32_t)(log10((double)effective_gain) * 2000.0);
            if (volume < -10000) {
                volume = -10000;
            } else if (volume > 0) {
                volume = 0;
            }
        }

        {
            void **vtable = *(void ***)channel->buffer;
            int32_t (__stdcall *set_volume)(void *, int32_t) = (int32_t (__stdcall *)(void *, int32_t))vtable[0x3c / 4];
            channel->volume = volume;
            set_volume(channel->buffer, volume);
        }
        channel->gain = effective_gain;
    }

    if (update == 0) {
        if (sound_linear_gain_to_attenuation_fabs(parameters->pitch - channel->pitch) >= 0.001f ||
            directsound_initialized == 0) {
            // playback rate times the channel's sample rate, clamped to [188, 191983] Hz
            float hertz = parameters->pitch *
                (float)k_sound_sample_rates[(channel->type_flags & _sound_channel_44khz_bit) >> 2];
            int32_t frequency;

            if (hertz < 188.0f) {
                hertz = 188.0f;
            } else if (hertz > 191983.0f) {
                hertz = 191983.0f;
            }
            frequency = (int32_t)hertz;

            if (directsound_hardware_mode != 3) {
                void **vtable = *(void ***)channel->buffer;
                int32_t (__stdcall *set_frequency)(void *, int32_t) = (int32_t (__stdcall *)(void *, int32_t))vtable[0x44 / 4];
                set_frequency(channel->buffer, frequency);
            }
            channel->pitch = parameters->pitch;
            channel->frequency = (int16_t)frequency;
            directsound_deferred_dirty = 1;
        }

        if ((channel->type_flags & _sound_channel_3d_bit) != 0) {
            void **vtable_3d = *(void ***)channel->buffer_3d;

            if (sound_linear_gain_to_attenuation_fabs(parameters->maximum_distance - channel->maximum_distance) >= 0.05f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_max_distance)(void *, float, uint32_t) =
                    (int32_t (__stdcall *)(void *, float, uint32_t))vtable_3d[0x40 / 4];
                set_max_distance(channel->buffer_3d, parameters->maximum_distance, 1);
                channel->maximum_distance = parameters->maximum_distance;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->minimum_distance - channel->minimum_distance) >= 0.05f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_min_distance)(void *, float, uint32_t) =
                    (int32_t (__stdcall *)(void *, float, uint32_t))vtable_3d[0x44 / 4];
                set_min_distance(channel->buffer_3d, parameters->minimum_distance, 1);
                channel->minimum_distance = parameters->minimum_distance;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->inner_cone_angle - channel->inner_cone_angle) >= 0.034906585f ||
                sound_linear_gain_to_attenuation_fabs(parameters->outer_cone_angle - channel->outer_cone_angle) >= 0.034906585f ||
                directsound_initialized == 0) {
                int32_t (__stdcall *set_cone_angles)(void *, uint32_t, uint32_t, uint32_t) =
                    (int32_t (__stdcall *)(void *, uint32_t, uint32_t, uint32_t))vtable_3d[0x34 / 4];
                uint32_t outer_degrees = (uint32_t)(int32_t)(parameters->outer_cone_angle * 57.29578f);
                uint32_t inner_degrees = (uint32_t)(int32_t)(parameters->inner_cone_angle * 57.29578f);
                set_cone_angles(channel->buffer_3d, inner_degrees, outer_degrees, 1); // DS3D_DEFERRED
                channel->inner_cone_angle = parameters->inner_cone_angle;
                channel->outer_cone_angle = parameters->outer_cone_angle;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->outer_cone_gain - channel->cone_outside_gain) >= 0.001f ||
                directsound_initialized == 0) {
                // IDirectSound3DBuffer::SetConeOutsideVolume (3D buffer vtable +0x3c), deferred
                int32_t (__stdcall *set_cone_outside_volume)(void *, int32_t, uint32_t) =
                    (int32_t (__stdcall *)(void *, int32_t, uint32_t))vtable_3d[0x3c / 4];
                int32_t volume = sound_linear_gain_to_attenuation(parameters->outer_cone_gain, 0);
                set_cone_outside_volume(channel->buffer_3d, volume, 1);
                channel->cone_outside_gain = parameters->outer_cone_gain;
                directsound_deferred_dirty = 1;
            }

            if (sound_linear_gain_to_attenuation_fabs(parameters->eax_value - channel->eax_value) >= 0.001f ||
                directsound_initialized == 0) {
                channel->eax_value = parameters->eax_value;
                if (global_sound_effect_object != 0) {
                    sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;
                    if (fx_vtable->channel_supported(global_sound_effect_object) != 0) {
                        fx_vtable->apply_channel(global_sound_effect_object, channel_index);
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5475b0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_005475b0(short param_1,char param_2)

{
  int *piVar1;
  float fVar2;
  int iVar3;
  undefined4 uVar4;
  int iVar5;
  int iVar6;
  float *unaff_EDI;
  bool bVar7;

  iVar5 = (int)param_1;
  fVar2 = _DAT_0074611c * unaff_EDI[3];
  iVar6 = iVar5 * 0x678;
  if ((0.001 <= ABS(fVar2 - (float)(&DAT_0072546c)[iVar5 * 0x19e])) || (DAT_007252e0 == '\0')) {
    if (fVar2 == 0.0) {
      iVar3 = -10000;
    }
    else {
      log2((float10)fVar2);
      iVar3 = __ftol();
      if (iVar3 < -10000) {
        iVar3 = -10000;
      }
      else if (0 < iVar3) {
        iVar3 = 0;
      }
    }
    piVar1 = (int *)(&DAT_00725aa0)[iVar5 * 0x19e];
    *(int *)(&DAT_007254a0 + iVar6) = iVar3;
    (**(code **)(*piVar1 + 0x3c))(piVar1,iVar3);
    (&DAT_0072546c)[iVar5 * 0x19e] = fVar2;
  }
  if (param_2 == '\0') {
    if ((0.001 <= ABS(unaff_EDI[2] - (float)(&DAT_00725470)[iVar5 * 0x19e])) ||
       (DAT_007252e0 == '\0')) {
      uVar4 = __ftol();
      if (DAT_0074612c != 3) {
        (**(code **)(*(int *)(&DAT_00725aa0)[iVar5 * 0x19e] + 0x44))
                  ((int *)(&DAT_00725aa0)[iVar5 * 0x19e],uVar4);
      }
      (&DAT_00725470)[iVar5 * 0x19e] = unaff_EDI[2];
      *(short *)(&DAT_0072549c + iVar6) = (short)uVar4;
      DAT_00746132 = 1;
    }
    if (((&DAT_00725468)[iVar6] & 1) != 0) {
      if ((0.05 <= ABS(unaff_EDI[1] - *(float *)(&DAT_00725480 + iVar6))) || (DAT_007252e0 == '\0'))
      {
        (**(code **)(*(int *)(&DAT_00725aa4)[iVar5 * 0x19e] + 0x40))
                  ((int *)(&DAT_00725aa4)[iVar5 * 0x19e],unaff_EDI[1],1);
        *(float *)(&DAT_00725480 + iVar6) = unaff_EDI[1];
        DAT_00746132 = 1;
      }
      if ((0.05 <= ABS(*unaff_EDI - *(float *)(&DAT_0072547c + iVar6))) || (DAT_007252e0 == '\0')) {
        (**(code **)(*(int *)(&DAT_00725aa4)[iVar5 * 0x19e] + 0x44))
                  ((int *)(&DAT_00725aa4)[iVar5 * 0x19e],*unaff_EDI,1);
        *(float *)(&DAT_0072547c + iVar6) = *unaff_EDI;
        DAT_00746132 = 1;
      }
      if (((0.034906585 <= ABS(unaff_EDI[4] - *(float *)(&DAT_00725488 + iVar6))) ||
          (0.034906585 <= ABS(unaff_EDI[5] - *(float *)(&DAT_0072548c + iVar6)))) ||
         (DAT_007252e0 == '\0')) {
        piVar1 = (int *)(&DAT_00725aa4)[iVar5 * 0x19e];
        iVar3 = *piVar1;
        uVar4 = __ftol(1);
        uVar4 = __ftol(uVar4);
        (**(code **)(iVar3 + 0x34))(piVar1,uVar4);
        *(float *)(&DAT_00725488 + iVar6) = unaff_EDI[4];
        *(float *)(&DAT_0072548c + iVar6) = unaff_EDI[5];
        DAT_00746132 = 1;
      }
      if ((0.001 <= ABS(unaff_EDI[6] - *(float *)(&DAT_00725484 + iVar6))) || (DAT_007252e0 == '\0')
         ) {
        piVar1 = (int *)(&DAT_00725aa4)[iVar5 * 0x19e];
        iVar5 = *piVar1;
        uVar4 = sound_linear_gain_to_attenuation(unaff_EDI[6],0,1);
        (**(code **)(iVar5 + 0x3c))(piVar1,uVar4);
        *(float *)(&DAT_00725484 + iVar6) = unaff_EDI[6];
        DAT_00746132 = 1;
      }
      piVar1 = DAT_00721f24;
      if (((0.001 <= ABS(unaff_EDI[7] - *(float *)(&DAT_00725490 + iVar6))) ||
          (DAT_007252e0 == '\0')) &&
         ((bVar7 = DAT_00721f24 != (int *)0x0, *(float *)(&DAT_00725490 + iVar6) = unaff_EDI[7],
          bVar7 && (iVar5 = (**(code **)(*piVar1 + 0x10))(), iVar5 != 0)))) {
        (**(code **)(*DAT_00721f24 + 0x14))((int)param_1);
      }
    }
  }
  return;
}

Disassembly (0x5475b0..0x547883, capstone; phase-4 review):

0x5475b0: push ecx
0x5475b1: fld dword ptr [0x74611c]
0x5475b7: push esi
0x5475b8: movsx esi, word ptr [esp + 0xc]
0x5475bd: fmul dword ptr [edi + 0xc]
0x5475c0: imul esi, esi, 0x678
0x5475c6: fst dword ptr [esp + 4]
0x5475ca: add esi, 0x725430
0x5475d0: fsub dword ptr [esi + 0x3c]
0x5475d3: fabs 
0x5475d5: fcomp qword ptr [0x672b18]
0x5475db: fnstsw ax
0x5475dd: test ah, 5
0x5475e0: jp 0x5475eb
0x5475e2: mov al, byte ptr [0x7252e0]
0x5475e7: test al, al
0x5475e9: jne 0x547645
0x5475eb: fld dword ptr [0x672ac0]
0x5475f1: fld dword ptr [esp + 4]
0x5475f5: fucompp 
0x5475f7: fnstsw ax
0x5475f9: test ah, 0x44
0x5475fc: jp 0x547605
0x5475fe: mov eax, 0xffffd8f0
0x547603: jmp 0x54762e
0x547605: fld dword ptr [esp + 4]
0x547609: fldlg2 
0x54760b: fxch st(1)
0x54760d: fyl2x 
0x54760f: fmul qword ptr [0x672b10]
0x547615: call 0x6391b4
0x54761a: cmp eax, 0xffffd8f0
0x54761f: jge 0x547628
0x547621: mov eax, 0xffffd8f0
0x547626: jmp 0x54762e
0x547628: test eax, eax
0x54762a: jle 0x54762e
0x54762c: xor eax, eax
0x54762e: mov ecx, dword ptr [esi + 0x670]
0x547634: push eax
0x547635: mov dword ptr [esi + 0x70], eax
0x547638: mov edx, dword ptr [ecx]
0x54763a: push ecx
0x54763b: call dword ptr [edx + 0x3c]
0x54763e: mov eax, dword ptr [esp + 4]
0x547642: mov dword ptr [esi + 0x3c], eax
0x547645: mov al, byte ptr [esp + 0x10]
0x547649: test al, al
0x54764b: jne 0x547880
0x547651: fld dword ptr [edi + 8]
0x547654: push ebx
0x547655: fsub dword ptr [esi + 0x40]
0x547658: push ebp
0x547659: mov ebx, 1
0x54765e: fabs 
0x547660: fcomp qword ptr [0x672b18]
0x547666: fnstsw ax
0x547668: test ah, 5
0x54766b: jp 0x547676
0x54766d: mov al, byte ptr [0x7252e0]
0x547672: test al, al
0x547674: jne 0x5476ec
0x547676: fld dword ptr [edi + 8]
0x547679: xor ecx, ecx
0x54767b: mov cl, byte ptr [esi + 0x38]
0x54767e: and ecx, 4
0x547681: shr ecx, 2
0x547684: mov edx, dword ptr [ecx*4 + 0x65e4f8]
0x54768b: mov dword ptr [esp + 0x18], edx
0x54768f: fimul dword ptr [esp + 0x18]
0x547693: fcom dword ptr [0x672b38]
0x547699: fnstsw ax
0x54769b: test ah, 5
0x54769e: jp 0x5476aa
0x5476a0: fstp st(0)
0x5476a2: fld dword ptr [0x672b38]
0x5476a8: jmp 0x5476bf
0x5476aa: fcom dword ptr [0x672b34]
0x5476b0: fnstsw ax
0x5476b2: test ah, 0x41
0x5476b5: jne 0x5476bf
0x5476b7: fstp st(0)
0x5476b9: fld dword ptr [0x672b34]
0x5476bf: call 0x6391b4
0x5476c4: mov ebp, eax
0x5476c6: cmp dword ptr [0x74612c], 3
0x5476cd: je 0x5476dc
0x5476cf: mov eax, dword ptr [esi + 0x670]
0x5476d5: mov ecx, dword ptr [eax]
0x5476d7: push ebp
0x5476d8: push eax
0x5476d9: call dword ptr [ecx + 0x44]
0x5476dc: mov edx, dword ptr [edi + 8]
0x5476df: mov dword ptr [esi + 0x40], edx
0x5476e2: mov word ptr [esi + 0x6c], bp
0x5476e6: mov byte ptr [0x746132], bl
0x5476ec: test byte ptr [esi + 0x38], bl
0x5476ef: je 0x54787e
0x5476f5: fld dword ptr [edi + 4]
0x5476f8: fsub dword ptr [esi + 0x50]
0x5476fb: fabs 
0x5476fd: fcomp qword ptr [0x672b28]
0x547703: fnstsw ax
0x547705: test ah, 5
0x547708: jp 0x547713
0x54770a: mov al, byte ptr [0x7252e0]
0x54770f: test al, al
0x547711: jne 0x547730
0x547713: mov edx, dword ptr [edi + 4]
0x547716: mov eax, dword ptr [esi + 0x674]
0x54771c: mov ecx, dword ptr [eax]
0x54771e: push ebx
0x54771f: push edx
0x547720: push eax
0x547721: call dword ptr [ecx + 0x40]
0x547724: mov eax, dword ptr [edi + 4]
0x547727: mov dword ptr [esi + 0x50], eax
0x54772a: mov byte ptr [0x746132], bl
0x547730: fld dword ptr [edi]
0x547732: fsub dword ptr [esi + 0x4c]
0x547735: fabs 
0x547737: fcomp qword ptr [0x672b28]
0x54773d: fnstsw ax
0x54773f: test ah, 5
0x547742: jp 0x54774d
0x547744: mov al, byte ptr [0x7252e0]
0x547749: test al, al
0x54774b: jne 0x547768
0x54774d: mov edx, dword ptr [edi]
0x54774f: mov eax, dword ptr [esi + 0x674]
0x547755: mov ecx, dword ptr [eax]
0x547757: push ebx
0x547758: push edx
0x547759: push eax
0x54775a: call dword ptr [ecx + 0x44]
0x54775d: mov eax, dword ptr [edi]
0x54775f: mov dword ptr [esi + 0x4c], eax
0x547762: mov byte ptr [0x746132], bl
0x547768: fld dword ptr [edi + 0x10]
0x54776b: fsub dword ptr [esi + 0x58]
0x54776e: fabs 
0x547770: fcomp qword ptr [0x672cd8]
0x547776: fnstsw ax
0x547778: test ah, 5
0x54777b: jp 0x54779b
0x54777d: fld dword ptr [edi + 0x14]
0x547780: fsub dword ptr [esi + 0x5c]
0x547783: fabs 
0x547785: fcomp qword ptr [0x672cd8]
0x54778b: fnstsw ax
0x54778d: test ah, 5
0x547790: jp 0x54779b
0x547792: mov al, byte ptr [0x7252e0]
0x547797: test al, al
0x547799: jne 0x5477e6
0x54779b: fld dword ptr [edi + 0x14]
0x54779e: mov ebp, dword ptr [esi + 0x674]
0x5477a4: mov eax, dword ptr [ebp]
0x5477a7: fmul dword ptr [0x672b30]
0x5477ad: push ebx
0x5477ae: mov dword ptr [esp + 0x1c], eax
0x5477b2: call 0x6391b4
0x5477b7: fld dword ptr [edi + 0x10]
0x5477ba: fmul dword ptr [0x672b30]
0x5477c0: push eax
0x5477c1: call 0x6391b4
0x5477c6: push eax
0x5477c7: mov eax, dword ptr [esp + 0x24]
0x5477cb: push ebp
0x5477cc: call dword ptr [eax + 0x34]
0x5477cf: mov ecx, dword ptr [edi + 0x10]
0x5477d2: mov dword ptr [esi + 0x58], ecx
0x5477d5: mov edx, dword ptr [edi + 0x14]
0x5477d8: mov ebx, 1
0x5477dd: mov dword ptr [esi + 0x5c], edx
0x5477e0: mov byte ptr [0x746132], bl
0x5477e6: fld dword ptr [edi + 0x18]
0x5477e9: fsub dword ptr [esi + 0x54]
0x5477ec: fabs 
0x5477ee: fcomp qword ptr [0x672b18]
0x5477f4: fnstsw ax
0x5477f6: test ah, 5
0x5477f9: jp 0x547804
0x5477fb: mov al, byte ptr [0x7252e0]
0x547800: test al, al
0x547802: jne 0x547836
0x547804: mov ebp, dword ptr [esi + 0x674]
0x54780a: mov ecx, dword ptr [edi + 0x18]
0x54780d: mov eax, dword ptr [ebp]
0x547810: push ebx
0x547811: push 0
0x547813: push ecx
0x547814: mov dword ptr [esp + 0x24], eax
0x547818: call 0x545710
0x54781d: add esp, 8
0x547820: push eax
0x547821: mov eax, dword ptr [esp + 0x20]
0x547825: push ebp
0x547826: call dword ptr [eax + 0x3c]
0x547829: mov edx, dword ptr [edi + 0x18]
0x54782c: mov dword ptr [esi + 0x54], edx
0x54782f: mov byte ptr [0x746132], 1
0x547836: fld dword ptr [edi + 0x1c]
0x547839: fsub dword ptr [esi + 0x60]
0x54783c: fabs 
0x54783e: fcomp qword ptr [0x672b18]
0x547844: fnstsw ax
0x547846: test ah, 5
0x547849: jp 0x547854
0x54784b: mov al, byte ptr [0x7252e0]
0x547850: test al, al
0x547852: jne 0x54787e
0x547854: mov ecx, dword ptr [0x721f24]
0x54785a: test ecx, ecx
0x54785c: mov eax, dword ptr [edi + 0x1c]
0x54785f: mov dword ptr [esi + 0x60], eax
0x547862: je 0x54787e
0x547864: mov edx, dword ptr [ecx]
0x547866: call dword ptr [edx + 0x10]
0x547869: test eax, eax
0x54786b: je 0x54787e
0x54786d: movsx eax, word ptr [esp + 0x14]
0x547872: mov ecx, dword ptr [0x721f24]
0x547878: mov edx, dword ptr [ecx]
0x54787a: push eax
0x54787b: call dword ptr [edx + 0x14]
0x54787e: pop ebp
0x54787f: pop ebx
0x547880: pop esi
0x547881: pop ecx
0x547882: ret 
#endif
