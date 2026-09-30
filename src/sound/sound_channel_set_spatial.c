// sound_channel_set_spatial  (Ghidra: FUN_005472d0)
// address 0x5472d0, size 728 bytes
// name confidence: 0.4   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Updates a 3D sound channel's positional/
//   orientation parameters and mute mode when they differ from the last committed values.";
//   types/sound.h sound_driver.channel_set_spatial's own signature comment "(int16_t
//   channel_index, uint8_t spatialized, sound_channel_spatial *spatial, float obstruction,
//   float occlusion, uint8_t underwater, int16_t sound_class)" matches this function's five
//   parameters (plus `spatialized`/`spatial` as the two register ones below) exactly;
//   directsound_channel.spatialized/buffer_3d/gain.../obstruction/occlusion/underwater fields and
//   the position/cone_orientation/velocity caches (0x0072543c../0x725448../0x725454..) match by
//   offset; IDirectSound3DBuffer vtable slots 0x48/0x4c/0x38/0x50 are SetMode/SetPosition/
//   SetConeOrientation/SetVelocity (Y negated on every Set*, per types/sound.h's own field
//   comments); sound_effect_object_vtable.channel_supported/apply_channel (0x10/0x14) match the
//   established struct.
// register convention: channel index/obstruction/occlusion/underwater/sound_class are this
//   function's own recognized stack parameters; spatialized in BL (unaff_BL), the
//   sound_channel_spatial* in EDI (unaff_EDI).
// blam-cc: stack -> (channel_index, obstruction, occlusion, underwater, sound_class), BL ->
//   spatialized, EDI -> spatial
// Phase-4 review: checked line by line against the disassembly appended below; it matches
//   (thresholds 0.05 position / cone, 0.01 velocity, 0.001 obstruction / occlusion; SetMode 1 plus
//   a zero position for scripted dialog classes 0x2c..0x2f on an unspatialized channel while
//   0x00722b5c is set).

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

static float sound_channel_set_spatial_fabsf(float x) { return (x < 0.0f) ? -x : x; }

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430
extern uint8_t directsound_initialized; // 0x007252e0
extern uint32_t config_head_relative_speech; // 0x00722b5c, UNSURE, see types/sound.h "referenced, owned elsewhere"
extern uint8_t directsound_deferred_dirty; // 0x00746132
extern sound_effect_object *global_sound_effect_object; // 0x00721f24

