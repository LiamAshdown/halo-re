// sound_effects_object_detect_mode  (Ghidra: sound_effects_object_detect_mode, already named,
// __cdecl)
// address 0x551270, size 406 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Probes for EAX3, EAX2, then EAX1 sound-effects
// backend support in order, selects the first available implementation, and formats a 'Sound
// effect mode set to %s' status string."; out/phase4/sound_types_notes.md attributes the
// 0xe8/0x1c operator_new sizes here to sound_eax_effect_object/sound_effect_object
// (types/sound.h). directsound_eax_enabled/available (0x00746121/0x00746120) and
// sound_effect_object_state (0x00746130) match the header's globals list.
// register convention: EAX -> channel_index, EDI -> channel (the directsound_channel being
//   created by sound_channel_create 0x546760, the only caller). Both are forwarded to each
//   candidate object's initialize(this = ECX, channel, channel_index) (vtable +4, __thiscall).
// Phase-4 review (disassembly): the draft took no arguments, passed (directsound_channels, 0) to
//   initialize and cleared `mode` (+4) after construction; the binary clears
//   supported_properties (+8).
// vtable->initialize's (channels, unknown) arguments are not visible at any of these three call
// sites, but sound_effects_object_reinitialize.c (0x5514d0) makes the identical three calls with
// explicit arguments (&directsound_channels[0], 0); used here on that evidence.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern uint8_t directsound_eax_enabled;   // 0x00746121
extern uint8_t directsound_eax_available; // 0x00746120
extern sound_effect_object *global_sound_effect_object; // 0x00721f24
extern int16_t sound_effect_object_state; // 0x00746130

extern sound_effect_object_vtable sound_eax3_vtable; // 0x00671d28
extern sound_effect_object_vtable sound_eax2_vtable; // 0x00671d04
extern sound_effect_object_vtable sound_eax1_vtable; // 0x00671d4c

extern void *operator_new(uint32_t size); // 0x6277da, MSVC CRT
extern int32_t sprintf(char *buffer, const char *format, ...); // 0x623693 _sprintf

// Probes EAX3, then EAX2, then EAX1 support (constructing and initializing a fresh sound effect
// object for each attempt, discarding it on failure) and keeps the first one that initializes
// successfully. Returns 2/1/0 for EAX3/EAX2/EAX1, or -1 if none are available (or EAX is
// disabled/unavailable), logging the chosen mode either way.
int32_t sound_effects_object_detect_mode(int16_t channel_index, directsound_channel *channel)
{
    char mode_name[32];
    char message[64];
    int32_t mode;
    const char *format;

    mode = -1;

    if (directsound_eax_enabled && directsound_eax_available) {
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }

        global_sound_effect_object = (sound_effect_object *)operator_new(0xe8);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax3_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 2;
                global_sound_effect_object->mode = 2;
                goto format_message;
            }
            if (global_sound_effect_object != 0) {
                global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
            }
        }

        global_sound_effect_object = (sound_effect_object *)operator_new(0xe8);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax2_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 1;
                global_sound_effect_object->mode = 1;
                goto format_message;
            }
            if (global_sound_effect_object != 0) {
                global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
            }
        }

        global_sound_effect_object = (sound_effect_object *)operator_new(0x1c);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable = &sound_eax1_vtable;
            global_sound_effect_object->supported_properties = 0;
            if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, channel, channel_index) != 0) {
                mode = 0;
                global_sound_effect_object->supported_properties = 0;
                goto format_message;
            }
        }
    }

    mode = -1;
    sound_effect_object_state = 2;

format_message:
    switch (mode) {
        case 0:  format = "SOUND_EFFECT_OBJECT_EAX1"; break;
        case 1:  format = "SOUND_EFFECT_OBJECT_EAX2"; break;
        case 2:  format = "SOUND_EFFECT_OBJECT_EAX3"; break;
        case 3:  format = "SOUND_EFFECT_OBJECT_DIRECTX"; break;
        case -1: format = "SOUND_EFFECT_OBJECT_NONE"; break;
        default:
            mode = -1;
            format = "SOUND_EFFECT_OBJECT_NONE";
            break;
    }
    sprintf(mode_name, format);
    sprintf(message, "Sound effect mode set to %s", mode_name);
    return mode;
}

#if 0
Original Ghidra decompilation (0x551270):

