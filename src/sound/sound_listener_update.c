// sound_listener_update  (Ghidra: sound_listener_update, already named)
// address 0x547070, size 595 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Updates the 3D audio listener's position,
//   orientation, velocity, and EAX environment settings only when they have changed since the
//   last call."; types/sound.h sound_listener_parameters (position/forward/up/velocity/
//   environment at 0x00/0x0c/0x18/0x24/0x30) and its own header note "vtable SetPosition 0x38 /
//   SetOrientation 0x34 / SetVelocity 0x40, environment compared as 0x12 dwords" match exactly;
//   directsound_listener_cache (0x00746030) and directsound_environment_cache (0x00746064) cache
//   the last-committed values; sound_effect_object_vtable.listener_supported/apply_listener
//   (slots 0xc/0x1c) match the established struct.
// register convention: plain __cdecl, listener parameters pointer as the recognized stack
//   parameter (param_1).
// blam-cc: stack -> parameters
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "sound.h"
#include "fn_sound.h"

static float sound_listener_update_fabsf(float x) { return (x < 0.0f) ? -x : x; }

extern void *directsound_listener; // 0x00746114
extern uint8_t directsound_initialized; // 0x007252e0
extern directsound_listener_cache directsound_listener_cached; // 0x00746030
extern SoundEnvironment directsound_environment_cache;     // 0x00746064
extern sound_effect_object *global_sound_effect_object;    // 0x00721f24