// blam-cc: stack -> (channel_index, obstruction, occlusion, underwater, sound_class), BL ->
// spatialized, EDI -> spatial
// Updates one directsound_channel's 3D mode, position, cone orientation, velocity (all cached and
// only re-committed to the buffer when they change by more than a small epsilon, or the mode
// itself just changed, or this is the first call since initialization), and obstruction/
// occlusion/underwater state, then reapplies the EAX per-channel effect when supported.
void sound_channel_set_spatial(int16_t channel_index, uint8_t spatialized, sound_channel_spatial *spatial,
    float obstruction, float occlusion, uint8_t underwater, int16_t sound_class)
{
    directsound_channel *channel = &directsound_channels[channel_index];
    void **vtable = *(void ***)channel->buffer_3d;
    int32_t (__stdcall *set_mode)(void *, uint32_t, uint32_t) = (int32_t (__stdcall *)(void *, uint32_t, uint32_t))vtable[0x48 / 4];
    int32_t (__stdcall *set_position)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[0x4c / 4];
    int32_t (__stdcall *set_cone_orientation)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[0x38 / 4];
    int32_t (__stdcall *set_velocity)(void *, float, float, float, uint32_t) =
        (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[0x50 / 4];
    uint8_t mode_changed = 0;
    uint8_t dialog_class = (sound_class >= 0x2c && sound_class <= 0x2f);

    if (channel->spatialized != spatialized || directsound_initialized == 0) {
        if (config_head_relative_speech == 0 || spatialized != 0 || !dialog_class) {
            set_mode(channel->buffer_3d, (spatialized != 0) ? 0u : 2u, 1); // DS3DMODE_NORMAL/DISABLE
        } else {
            set_mode(channel->buffer_3d, 1, 1); // DS3DMODE_HEADRELATIVE
            set_position(channel->buffer_3d, 0.0f, 0.0f, 0.0f, 1);
        }
        channel->spatialized = spatialized;
        mode_changed = 1;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->position.x - channel->position.x) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->position.y - channel->position.y) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->position.z - channel->position.z) >= 0.05f)) ||
        directsound_initialized == 0) {
        if (config_head_relative_speech == 0 || spatialized != 0 || !dialog_class) {
            set_position(channel->buffer_3d, spatial->position.x, -spatial->position.y, spatial->position.z, 1);
        } else {
            set_position(channel->buffer_3d, 0.0f, 0.0f, 0.0f, 1);
        }
        channel->position = spatial->position;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->forward.i - channel->cone_orientation.i) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->forward.j - channel->cone_orientation.j) >= 0.05f ||
          sound_channel_set_spatial_fabsf(spatial->forward.k - channel->cone_orientation.k) >= 0.05f)) ||
        directsound_initialized == 0) {
        set_cone_orientation(channel->buffer_3d, spatial->forward.i, -spatial->forward.j,
            spatial->forward.k, 1);
        channel->cone_orientation = spatial->forward;
        directsound_deferred_dirty = 1;
    }

    if ((channel->spatialized != 0 &&
         (sound_channel_set_spatial_fabsf(spatial->velocity.i - channel->velocity.i) >= 0.01f ||
          sound_channel_set_spatial_fabsf(spatial->velocity.j - channel->velocity.j) >= 0.01f ||
          sound_channel_set_spatial_fabsf(spatial->velocity.k - channel->velocity.k) >= 0.01f)) ||
        directsound_initialized == 0) {
        set_velocity(channel->buffer_3d, spatial->velocity.i, -spatial->velocity.j, spatial->velocity.k, 1);
        channel->velocity = spatial->velocity;
        directsound_deferred_dirty = 1;
    }

    if (sound_channel_set_spatial_fabsf(obstruction - channel->obstruction) >= 0.001f ||
        sound_channel_set_spatial_fabsf(occlusion - channel->occlusion) >= 0.001f ||
        channel->underwater != underwater || mode_changed || directsound_initialized == 0) {
        channel->obstruction = obstruction;
        channel->occlusion = occlusion;
        channel->underwater = underwater;

        if (global_sound_effect_object != 0) {
            sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;

            if (fx_vtable->channel_supported(global_sound_effect_object) != 0) {
                fx_vtable->apply_channel(global_sound_effect_object, channel_index);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5472d0):

void FUN_005472d0(short param_1,float param_2,float param_3,char param_4,int param_5)

{
  int *piVar1;
  char unaff_BL;
  int iVar2;
  int iVar3;
  float *unaff_EDI;
  bool bVar4;

  iVar2 = (int)param_1;
  iVar3 = iVar2 * 0x678;
  bVar4 = false;
  if (((&DAT_00725436)[iVar3] != unaff_BL) || (DAT_007252e0 == '\0')) {
    if ((DAT_00722b5c == 0) || (((unaff_BL != '\0' || (param_5 < 0x2c)) || (0x2f < param_5)))) {
      (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x48))
                ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],(unaff_BL != '\0') - 1U & 2,1);
    }
    else {
      (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x48))
                ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],1,1);
      (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x4c))
                ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],0,0,0,1);
    }
    (&DAT_00725436)[iVar3] = unaff_BL;
    bVar4 = true;
    DAT_00746132 = 1;
  }
  if (((((0.05 <= ABS(*unaff_EDI - *(float *)(&DAT_0072543c + iVar3))) ||
        (0.05 <= ABS(unaff_EDI[1] - *(float *)(&DAT_00725440 + iVar3)))) ||
       (0.05 <= ABS(unaff_EDI[2] - *(float *)(&DAT_00725444 + iVar3)))) &&
      ((&DAT_00725436)[iVar3] != '\0')) || (DAT_007252e0 == '\0')) {
    if (((DAT_00722b5c == 0) || (unaff_BL != '\0')) || ((param_5 < 0x2c || (0x2f < param_5)))) {
      (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x4c))
                ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],*unaff_EDI,-unaff_EDI[1],unaff_EDI[2],1);
    }
    else {
      (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x4c))
                ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],0,0,0,1);
    }
    *(float *)(&DAT_0072543c + iVar3) = *unaff_EDI;
    *(float *)(&DAT_00725440 + iVar3) = unaff_EDI[1];
    *(float *)(&DAT_00725444 + iVar3) = unaff_EDI[2];
    DAT_00746132 = 1;
  }
  if (((((0.05 <= ABS(unaff_EDI[3] - *(float *)(&DAT_00725448 + iVar3))) ||
        (0.05 <= ABS(unaff_EDI[4] - *(float *)(&DAT_0072544c + iVar3)))) ||
       (0.05 <= ABS(unaff_EDI[5] - *(float *)(&DAT_00725450 + iVar3)))) &&
      ((&DAT_00725436)[iVar3] != '\0')) || (DAT_007252e0 == '\0')) {
    (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x38))
              ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],unaff_EDI[3],-unaff_EDI[4],unaff_EDI[5],1);
    *(float *)(&DAT_00725448 + iVar3) = unaff_EDI[3];
    *(float *)(&DAT_0072544c + iVar3) = unaff_EDI[4];
    *(float *)(&DAT_00725450 + iVar3) = unaff_EDI[5];
    DAT_00746132 = 1;
  }
  if (((((0.01 <= ABS(unaff_EDI[6] - *(float *)(&DAT_00725454 + iVar3))) ||
        (0.01 <= ABS(unaff_EDI[7] - *(float *)(&DAT_00725458 + iVar3)))) ||
       (0.01 <= ABS(unaff_EDI[8] - *(float *)(&DAT_0072545c + iVar3)))) &&
      ((&DAT_00725436)[iVar3] != '\0')) || (DAT_007252e0 == '\0')) {
    (**(code **)(*(int *)(&DAT_00725aa4)[iVar2 * 0x19e] + 0x50))
              ((int *)(&DAT_00725aa4)[iVar2 * 0x19e],unaff_EDI[6],-unaff_EDI[7],unaff_EDI[8],1);
    *(float *)(&DAT_00725454 + iVar3) = unaff_EDI[6];
    *(float *)(&DAT_00725458 + iVar3) = unaff_EDI[7];
    *(float *)(&DAT_0072545c + iVar3) = unaff_EDI[8];
    DAT_00746132 = 1;
  }
  if (((0.001 <= ABS(param_2 - *(float *)(&DAT_00725474 + iVar3))) ||
      (0.001 <= ABS(param_3 - *(float *)(&DAT_00725478 + iVar3)))) ||
     (((&DAT_00725437)[iVar3] != param_4 || ((bVar4 || (DAT_007252e0 == '\0')))))) {
    *(float *)(&DAT_00725474 + iVar3) = param_2;
    piVar1 = DAT_00721f24;
    bVar4 = DAT_00721f24 != (int *)0x0;
    *(float *)(&DAT_00725478 + iVar3) = param_3;
    (&DAT_00725437)[iVar3] = param_4;
    if (bVar4) {
      iVar2 = (**(code **)(*piVar1 + 0x10))();
      if (iVar2 != 0) {
        (**(code **)(*DAT_00721f24 + 0x14))((int)param_1);
      }
    }
  }
  return;
}