int __cdecl sound_effects_object_detect_mode(void)

{
  int iVar1;
  char *_Format;
  char acStack_60 [32];
  char acStack_40 [64];

  if ((DAT_00746121 != '\0') && (DAT_00746120 != '\0')) {
    if (DAT_00721f24 != (int *)0x0) {
      (**(code **)*DAT_00721f24)();
    }
    DAT_00721f24 = operator_new(0xe8);
    if (DAT_00721f24 == (int *)0x0) {
      DAT_00721f24 = (int *)0x0;
    }
    else {
      *DAT_00721f24 = (int)&PTR_sound_eax30_effect_shutdown_00671d28;
      DAT_00721f24[2] = 0;
      iVar1 = (**(code **)(*DAT_00721f24 + 4))();
      if (iVar1 != 0) {
        iVar1 = 2;
        DAT_00721f24[1] = 2;
        goto LAB_0055138c;
      }
      if (DAT_00721f24 != (int *)0x0) {
        (**(code **)*DAT_00721f24)();
      }
    }
    DAT_00721f24 = operator_new(0xe8);
    if (DAT_00721f24 == (int *)0x0) {
      DAT_00721f24 = (int *)0x0;
    }
    else {
      *DAT_00721f24 = (int)&PTR_sound_eax20_effect_shutdown_00671d04;
      DAT_00721f24[2] = 0;
      iVar1 = (**(code **)(*DAT_00721f24 + 4))();
      if (iVar1 != 0) {
        iVar1 = 1;
        DAT_00721f24[1] = 1;
        goto LAB_0055138c;
      }
      if (DAT_00721f24 != (int *)0x0) {
        (**(code **)*DAT_00721f24)();
      }
    }
    DAT_00721f24 = operator_new(0x1c);
    if (DAT_00721f24 == (int *)0x0) {
      DAT_00721f24 = (int *)0x0;
    }
    else {
      *DAT_00721f24 = (int)&PTR_FUN_00671d4c;
      DAT_00721f24[2] = 0;
      iVar1 = (**(code **)(*DAT_00721f24 + 4))();
      if (iVar1 != 0) {
        iVar1 = 0;
        DAT_00721f24[1] = 0;
        goto LAB_0055138c;
      }
    }
  }
  iVar1 = -1;
  DAT_00746130 = 2;
LAB_0055138c:
  switch(iVar1) {
  case 0:
    _Format = "SOUND_EFFECT_OBJECT_EAX1";
    break;
  case 1:
    _Format = "SOUND_EFFECT_OBJECT_EAX2";
    break;
  case 2:
    _Format = "SOUND_EFFECT_OBJECT_EAX3";
    break;
  case 3:
    _Format = "SOUND_EFFECT_OBJECT_DIRECTX";
    break;
  case -1:
    _Format = "SOUND_EFFECT_OBJECT_NONE";
    break;
  default:
    iVar1 = -1;
    _Format = "SOUND_EFFECT_OBJECT_NONE";
  }
  _sprintf(acStack_60,_Format);
  _sprintf(acStack_40,"Sound effect mode set to %s",acStack_60);
  return iVar1;
}

Disassembly (0x551270..0x551406, capstone; phase-4 review):

