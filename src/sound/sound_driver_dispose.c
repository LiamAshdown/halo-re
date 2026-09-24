// sound_driver_dispose  (Ghidra: game_sound_dispose; renamed per out/phase4/sound_types_notes.md:
//   "0x546a60 game_sound_dispose: this is the DirectSound driver dispose (sound_driver slot
//   0x08), not game sound. game_sound has no dispose in this range.")
// address 0x546a60, size 220 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Shuts down the sound engine, releasing all
//   channel, listener, EAX, and DirectSound device COM interfaces."; types/sound.h sound_driver
//   lists this address as its `dispose` slot (0x08); directsound_channel.buffer/buffer_3d
//   (0x670/0x674), directsound_channel_count (0x00725428), sound_effect_object (0x00721f24),
//   directsound_listener/primary_buffer (0x00746114/0x00746110), directsound (0x0074610c) and
//   directsound_initialized (0x007252e0) all match by address. IDirectSoundBuffer vtable slot
//   0x48 is Stop; IDirectSound vtable slot 0x18 is SetCooperativeLevel(hwnd, level).
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// Phase-4 review: checked against the disassembly appended below; matches. The channel loop
// now reads directsound_channels[i].buffer / .buffer_3d instead of a separate alias global at
// 0x00725aa0.

#include "tags.h"
#include "memory.h"
#include "sound.h"

extern directsound_channel directsound_channels[k_maximum_sound_channels]; // 0x00725430 (the loop walks &[i].buffer, 0x00725aa0)
extern int16_t directsound_channel_count;   // 0x00725428
extern sound_effect_object *global_sound_effect_object; // 0x00721f24
extern int16_t sound_effect_object_state;    // 0x00746130
extern void *directsound_listener;           // 0x00746114
extern void *directsound_primary_buffer;     // 0x00746110
extern void *directsound;                    // 0x0074610c
extern uint8_t directsound_initialized;      // 0x007252e0

extern void free(void *block); // 0x6277e8, CRT free
extern void *GetActiveWindow(void); // import 0x0063a424

// Releases every directsound_channel's buffer and 3D-buffer interfaces, shuts down and frees the
// global EAX sound effects object, releases the listener and primary buffer, restores the
// DirectSound device's cooperative level before releasing it, and clears every associated global.
void sound_driver_dispose(void)
{
    int32_t i;

    for (i = 0; i < directsound_channel_count; i++) {
        void *buffer = directsound_channels[i].buffer;
        void *buffer_3d = directsound_channels[i].buffer_3d;

        if (buffer_3d != 0) {
            void **vtable = *(void ***)buffer_3d;
            void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
            release(buffer_3d);
        }
        if (buffer != 0) {
            void **vtable = *(void ***)buffer;
            void (__stdcall *stop)(void *) = (void (__stdcall *)(void *))vtable[0x48 / 4];
            void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
            stop(buffer);
            release(buffer);
        }
    }
    directsound_channel_count = 0;

    if (global_sound_effect_object != 0) {
        void **vtable = *(void ***)global_sound_effect_object;
        void (__stdcall *shutdown)(void *) = (void (__stdcall *)(void *))vtable[0];
        shutdown(global_sound_effect_object);
        free(global_sound_effect_object);
        global_sound_effect_object = 0;
        sound_effect_object_state = 0;
    }

    if (directsound_listener != 0) {
        void **vtable = *(void ***)directsound_listener;
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        release(directsound_listener);
        directsound_listener = 0;
    }

    if (directsound_primary_buffer != 0) {
        void **vtable = *(void ***)directsound_primary_buffer;
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        release(directsound_primary_buffer);
        directsound_primary_buffer = 0;
    }

    if (directsound != 0) {
        void **vtable = *(void ***)directsound;
        int32_t (__stdcall *set_cooperative_level)(void *, void *, uint32_t) =
            (int32_t (__stdcall *)(void *, void *, uint32_t))vtable[0x18 / 4];
        void (__stdcall *release)(void *) = (void (__stdcall *)(void *))vtable[2];
        void *active_window = GetActiveWindow();

        set_cooperative_level(directsound, active_window, 1);
        release(directsound);
        directsound = 0;
    }

    directsound_initialized = 0;
}

#if 0
Original Ghidra decompilation (0x546a60):

void __cdecl game_sound_dispose(void)

