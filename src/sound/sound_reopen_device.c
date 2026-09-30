// sound_reopen_device  (Ghidra: FUN_005494a0)
// address 0x5494a0, size 319 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Stops all playing sounds and reopens the sound
//   device with a new parameter, re-seeding the looping-sound slot table on success."; identical
//   sound_channels[] rebuild loop to src/sound/sound_initialize.c (same field offsets), this time
//   reading slot_counts from the caller-supplied sound_driver_parameters* rather than the global
//   one; sound_driver.dispose/initialize/eax_available (0x08/0x04/0x3c) called as ordinary struct
//   function pointers here (unlike sound_initialize's short-indexed calls into the same struct).
// register convention: new driver parameters pointer as the recognized stack parameter (param_1).
// blam-cc: stack -> new_parameters
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern data_array *sound_data;              // 0x007252c0
extern sound_driver *current_sound_driver;      // 0x00725208
extern uint8_t sound_initialized;           // 0x00725200
extern uint8_t sound_enabled;               // 0x00725201
extern int16_t sound_channel_count;         // 0x007252b4
extern sound_channel_parameters_proc sound_channel_parameters_proc_ptr; // 0x006e36cc
extern uint16_t sound_channel_type_flag_table[4]; // 0x0069f528
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630
extern void sound_instance_stop(datum_index sound_index); // 0x54b180


// blam-cc: stack -> new_parameters
// Stops every currently playing sound, disposes the current DirectSound device, then reinitializes
// it with `new_parameters`; on success, rebuilds sound_channels[] from the new per-type slot
// counts exactly as src/sound/sound_initialize.c does, and re-detects EAX availability.
uint8_t sound_reopen_device(sound_driver_parameters *new_parameters)
{
    datum_index index = datum_next(-1, sound_data);
    uint8_t initialized;

    while (index != k_datum_index_none) {
        sound_instance_stop(index);
        index = datum_next((int16_t)index, sound_data);
    }

    current_sound_driver->dispose();
    sound_initialized = 0;
    sound_enabled = 1;
    sound_channel_count = 0;
    sound_channel_parameters_proc_ptr = sound_channel_parameters_proc_default;

    initialized = current_sound_driver->initialize(new_parameters);

    if (initialized != 0) {
        int16_t channel_index = 0;
        int32_t type;

        for (type = 0; type < 4; type++) {
            int16_t slot_count = new_parameters->slot_counts[type];

            sound_channel_count += slot_count;

            if (slot_count > 0) {
                uint16_t type_flags = sound_channel_type_flag_table[type];
                int16_t remaining = slot_count;

                do {
                    sound_channels[channel_index].sound_index = (datum_index)0xffffffff;
                    sound_channels[channel_index].type_flags = type_flags;
                    sound_channels[channel_index].current_permutation = (SoundPermutation *)0;
                    sound_channels[channel_index].next_permutation = (SoundPermutation *)0;
                    channel_index++;
                    remaining--;
                } while (remaining != 0);
            }
        }

        sound_initialized = 1;

        if (current_sound_driver->eax_available() != 0) {
            sound_channel_parameters_proc_ptr = sound_channel_parameters_proc_eax;
        }
    }

    return initialized;
}

#if 0
Original Ghidra decompilation (0x5494a0):

char FUN_005494a0(int param_1)

{
  undefined2 uVar1;
  char cVar2;
  char cVar3;
  uint uVar4;
  short *psVar5;
  short sVar6;
  short sVar7;
  int iVar8;
  undefined2 *puVar9;
  int iStack_4;

  uVar4 = datum_next();
  do {
    do {
      if (uVar4 == 0xffffffff) {
        (**(code **)(DAT_00725208 + 8))();
        DAT_00725200 = 0;
        DAT_00725201 = 1;
        DAT_007252b4 = 0;
        DAT_006e36cc = &LAB_0054ce50;
        cVar2 = (**(code **)(DAT_00725208 + 4))(param_1);
        if (cVar2 != '\0') {
          sVar6 = 0;
          puVar9 = &DAT_0069f528;
          psVar5 = (short *)(param_1 + 10);
          iStack_4 = 4;
          do {
            DAT_007252b4 = DAT_007252b4 + *psVar5;
            sVar7 = 0;
            if (0 < *psVar5) {
              uVar1 = *puVar9;
              do {
                iVar8 = (int)sVar6;
                sVar6 = sVar6 + 1;
                sVar7 = sVar7 + 1;
                (&DAT_00724a60)[iVar8 * 6] = 0xffffffff;
                (&DAT_00724a64)[iVar8 * 0xc] = uVar1;
                (&DAT_00724a70)[iVar8 * 6] = 0;
                (&DAT_00724a74)[iVar8 * 6] = 0;
              } while (sVar7 < *psVar5);
            }
            puVar9 = puVar9 + 1;
            psVar5 = psVar5 + 1;
            iStack_4 = iStack_4 + -1;
          } while (iStack_4 != 0);
          DAT_00725200 = 1;
          cVar3 = (**(code **)(DAT_00725208 + 0x3c))();
          if (cVar3 != '\0') {
            DAT_006e36cc = &LAB_0054cf80;
          }
        }
        return cVar2;
      }
      sound_instance_stop(uVar4);
      iVar8 = uVar4 + 1;
      uVar4 = 0xffffffff;
      sVar6 = (short)iVar8;
    } while ((sVar6 < 0) || (*(short *)(DAT_007252c0 + 0x2e) <= sVar6));
    psVar5 = (short *)((int)sVar6 * (int)*(short *)(DAT_007252c0 + 0x22) +
                      *(int *)(DAT_007252c0 + 0x34));
    do {
      if (*psVar5 != 0) {
        uVar4 = (int)*psVar5 << 0x10 | (int)(short)iVar8;
        break;
      }
      iVar8 = iVar8 + 1;
      psVar5 = (short *)((int)psVar5 + (int)*(short *)(DAT_007252c0 + 0x22));
    } while ((short)iVar8 < *(short *)(DAT_007252c0 + 0x2e));
  } while( true );
}

