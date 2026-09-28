// sound_effects_object_reinitialize  (Ghidra: sound_effects_object_reinitialize, already named,
// __cdecl)
// address 0x5514d0, size 321 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Enables or disables the global EAX sound effects
// object, (re)initializing it against the current listener by trying EAX3, EAX2, then EAX1
// backends in order."; same probe sequence as sound_effects_object_detect_mode.c (0x551270),
// confirmed identical by the shared malloc sizes and vtable pointers, but with an explicit
// channels argument (&directsound_channels[0], 0) at every initialize() call -- used to correct
// that file's own initialize() argument guess.
// register convention: __cdecl, stack -> enable.
// The double `vtable->shutdown()` call right before re-probing (once unconditionally, once more
// if the object global still isn't null afterward) is preserved literally; it looks redundant
// but nothing here clears the global between the two calls, so it is not obviously a decompiler
// artifact.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "sound.h"

extern uint8_t directsound_initialized; // 0x007252e0
extern sound_effect_object *global_sound_effect_object; // 0x00721f24
extern int16_t sound_effect_object_state; // 0x00746130
extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430

extern sound_effect_object_vtable sound_eax3_vtable; // 0x00671d28
extern sound_effect_object_vtable sound_eax2_vtable; // 0x00671d04
extern sound_effect_object_vtable sound_eax1_vtable; // 0x00671d4c

extern void sound_effects_object_shutdown(void); // this module, 0x551420
extern int sound_effects_object_apply_all_channels(void); // this module, 0x551480

void __cdecl sound_effects_object_reinitialize(int enable)
{
    if (!directsound_initialized) {
        return;
    }

    if (enable == 0) {
        if (global_sound_effect_object == 0) {
            return;
        }
        sound_effects_object_shutdown();
        sound_effect_object_state = 2;
        return;
    }

    if (global_sound_effect_object != 0) {
        int16_t mode = global_sound_effect_object->mode;
        if (mode == 0 || mode == 1 || mode == 2) {
            return; // already initialized to a real EAX backend
        }
    }

    sound_effect_object_state = 0;
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable = &sound_eax3_vtable;
        global_sound_effect_object->supported_properties = 0;
        if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
            sound_effects_object_apply_all_channels();
            return;
        }
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0xe8);
    if (global_sound_effect_object != 0) {
        global_sound_effect_object->vtable = &sound_eax2_vtable;
        global_sound_effect_object->supported_properties = 0;
        if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
            sound_effects_object_apply_all_channels();
            return;
        }
        if (global_sound_effect_object != 0) {
            global_sound_effect_object->vtable->shutdown(global_sound_effect_object);
        }
    }

    global_sound_effect_object = (sound_effect_object *)malloc(0x1c);
    if (global_sound_effect_object == 0) {
        return;
    }
    global_sound_effect_object->vtable = &sound_eax1_vtable;
    global_sound_effect_object->supported_properties = 0;
    if (global_sound_effect_object->vtable->initialize(global_sound_effect_object, directsound_channels, 0) != 0) {
        sound_effects_object_apply_all_channels();
    }
}

#if 0
Original Ghidra decompilation (0x5514d0):

void __cdecl sound_effects_object_reinitialize(int enable)

{
  int iVar1;

  if (DAT_007252e0 != '\0') {
    if (enable == 0) {
      if (DAT_00721f24 == (int *)0x0) {
        return;
      }
      sound_effects_object_shutdown();
      DAT_00746130 = 2;
      return;
    }
    if (DAT_00721f24 != (int *)0x0) {
      iVar1 = DAT_00721f24[1];
      if (iVar1 == 0) {
        return;
      }
      if (iVar1 == 1) {
        return;
      }
      if (iVar1 == 2) {
        return;
      }
    }
    DAT_00746130 = 0;
    if ((DAT_00721f24 != (int *)0x0) && ((**(code **)*DAT_00721f24)(), DAT_00721f24 != (int *)0x0))
    {
      (**(code **)*DAT_00721f24)();
    }
    DAT_00721f24 = operator_new(0xe8);
    if (DAT_00721f24 == (int *)0x0) {
      DAT_00721f24 = (int *)0x0;
    }
    else {
      *DAT_00721f24 = (int)&PTR_sound_eax30_effect_shutdown_00671d28;
      DAT_00721f24[2] = 0;
      iVar1 = (**(code **)(*DAT_00721f24 + 4))(&DAT_00725430,0);
      if (iVar1 != 0) goto LAB_005515db;
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
      iVar1 = (**(code **)(*DAT_00721f24 + 4))(&DAT_00725430,0);
      if (iVar1 != 0) goto LAB_005515db;
      if (DAT_00721f24 != (int *)0x0) {
        (**(code **)*DAT_00721f24)();
      }
    }
    DAT_00721f24 = operator_new(0x1c);
    if (DAT_00721f24 == (int *)0x0) {
      DAT_00721f24 = (int *)0x0;
      return;
    }
    *DAT_00721f24 = (int)&PTR_FUN_00671d4c;
    DAT_00721f24[2] = 0;
    iVar1 = (**(code **)(*DAT_00721f24 + 4))(&DAT_00725430,0);
    if (iVar1 != 0) {
LAB_005515db:
      sound_effects_object_apply_all_channels();
      return;
    }
  }
  return;
}