Disassembly (0x5472d0..0x5475a8, capstone; phase-4 review):

0x5472d0: push ecx
0x5472d1: push ebp
0x5472d2: mov ebp, dword ptr [esp + 0x1c]
0x5472d6: push esi
0x5472d7: movsx esi, word ptr [esp + 0x10]
0x5472dc: imul esi, esi, 0x678
0x5472e2: add esi, 0x725430
0x5472e8: cmp byte ptr [esi + 6], bl
0x5472eb: mov byte ptr [esp + 0xb], 0
0x5472f0: jne 0x5472fb
0x5472f2: mov al, byte ptr [0x7252e0]
0x5472f7: test al, al
0x5472f9: jne 0x547361
0x5472fb: mov eax, dword ptr [0x722b5c]
0x547300: test eax, eax
0x547302: je 0x547338
0x547304: test bl, bl
0x547306: jne 0x547338
0x547308: cmp ebp, 0x2c
0x54730b: jl 0x547338
0x54730d: cmp ebp, 0x2f
0x547310: jg 0x547338
0x547312: mov eax, dword ptr [esi + 0x674]
0x547318: mov ecx, dword ptr [eax]
0x54731a: push 1
0x54731c: push 1
0x54731e: push eax
0x54731f: call dword ptr [ecx + 0x48]
0x547322: mov eax, dword ptr [esi + 0x674]
0x547328: mov edx, dword ptr [eax]
0x54732a: push 1
0x54732c: push 0
0x54732e: push 0
0x547330: push 0
0x547332: push eax
0x547333: call dword ptr [edx + 0x4c]
0x547336: jmp 0x547352
0x547338: mov eax, dword ptr [esi + 0x674]
0x54733e: mov ecx, dword ptr [eax]
0x547340: xor edx, edx
0x547342: test bl, bl
0x547344: setne dl
0x547347: push 1
0x547349: dec edx
0x54734a: and edx, 2
0x54734d: push edx
0x54734e: push eax
0x54734f: call dword ptr [ecx + 0x48]
0x547352: mov byte ptr [esi + 6], bl
0x547355: mov byte ptr [esp + 0xb], 1
0x54735a: mov byte ptr [0x746132], 1
0x547361: fld dword ptr [edi]
0x547363: fsub dword ptr [esi + 0xc]
0x547366: fabs 
0x547368: fcomp qword ptr [0x672b28]
0x54736e: fnstsw ax
0x547370: test ah, 5
0x547373: jp 0x54739f
0x547375: fld dword ptr [edi + 4]
0x547378: fsub dword ptr [esi + 0x10]
0x54737b: fabs 
0x54737d: fcomp qword ptr [0x672b28]
0x547383: fnstsw ax
0x547385: test ah, 5
0x547388: jp 0x54739f
0x54738a: fld dword ptr [edi + 8]
0x54738d: fsub dword ptr [esi + 0x14]
0x547390: fabs 
0x547392: fcomp qword ptr [0x672b28]
0x547398: fnstsw ax
0x54739a: test ah, 5
0x54739d: jnp 0x5473a6
0x54739f: mov al, byte ptr [esi + 6]
0x5473a2: test al, al
0x5473a4: jne 0x5473af
0x5473a6: mov al, byte ptr [0x7252e0]
0x5473ab: test al, al
0x5473ad: jne 0x547414
0x5473af: mov eax, dword ptr [0x722b5c]
0x5473b4: test eax, eax
0x5473b6: je 0x5473dc
0x5473b8: test bl, bl
0x5473ba: jne 0x5473dc
0x5473bc: cmp ebp, 0x2c
0x5473bf: jl 0x5473dc
0x5473c1: cmp ebp, 0x2f
0x5473c4: jg 0x5473dc
0x5473c6: mov eax, dword ptr [esi + 0x674]
0x5473cc: mov ecx, dword ptr [eax]
0x5473ce: push 1
0x5473d0: push 0
0x5473d2: push 0
0x5473d4: push 0
0x5473d6: push eax
0x5473d7: call dword ptr [ecx + 0x4c]
0x5473da: jmp 0x5473fa
0x5473dc: mov ecx, dword ptr [edi + 8]
0x5473df: fld dword ptr [edi + 4]
0x5473e2: mov eax, dword ptr [esi + 0x674]
0x5473e8: fchs 
0x5473ea: mov edx, dword ptr [eax]
0x5473ec: push 1
0x5473ee: push ecx
0x5473ef: push ecx
0x5473f0: mov ecx, dword ptr [edi]
0x5473f2: fstp dword ptr [esp]
0x5473f5: push ecx
0x5473f6: push eax
0x5473f7: call dword ptr [edx + 0x4c]
0x5473fa: mov edx, edi
0x5473fc: mov eax, dword ptr [edx]
0x5473fe: mov dword ptr [esi + 0xc], eax
0x547401: mov ecx, dword ptr [edx + 4]
0x547404: mov dword ptr [esi + 0x10], ecx
0x547407: mov edx, dword ptr [edx + 8]
0x54740a: mov dword ptr [esi + 0x14], edx
0x54740d: mov byte ptr [0x746132], 1
0x547414: fld dword ptr [edi + 0xc]
0x547417: fsub dword ptr [esi + 0x18]
0x54741a: fabs 
0x54741c: fcomp qword ptr [0x672b28]
0x547422: fnstsw ax
0x547424: test ah, 5
0x547427: jp 0x547453
0x547429: fld dword ptr [edi + 0x10]
0x54742c: fsub dword ptr [esi + 0x1c]
0x54742f: fabs 
0x547431: fcomp qword ptr [0x672b28]
0x547437: fnstsw ax
0x547439: test ah, 5
0x54743c: jp 0x547453
0x54743e: fld dword ptr [edi + 0x14]
0x547441: fsub dword ptr [esi + 0x20]
0x547444: fabs 
0x547446: fcomp qword ptr [0x672b28]
0x54744c: fnstsw ax
0x54744e: test ah, 5
0x547451: jnp 0x54745a
0x547453: mov al, byte ptr [esi + 6]
0x547456: test al, al
0x547458: jne 0x547463
0x54745a: mov al, byte ptr [0x7252e0]
0x54745f: test al, al
0x547461: jne 0x54749b
0x547463: mov eax, dword ptr [esi + 0x674]
0x547469: fld dword ptr [edi + 0x10]
0x54746c: mov edx, dword ptr [edi + 0x14]
0x54746f: fchs 
0x547471: mov ecx, dword ptr [eax]
0x547473: push 1
0x547475: push edx
0x547476: mov edx, dword ptr [edi + 0xc]
0x547479: push ecx
0x54747a: fstp dword ptr [esp]
0x54747d: push edx
0x54747e: push eax
0x54747f: call dword ptr [ecx + 0x38]
0x547482: mov eax, dword ptr [edi + 0xc]
0x547485: mov dword ptr [esi + 0x18], eax
0x547488: mov ecx, dword ptr [edi + 0x10]
0x54748b: mov dword ptr [esi + 0x1c], ecx
0x54748e: mov edx, dword ptr [edi + 0x14]
0x547491: mov dword ptr [esi + 0x20], edx
0x547494: mov byte ptr [0x746132], 1
0x54749b: fld dword ptr [edi + 0x18]
0x54749e: lea ebp, [edi + 0x18]
0x5474a1: fsub dword ptr [esi + 0x24]
0x5474a4: fabs 
0x5474a6: fcomp qword ptr [0x672b20]
0x5474ac: fnstsw ax
0x5474ae: test ah, 5
0x5474b1: jp 0x5474dd
0x5474b3: fld dword ptr [edi + 0x1c]
0x5474b6: fsub dword ptr [esi + 0x28]
0x5474b9: fabs 
0x5474bb: fcomp qword ptr [0x672b20]
0x5474c1: fnstsw ax
0x5474c3: test ah, 5
0x5474c6: jp 0x5474dd
0x5474c8: fld dword ptr [edi + 0x20]
0x5474cb: fsub dword ptr [esi + 0x2c]
0x5474ce: fabs 
0x5474d0: fcomp qword ptr [0x672b20]
0x5474d6: fnstsw ax
0x5474d8: test ah, 5
0x5474db: jnp 0x5474e4
0x5474dd: mov al, byte ptr [esi + 6]
0x5474e0: test al, al
0x5474e2: jne 0x5474ed
0x5474e4: mov al, byte ptr [0x7252e0]
0x5474e9: test al, al
0x5474eb: jne 0x547525
0x5474ed: mov eax, dword ptr [esi + 0x674]
0x5474f3: fld dword ptr [edi + 0x1c]
0x5474f6: mov edx, dword ptr [edi + 0x20]
0x5474f9: fchs 
0x5474fb: mov ecx, dword ptr [eax]
0x5474fd: push 1
0x5474ff: push edx
0x547500: mov edx, dword ptr [ebp]
0x547503: push ecx
0x547504: fstp dword ptr [esp]
0x547507: push edx
0x547508: push eax
0x547509: call dword ptr [ecx + 0x50]
0x54750c: mov eax, dword ptr [ebp]
0x54750f: mov dword ptr [esi + 0x24], eax
0x547512: mov ecx, dword ptr [ebp + 4]
0x547515: mov dword ptr [esi + 0x28], ecx
0x547518: mov edx, dword ptr [ebp + 8]
0x54751b: mov dword ptr [esi + 0x2c], edx
0x54751e: mov byte ptr [0x746132], 1
0x547525: fld dword ptr [esp + 0x14]
0x547529: fsub dword ptr [esi + 0x44]
0x54752c: fabs 
0x54752e: fcomp qword ptr [0x672b18]
0x547534: fnstsw ax
0x547536: test ah, 5
0x547539: jp 0x54756b
0x54753b: fld dword ptr [esp + 0x18]
0x54753f: fsub dword ptr [esi + 0x48]
0x547542: fabs 
0x547544: fcomp qword ptr [0x672b18]
0x54754a: fnstsw ax
0x54754c: test ah, 5
0x54754f: jp 0x54756b
0x547551: mov al, byte ptr [esi + 7]
0x547554: cmp al, byte ptr [esp + 0x1c]
0x547558: jne 0x54756b
0x54755a: mov al, byte ptr [esp + 0xb]
0x54755e: test al, al
0x547560: jne 0x54756b
0x547562: mov al, byte ptr [0x7252e0]
0x547567: test al, al
0x547569: jne 0x5475a4
0x54756b: mov ecx, dword ptr [esp + 0x14]
0x54756f: mov edx, dword ptr [esp + 0x18]
0x547573: mov al, byte ptr [esp + 0x1c]
0x547577: mov dword ptr [esi + 0x44], ecx
0x54757a: mov ecx, dword ptr [0x721f24]
0x547580: test ecx, ecx
0x547582: mov dword ptr [esi + 0x48], edx
0x547585: mov byte ptr [esi + 7], al
0x547588: je 0x5475a4
0x54758a: mov edx, dword ptr [ecx]
0x54758c: call dword ptr [edx + 0x10]
0x54758f: test eax, eax
0x547591: je 0x5475a4
0x547593: movsx eax, word ptr [esp + 0x10]
0x547598: mov ecx, dword ptr [0x721f24]
0x54759e: mov edx, dword ptr [ecx]
0x5475a0: push eax
0x5475a1: call dword ptr [edx + 0x14]
0x5475a4: pop esi
0x5475a5: pop ebp
0x5475a6: pop ecx
0x5475a7: ret 
#endif