// blam-cc: stack -> parameters
// Pushes each part of `parameters` (position, orientation, velocity, EAX environment) to the
// DirectSound 3D listener and, when supported, the active EAX effects object, but only when it
// has actually changed (by more than 0.05 for position/orientation, 0.01 for velocity, or any
// difference for the environment) since the last call, or on the very first call after
// (re)initialization.
void sound_listener_update(sound_listener_parameters *parameters)
{
    void **vtable = *(void ***)directsound_listener;

    if (sound_listener_update_fabsf(parameters->position.x - directsound_listener_cached.position.x) >= 0.05f ||
        sound_listener_update_fabsf(parameters->position.y - directsound_listener_cached.position.y) >= 0.05f ||
        sound_listener_update_fabsf(parameters->position.z - directsound_listener_cached.position.z) >= 0.05f ||
        directsound_initialized == 0) {
        int32_t (__stdcall *set_position)(void *, float, float, float, uint32_t) =
            (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[0x38 / 4];
        set_position(directsound_listener, parameters->position.x, parameters->position.y,
            parameters->position.z, 0);
        directsound_listener_cached.position = parameters->position;
    }

    {
        float *orientation = (float *)&parameters->forward; // forward[3] + up[3], 6 floats
        uint8_t changed = directsound_initialized == 0;
        int32_t i;

        for (i = 0; i < 6; i++) {
            if (sound_listener_update_fabsf(orientation[i] - ((float *)&directsound_listener_cached.forward)[i]) >= 0.05f) {
                changed = 1;
            }
        }

        if (changed) {
            int32_t (__stdcall *set_orientation)(void *, float, float, float, float, float, float, uint32_t) =
                (int32_t (__stdcall *)(void *, float, float, float, float, float, float, uint32_t))vtable[0x34 / 4];
            set_orientation(directsound_listener, orientation[0], orientation[1], orientation[2],
                orientation[3], orientation[4], orientation[5], 0);
            for (i = 0; i < 6; i++) {
                ((float *)&directsound_listener_cached.forward)[i] = orientation[i];
            }
        }
    }

    if (sound_listener_update_fabsf(parameters->velocity.i - directsound_listener_cached.velocity.i) >= 0.01f ||
        sound_listener_update_fabsf(parameters->velocity.j - directsound_listener_cached.velocity.j) >= 0.01f ||
        sound_listener_update_fabsf(parameters->velocity.k - directsound_listener_cached.velocity.k) >= 0.01f ||
        directsound_initialized == 0) {
        int32_t (__stdcall *set_velocity)(void *, float, float, float, uint32_t) =
            (int32_t (__stdcall *)(void *, float, float, float, uint32_t))vtable[0x40 / 4];
        set_velocity(directsound_listener, parameters->velocity.i, parameters->velocity.j,
            parameters->velocity.k, 0);
        directsound_listener_cached.velocity = parameters->velocity;
    }

    {
        uint32_t *environment_words = (uint32_t *)parameters->environment;
        uint32_t *cache_words = (uint32_t *)&directsound_environment_cache;
        uint8_t same = 1;
        int32_t i;

        for (i = 0; i < 0x12; i++) {
            if (environment_words[i] != cache_words[i]) {
                same = 0;
                break;
            }
        }

        if (!same || directsound_initialized == 0) {
            directsound_environment_cache = *parameters->environment;

            if (global_sound_effect_object != 0) {
                sound_effect_object_vtable *fx_vtable = global_sound_effect_object->vtable;

                if (fx_vtable->listener_supported(global_sound_effect_object) != 0) {
                    fx_vtable->apply_listener(global_sound_effect_object, parameters->environment);
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x547070):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void sound_listener_update(float *param_1)

{
  int *piVar1;
  int iVar2;
  int *piVar3;
  int *piVar4;
  bool bVar5;

  if ((((0.05 <= ABS(*param_1 - _DAT_00746030)) || (0.05 <= ABS(param_1[1] - _DAT_00746034))) ||
      (0.05 <= ABS(param_1[2] - _DAT_00746038))) || (DAT_007252e0 == '\0')) {
    (**(code **)(*DAT_00746114 + 0x38))(DAT_00746114,*param_1,param_1[1],param_1[2],0);
    _DAT_00746030 = *param_1;
    _DAT_00746034 = param_1[1];
    _DAT_00746038 = param_1[2];
  }
  if ((((0.05 <= ABS(param_1[3] - _DAT_0074603c)) || (0.05 <= ABS(param_1[4] - _DAT_00746040))) ||
      ((0.05 <= ABS(param_1[5] - _DAT_00746044) ||
       ((0.05 <= ABS(param_1[6] - _DAT_00746048) || (0.05 <= ABS(param_1[7] - _DAT_0074604c)))))))
     || ((0.05 <= ABS(param_1[8] - _DAT_00746050) || (DAT_007252e0 == '\0')))) {
    (**(code **)(*DAT_00746114 + 0x34))
              (DAT_00746114,param_1[3],param_1[4],param_1[5],param_1[6],param_1[7],param_1[8],0);
    _DAT_0074603c = param_1[3];
    _DAT_00746040 = param_1[4];
    _DAT_00746044 = param_1[5];
    _DAT_00746048 = param_1[6];
    _DAT_0074604c = param_1[7];
    _DAT_00746050 = param_1[8];
  }
  if ((((0.01 <= ABS(param_1[9] - _DAT_00746054)) || (0.01 <= ABS(param_1[10] - _DAT_00746058))) ||
      (0.01 <= ABS(param_1[0xb] - _DAT_0074605c))) || (DAT_007252e0 == '\0')) {
    (**(code **)(*DAT_00746114 + 0x40))(DAT_00746114,param_1[9],param_1[10],param_1[0xb],0);
    _DAT_00746054 = param_1[9];
    _DAT_00746058 = param_1[10];
    _DAT_0074605c = param_1[0xb];
  }
  piVar1 = (int *)param_1[0xc];
  iVar2 = 0x12;
  bVar5 = true;
  piVar3 = piVar1;
  piVar4 = &DAT_00746064;
  do {
    if (iVar2 == 0) break;
    iVar2 = iVar2 + -1;
    bVar5 = *piVar3 == *piVar4;
    piVar3 = piVar3 + 1;
    piVar4 = piVar4 + 1;
  } while (bVar5);
  if ((!bVar5) || (DAT_007252e0 == '\0')) {
    piVar3 = piVar1;
    piVar4 = &DAT_00746064;
    for (iVar2 = 0x12; iVar2 != 0; iVar2 = iVar2 + -1) {
      *piVar4 = *piVar3;
      piVar3 = piVar3 + 1;
      piVar4 = piVar4 + 1;
    }
    if (DAT_00721f24 != (int *)0x0) {
      iVar2 = (**(code **)(*DAT_00721f24 + 0xc))();
      if (iVar2 != 0) {
        (**(code **)(*DAT_00721f24 + 0x1c))(piVar1);
      }
    }
  }
  return;
}

Disassembly (0x547070..0x5472c3, capstone; phase-4 review):

0x547070: push ebx
0x547071: push esi
0x547072: mov esi, dword ptr [esp + 0xc]
0x547076: fld dword ptr [esi]
0x547078: push edi
0x547079: fsub dword ptr [0x746030]
0x54707f: fabs 
0x547081: fcomp qword ptr [0x672b28]
0x547087: fnstsw ax
0x547089: test ah, 5
0x54708c: jp 0x5470c7
0x54708e: fld dword ptr [esi + 4]
0x547091: fsub dword ptr [0x746034]
0x547097: fabs 
0x547099: fcomp qword ptr [0x672b28]
0x54709f: fnstsw ax
0x5470a1: test ah, 5
0x5470a4: jp 0x5470c7
0x5470a6: fld dword ptr [esi + 8]
0x5470a9: fsub dword ptr [0x746038]
0x5470af: fabs 
0x5470b1: fcomp qword ptr [0x672b28]
0x5470b7: fnstsw ax
0x5470b9: test ah, 5
0x5470bc: jp 0x5470c7
0x5470be: mov al, byte ptr [0x7252e0]
0x5470c3: test al, al
0x5470c5: jne 0x5470fa
0x5470c7: mov edx, dword ptr [esi + 8]
0x5470ca: mov eax, dword ptr [0x746114]
0x5470cf: mov ecx, dword ptr [eax]
0x5470d1: push 0
0x5470d3: push edx
0x5470d4: mov edx, dword ptr [esi + 4]
0x5470d7: push edx
0x5470d8: mov edx, dword ptr [esi]
0x5470da: push edx
0x5470db: push eax
0x5470dc: call dword ptr [ecx + 0x38]
0x5470df: mov eax, esi
0x5470e1: mov ecx, dword ptr [eax]
0x5470e3: mov dword ptr [0x746030], ecx
0x5470e9: mov edx, dword ptr [eax + 4]
0x5470ec: mov dword ptr [0x746034], edx
0x5470f2: mov eax, dword ptr [eax + 8]
0x5470f5: mov dword ptr [0x746038], eax
0x5470fa: fld dword ptr [esi + 0xc]
0x5470fd: fsub dword ptr [0x74603c]
0x547103: fabs 
0x547105: fcomp qword ptr [0x672b28]
0x54710b: fnstsw ax
0x54710d: test ah, 5
0x547110: jp 0x547197
0x547116: fld dword ptr [esi + 0x10]
0x547119: fsub dword ptr [0x746040]
0x54711f: fabs 
0x547121: fcomp qword ptr [0x672b28]
0x547127: fnstsw ax
0x547129: test ah, 5
0x54712c: jp 0x547197
0x54712e: fld dword ptr [esi + 0x14]
0x547131: fsub dword ptr [0x746044]
0x547137: fabs 
0x547139: fcomp qword ptr [0x672b28]
0x54713f: fnstsw ax
0x547141: test ah, 5
0x547144: jp 0x547197
0x547146: fld dword ptr [esi + 0x18]
0x547149: fsub dword ptr [0x746048]
0x54714f: fabs 
0x547151: fcomp qword ptr [0x672b28]
0x547157: fnstsw ax
0x547159: test ah, 5
0x54715c: jp 0x547197
0x54715e: fld dword ptr [esi + 0x1c]
0x547161: fsub dword ptr [0x74604c]
0x547167: fabs 
0x547169: fcomp qword ptr [0x672b28]
0x54716f: fnstsw ax
0x547171: test ah, 5
0x547174: jp 0x547197
0x547176: fld dword ptr [esi + 0x20]
0x547179: fsub dword ptr [0x746050]
0x54717f: fabs 
0x547181: fcomp qword ptr [0x672b28]
0x547187: fnstsw ax
0x547189: test ah, 5
0x54718c: jp 0x547197
0x54718e: mov al, byte ptr [0x7252e0]
0x547193: test al, al
0x547195: jne 0x5471f0
0x547197: mov edx, dword ptr [esi + 0x20]
0x54719a: mov eax, dword ptr [0x746114]
0x54719f: mov ecx, dword ptr [eax]
0x5471a1: push 0
0x5471a3: push edx
0x5471a4: mov edx, dword ptr [esi + 0x1c]
0x5471a7: push edx
0x5471a8: mov edx, dword ptr [esi + 0x18]
0x5471ab: push edx
0x5471ac: mov edx, dword ptr [esi + 0x14]
0x5471af: push edx
0x5471b0: mov edx, dword ptr [esi + 0x10]
0x5471b3: push edx
0x5471b4: mov edx, dword ptr [esi + 0xc]
0x5471b7: push edx
0x5471b8: push eax
0x5471b9: call dword ptr [ecx + 0x34]
0x5471bc: mov eax, dword ptr [esi + 0xc]
0x5471bf: mov dword ptr [0x74603c], eax
0x5471c4: mov ecx, dword ptr [esi + 0x10]
0x5471c7: mov dword ptr [0x746040], ecx
0x5471cd: mov edx, dword ptr [esi + 0x14]
0x5471d0: mov dword ptr [0x746044], edx
0x5471d6: mov eax, dword ptr [esi + 0x18]
0x5471d9: mov dword ptr [0x746048], eax
0x5471de: mov ecx, dword ptr [esi + 0x1c]
0x5471e1: mov dword ptr [0x74604c], ecx
0x5471e7: mov edx, dword ptr [esi + 0x20]
0x5471ea: mov dword ptr [0x746050], edx
0x5471f0: fld dword ptr [esi + 0x24]
0x5471f3: fsub dword ptr [0x746054]
0x5471f9: fabs 
0x5471fb: fcomp qword ptr [0x672b20]
0x547201: fnstsw ax
0x547203: test ah, 5
0x547206: jp 0x547241
0x547208: fld dword ptr [esi + 0x28]
0x54720b: fsub dword ptr [0x746058]
0x547211: fabs 
0x547213: fcomp qword ptr [0x672b20]
0x547219: fnstsw ax
0x54721b: test ah, 5
0x54721e: jp 0x547241
0x547220: fld dword ptr [esi + 0x2c]
0x547223: fsub dword ptr [0x74605c]
0x547229: fabs 
0x54722b: fcomp qword ptr [0x672b20]
0x547231: fnstsw ax
0x547233: test ah, 5
0x547236: jp 0x547241
0x547238: mov al, byte ptr [0x7252e0]
0x54723d: test al, al
0x54723f: jne 0x547274
0x547241: mov edx, dword ptr [esi + 0x2c]
0x547244: mov eax, dword ptr [0x746114]
0x547249: mov ecx, dword ptr [eax]
0x54724b: push 0
0x54724d: push edx
0x54724e: mov edx, dword ptr [esi + 0x28]
0x547251: push edx
0x547252: mov edx, dword ptr [esi + 0x24]
0x547255: push edx
0x547256: push eax
0x547257: call dword ptr [ecx + 0x40]
0x54725a: mov eax, dword ptr [esi + 0x24]
0x54725d: mov dword ptr [0x746054], eax
0x547262: mov ecx, dword ptr [esi + 0x28]
0x547265: mov dword ptr [0x746058], ecx
0x54726b: mov edx, dword ptr [esi + 0x2c]
0x54726e: mov dword ptr [0x74605c], edx
0x547274: mov ebx, dword ptr [esi + 0x30]
0x547277: mov ecx, 0x12
0x54727c: mov edi, 0x746064
0x547281: mov esi, ebx
0x547283: xor eax, eax
0x547285: repe cmpsd dword ptr [esi], dword ptr es:[edi]
0x547287: jne 0x547292
0x547289: mov al, byte ptr [0x7252e0]
0x54728e: test al, al
0x547290: jne 0x5472bf
0x547292: mov ecx, 0x12
0x547297: mov esi, ebx
0x547299: mov edi, 0x746064
0x54729e: rep movsd dword ptr es:[edi], dword ptr [esi]
0x5472a0: mov ecx, dword ptr [0x721f24]
0x5472a6: test ecx, ecx
0x5472a8: je 0x5472bf
0x5472aa: mov edx, dword ptr [ecx]
0x5472ac: call dword ptr [edx + 0xc]
0x5472af: test eax, eax
0x5472b1: je 0x5472bf
0x5472b3: mov ecx, dword ptr [0x721f24]
0x5472b9: mov eax, dword ptr [ecx]
0x5472bb: push ebx
0x5472bc: call dword ptr [eax + 0x1c]
0x5472bf: pop edi
0x5472c0: pop esi
0x5472c1: pop ebx
0x5472c2: ret 
#endif