Disassembly (0x5494a0..0x5495df, capstone; phase-4 review):

0x5494a0: sub esp, 8
0x5494a3: push ebx
0x5494a4: push ebp
0x5494a5: push esi
0x5494a6: push edi
0x5494a7: mov edi, dword ptr [0x7252c0]
0x5494ad: or edx, 0xffffffff
0x5494b0: call 0x4d0630
0x5494b5: mov esi, eax
0x5494b7: xor ebx, ebx
0x5494b9: cmp esi, -1
0x5494bc: je 0x549511
0x5494be: mov edi, edi
0x5494c0: push esi
0x5494c1: call 0x54b180
0x5494c6: lea ecx, [esi + 1]
0x5494c9: add esp, 4
0x5494cc: or edi, 0xffffffff
0x5494cf: cmp cx, bx
0x5494d2: jl 0x54950a
0x5494d4: mov esi, dword ptr [0x7252c0]
0x5494da: mov bp, word ptr [esi + 0x2e]
0x5494de: cmp cx, bp
0x5494e1: jge 0x54950a
0x5494e3: movsx edx, word ptr [esi + 0x22]
0x5494e7: movsx eax, cx
0x5494ea: imul eax, edx
0x5494ed: add eax, dword ptr [esi + 0x34]
0x5494f0: cmp word ptr [eax], bx
0x5494f3: jne 0x5494ff
0x5494f5: inc ecx
0x5494f6: add eax, edx
0x5494f8: cmp cx, bp
0x5494fb: jl 0x5494f0
0x5494fd: jmp 0x54950a
0x5494ff: movsx edi, word ptr [eax]
0x549502: movsx eax, cx
0x549505: shl edi, 0x10
0x549508: or edi, eax
0x54950a: cmp edi, -1
0x54950d: mov esi, edi
0x54950f: jne 0x5494c0
0x549511: mov ecx, dword ptr [0x725208]
0x549517: call dword ptr [ecx + 8]
0x54951a: mov edi, dword ptr [esp + 0x1c]
0x54951e: mov edx, dword ptr [0x725208]
0x549524: push edi
0x549525: mov byte ptr [0x725200], bl
0x54952b: mov byte ptr [0x725201], 1
0x549532: mov word ptr [0x7252b4], bx
0x549539: mov dword ptr [0x6e36cc], 0x54ce50
0x549543: call dword ptr [edx + 4]
0x549546: add esp, 4
0x549549: cmp al, bl
0x54954b: mov byte ptr [esp + 0x13], al
0x54954f: je 0x5495e7
0x549555: xor esi, esi
0x549557: mov ebp, 0x69f528
0x54955c: lea edx, [edi + 0xa]
0x54955f: mov dword ptr [esp + 0x14], 4
0x549567: jmp 0x549570
0x549569: lea esp, [esp]
0x549570: mov ax, word ptr [edx]
0x549573: add word ptr [0x7252b4], ax
0x54957a: xor ecx, ecx
0x54957c: cmp word ptr [edx], bx
0x54957f: jle 0x5495b4
0x549581: mov di, word ptr [ebp]
0x549585: jmp 0x549590
0x549587: lea esp, [esp]
0x54958e: mov edi, edi
0x549590: movsx eax, si
0x549593: lea eax, [eax + eax*2]
0x549596: lea eax, [eax*8 + 0x724a60]
0x54959d: inc esi
0x54959e: inc ecx
0x54959f: mov dword ptr [eax], 0xffffffff
0x5495a5: mov word ptr [eax + 4], di
0x5495a9: mov dword ptr [eax + 0x10], ebx
0x5495ac: mov dword ptr [eax + 0x14], ebx
0x5495af: cmp cx, word ptr [edx]
0x5495b2: jl 0x549590
0x5495b4: mov eax, dword ptr [esp + 0x14]
0x5495b8: add ebp, 2
0x5495bb: add edx, 2
0x5495be: dec eax
0x5495bf: mov dword ptr [esp + 0x14], eax
0x5495c3: jne 0x549570
0x5495c5: mov ecx, dword ptr [0x725208]
0x5495cb: mov byte ptr [0x725200], 1
0x5495d2: call dword ptr [ecx + 0x3c]
0x5495d5: test al, al
0x5495d7: mov al, byte ptr [esp + 0x13]
0x5495db: je 0x5495e7
#endif