{
  int *piVar1;
  HWND pHVar2;
  int *piVar3;
  int iVar4;
  undefined4 uVar5;

  iVar4 = 0;
  if (0 < DAT_00725428) {
    piVar3 = &DAT_00725aa0;
    do {
      piVar1 = (int *)piVar3[1];
      if (piVar1 != (int *)0x0) {
        (**(code **)(*piVar1 + 8))(piVar1);
      }
      piVar1 = (int *)*piVar3;
      if (piVar1 != (int *)0x0) {
        (**(code **)(*piVar1 + 0x48))(piVar1);
        (**(code **)(*(int *)*piVar3 + 8))((int *)*piVar3);
      }
      iVar4 = iVar4 + 1;
      piVar3 = piVar3 + 0x19e;
    } while (iVar4 < DAT_00725428);
  }
  DAT_00725428 = 0;
  if (DAT_00721f24 != (undefined4 *)0x0) {
    (**(code **)*DAT_00721f24)();
    _free(DAT_00721f24);
    DAT_00721f24 = (undefined4 *)0x0;
    DAT_00746130 = 0;
  }
  if (DAT_00746114 != (int *)0x0) {
    (**(code **)(*DAT_00746114 + 8))(DAT_00746114);
    DAT_00746114 = (int *)0x0;
  }
  if (DAT_00746110 != (int *)0x0) {
    (**(code **)(*DAT_00746110 + 8))(DAT_00746110);
    DAT_00746110 = (int *)0x0;
  }
  if (DAT_0074610c != (int *)0x0) {
    iVar4 = *DAT_0074610c;
    uVar5 = 1;
    pHVar2 = GetActiveWindow();
    (**(code **)(iVar4 + 0x18))(DAT_0074610c,pHVar2,uVar5);
    (**(code **)(*DAT_0074610c + 8))(DAT_0074610c);
    DAT_0074610c = (int *)0x0;
  }
  DAT_007252e0 = 0;
  return;
}

Disassembly (0x546a60..0x546b3c, capstone; phase-4 review):

0x546a60: push ebx
0x546a61: push esi
0x546a62: push edi
0x546a63: xor ebx, ebx
0x546a65: xor edi, edi
0x546a67: cmp word ptr [0x725428], bx
0x546a6e: jle 0x546aa8
0x546a70: mov esi, 0x725aa0
0x546a75: mov eax, dword ptr [esi + 4]
0x546a78: cmp eax, ebx
0x546a7a: je 0x546a82
0x546a7c: mov ecx, dword ptr [eax]
0x546a7e: push eax
0x546a7f: call dword ptr [ecx + 8]
0x546a82: mov eax, dword ptr [esi]
0x546a84: cmp eax, ebx
0x546a86: je 0x546a96
0x546a88: mov edx, dword ptr [eax]
0x546a8a: push eax
0x546a8b: call dword ptr [edx + 0x48]
0x546a8e: mov eax, dword ptr [esi]
0x546a90: mov ecx, dword ptr [eax]
0x546a92: push eax
0x546a93: call dword ptr [ecx + 8]
0x546a96: movsx edx, word ptr [0x725428]
0x546a9d: inc edi
0x546a9e: add esi, 0x678
0x546aa4: cmp edi, edx
0x546aa6: jl 0x546a75
0x546aa8: mov ecx, dword ptr [0x721f24]
0x546aae: cmp ecx, ebx
0x546ab0: mov word ptr [0x725428], bx
0x546ab7: je 0x546ad9
0x546ab9: mov eax, dword ptr [ecx]
0x546abb: call dword ptr [eax]
0x546abd: mov ecx, dword ptr [0x721f24]
0x546ac3: push ecx
0x546ac4: call 0x6277e8
0x546ac9: add esp, 4
0x546acc: mov dword ptr [0x721f24], ebx
0x546ad2: mov word ptr [0x746130], bx
0x546ad9: mov eax, dword ptr [0x746114]
0x546ade: cmp eax, ebx
0x546ae0: je 0x546aee
0x546ae2: mov edx, dword ptr [eax]
0x546ae4: push eax
0x546ae5: call dword ptr [edx + 8]
0x546ae8: mov dword ptr [0x746114], ebx
0x546aee: mov eax, dword ptr [0x746110]
0x546af3: cmp eax, ebx
0x546af5: je 0x546b03
0x546af7: mov ecx, dword ptr [eax]
0x546af9: push eax
0x546afa: call dword ptr [ecx + 8]
0x546afd: mov dword ptr [0x746110], ebx
0x546b03: mov eax, dword ptr [0x74610c]
0x546b08: cmp eax, ebx
0x546b0a: je 0x546b32
0x546b0c: mov esi, dword ptr [eax]
0x546b0e: push 1
0x546b10: call dword ptr [0x63a424]
0x546b16: mov edx, dword ptr [0x74610c]
0x546b1c: push eax
0x546b1d: push edx
0x546b1e: call dword ptr [esi + 0x18]
0x546b21: mov eax, dword ptr [0x74610c]
0x546b26: mov ecx, dword ptr [eax]
0x546b28: push eax
0x546b29: call dword ptr [ecx + 8]
0x546b2c: mov dword ptr [0x74610c], ebx
0x546b32: pop edi
0x546b33: pop esi
0x546b34: mov byte ptr [0x7252e0], bl
0x546b3a: pop ebx
0x546b3b: ret 
#endif