Disassembly (0x5514d0..0x551611, capstone; phase-4 review):

0x5514d0: mov al, byte ptr [0x7252e0]
0x5514d5: push ebx
0x5514d6: xor ebx, ebx
0x5514d8: cmp al, bl
0x5514da: je 0x55160f
0x5514e0: cmp dword ptr [esp + 8], ebx
0x5514e4: je 0x5515f9
0x5514ea: mov ecx, dword ptr [0x721f24]
0x5514f0: cmp ecx, ebx
0x5514f2: je 0x551511
0x5514f4: mov eax, dword ptr [ecx + 4]
0x5514f7: cmp eax, ebx
0x5514f9: je 0x55160f
0x5514ff: cmp eax, 1
0x551502: je 0x55160f
0x551508: cmp eax, 2
0x55150b: je 0x55160f
0x551511: cmp ecx, ebx
0x551513: mov word ptr [0x746130], bx
0x55151a: je 0x55152e
0x55151c: mov eax, dword ptr [ecx]
0x55151e: call dword ptr [eax]
0x551520: mov ecx, dword ptr [0x721f24]
0x551526: cmp ecx, ebx
0x551528: je 0x55152e
0x55152a: mov edx, dword ptr [ecx]
0x55152c: call dword ptr [edx]
0x55152e: push 0xe8
0x551533: call 0x6277da
0x551538: add esp, 4
0x55153b: cmp eax, ebx
0x55153d: je 0x5515e1
0x551543: push ebx
0x551544: mov dword ptr [eax], 0x671d28
0x55154a: mov dword ptr [eax + 8], ebx
0x55154d: mov edx, dword ptr [eax]
0x55154f: push 0x725430
0x551554: mov ecx, eax
0x551556: mov dword ptr [0x721f24], eax
0x55155b: call dword ptr [edx + 4]
0x55155e: cmp eax, ebx
0x551560: jne 0x5515db
0x551562: mov ecx, dword ptr [0x721f24]
0x551568: cmp ecx, ebx
0x55156a: je 0x551570
0x55156c: mov eax, dword ptr [ecx]
0x55156e: call dword ptr [eax]
0x551570: push 0xe8
0x551575: call 0x6277da
0x55157a: add esp, 4
0x55157d: cmp eax, ebx
0x55157f: je 0x5515e9
0x551581: push ebx
0x551582: mov dword ptr [eax], 0x671d04
0x551588: mov dword ptr [eax + 8], ebx
0x55158b: mov edx, dword ptr [eax]
0x55158d: push 0x725430
0x551592: mov ecx, eax
0x551594: mov dword ptr [0x721f24], eax
0x551599: call dword ptr [edx + 4]
0x55159c: cmp eax, ebx
0x55159e: jne 0x5515db
0x5515a0: mov ecx, dword ptr [0x721f24]
0x5515a6: cmp ecx, ebx
0x5515a8: je 0x5515ae
0x5515aa: mov eax, dword ptr [ecx]
0x5515ac: call dword ptr [eax]
0x5515ae: push 0x1c
0x5515b0: call 0x6277da
0x5515b5: add esp, 4
0x5515b8: cmp eax, ebx
0x5515ba: je 0x5515f1
0x5515bc: push ebx
0x5515bd: mov dword ptr [eax], 0x671d4c
0x5515c3: mov dword ptr [eax + 8], ebx
0x5515c6: mov edx, dword ptr [eax]
0x5515c8: push 0x725430
0x5515cd: mov ecx, eax
0x5515cf: mov dword ptr [0x721f24], eax
0x5515d4: call dword ptr [edx + 4]
0x5515d7: cmp eax, ebx
0x5515d9: je 0x55160f
0x5515db: pop ebx
0x5515dc: jmp 0x551480
0x5515e1: mov dword ptr [0x721f24], ebx
0x5515e7: jmp 0x551570
0x5515e9: mov dword ptr [0x721f24], ebx
0x5515ef: jmp 0x5515ae
0x5515f1: mov dword ptr [0x721f24], ebx
0x5515f7: pop ebx
0x5515f8: ret 
0x5515f9: cmp dword ptr [0x721f24], ebx
0x5515ff: je 0x55160f
0x551601: call 0x551420
0x551606: mov word ptr [0x746130], 2
0x55160f: pop ebx
0x551610: ret 
#endif