0x551270: sub esp, 0x60
0x551273: push ebx
0x551274: push esi
0x551275: mov esi, eax
0x551277: mov al, byte ptr [0x746121]
0x55127c: xor ebx, ebx
0x55127e: cmp al, bl
0x551280: je 0x551380
0x551286: cmp byte ptr [0x746120], bl
0x55128c: je 0x551380
0x551292: mov ecx, dword ptr [0x721f24]
0x551298: cmp ecx, ebx
0x55129a: je 0x5512a0
0x55129c: mov eax, dword ptr [ecx]
0x55129e: call dword ptr [eax]
0x5512a0: push 0xe8
0x5512a5: call 0x6277da
0x5512aa: add esp, 4
0x5512ad: cmp eax, ebx
0x5512af: je 0x5512de
0x5512b1: push esi
0x5512b2: mov dword ptr [eax], 0x671d28
0x5512b8: mov dword ptr [eax + 8], ebx
0x5512bb: mov edx, dword ptr [eax]
0x5512bd: push edi
0x5512be: mov ecx, eax
0x5512c0: mov dword ptr [0x721f24], eax
0x5512c5: call dword ptr [edx + 4]
0x5512c8: cmp eax, ebx
0x5512ca: je 0x5512e6
0x5512cc: mov eax, dword ptr [0x721f24]
0x5512d1: mov esi, 2
0x5512d6: mov dword ptr [eax + 4], esi
0x5512d9: jmp 0x55138c
0x5512de: mov dword ptr [0x721f24], ebx
0x5512e4: jmp 0x5512f4
0x5512e6: mov ecx, dword ptr [0x721f24]
0x5512ec: cmp ecx, ebx
0x5512ee: je 0x5512f4
0x5512f0: mov edx, dword ptr [ecx]
0x5512f2: call dword ptr [edx]
0x5512f4: push 0xe8
0x5512f9: call 0x6277da
0x5512fe: add esp, 4
0x551301: cmp eax, ebx
0x551303: je 0x55132f
0x551305: push esi
0x551306: mov dword ptr [eax], 0x671d04
0x55130c: mov dword ptr [eax + 8], ebx
0x55130f: mov edx, dword ptr [eax]
0x551311: push edi
0x551312: mov ecx, eax
0x551314: mov dword ptr [0x721f24], eax
0x551319: call dword ptr [edx + 4]
0x55131c: cmp eax, ebx
0x55131e: je 0x551337
0x551320: mov eax, dword ptr [0x721f24]
0x551325: mov esi, 1
0x55132a: mov dword ptr [eax + 4], esi
0x55132d: jmp 0x55138c
0x55132f: mov dword ptr [0x721f24], ebx
0x551335: jmp 0x551345
0x551337: mov ecx, dword ptr [0x721f24]
0x55133d: cmp ecx, ebx
0x55133f: je 0x551345
0x551341: mov edx, dword ptr [ecx]
0x551343: call dword ptr [edx]
0x551345: push 0x1c
0x551347: call 0x6277da
0x55134c: add esp, 4
0x55134f: cmp eax, ebx
0x551351: je 0x55137a
0x551353: push esi
0x551354: mov dword ptr [eax], 0x671d4c
0x55135a: mov dword ptr [eax + 8], ebx
0x55135d: mov edx, dword ptr [eax]
0x55135f: push edi
0x551360: mov ecx, eax
0x551362: mov dword ptr [0x721f24], eax
0x551367: call dword ptr [edx + 4]
0x55136a: cmp eax, ebx
0x55136c: je 0x551380
0x55136e: mov eax, dword ptr [0x721f24]
0x551373: xor esi, esi
0x551375: mov dword ptr [eax + 4], ebx
0x551378: jmp 0x55138c
0x55137a: mov dword ptr [0x721f24], ebx
0x551380: or esi, 0xffffffff
0x551383: mov word ptr [0x746130], 2
0x55138c: lea eax, [esi + 1]
0x55138f: cmp eax, 4
0x551392: ja 0x5513d2
0x551394: jmp dword ptr [eax*4 + 0x551408]
0x55139b: push 0x671dfc
0x5513a0: lea ecx, [esp + 0xc]
0x5513a4: push ecx
0x5513a5: jmp 0x5513df
0x5513a7: push 0x671de0
0x5513ac: lea edx, [esp + 0xc]
0x5513b0: push edx
0x5513b1: jmp 0x5513df
0x5513b3: push 0x671dc4
0x5513b8: jmp 0x5513da
0x5513ba: push 0x671da8
0x5513bf: lea ecx, [esp + 0xc]
0x5513c3: push ecx
0x5513c4: jmp 0x5513df
0x5513c6: push 0x671d8c
0x5513cb: lea edx, [esp + 0xc]
0x5513cf: push edx
0x5513d0: jmp 0x5513df
0x5513d2: or esi, 0xffffffff
0x5513d5: push 0x671dfc
0x5513da: lea eax, [esp + 0xc]
0x5513de: push eax
0x5513df: call 0x623693
0x5513e4: add esp, 8
0x5513e7: lea ecx, [esp + 8]
0x5513eb: push ecx
0x5513ec: lea edx, [esp + 0x2c]
0x5513f0: push 0x671d70
0x5513f5: push edx
0x5513f6: call 0x623693
0x5513fb: add esp, 0xc
0x5513fe: mov eax, esi
0x551400: pop esi
0x551401: pop ebx
0x551402: add esp, 0x60
0x551405: ret 
#endif
