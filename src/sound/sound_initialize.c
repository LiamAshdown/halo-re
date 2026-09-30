// sound_initialize  (Ghidra: sound_initialize, already named)
// address 0x5492f0, size 425 bytes
// name confidence: 0.9   rewrite confidence: 0.9
// evidence: out/phase4/sound_functions.md summary "Allocates the sounds/looping-sounds datum
//   tables, resets the four gain sliders to 1.0, and opens the sound device via its driver
//   vtable."; types/sound.h explicitly documents this function's own quirk: "sound_initialize
//   calls the table through a `short *`, which is why the decompiled offsets there read `+2`,
//   `+0x1c`, `+0x1e` (bytes 4, 0x38, 0x3c)" -- i.e. offset 2 (shorts) = byte 4 = initialize,
//   offset 0x1c (shorts) = byte 0x38 = set_quality, offset 0x1e (shorts) = byte 0x3c =
//   eax_available; this rewrite calls those sound_driver fields by name instead of replicating
//   the short-indexed arithmetic. Only sound_channels[] (0x00724a60) is touched here, via mixed
//   int*/short* indexing that
//   resolves cleanly to sound_channel.sound_index/type_flags/current_permutation/next_permutation
//   (0x00/0x04/0x10/0x14) once read as that struct.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// UNSURE: LAB_0054ce50 / LAB_0054cf80 are small local thunks (sound_channel_parameters_proc
//   targets) embedded in this function's own code range, not in the 134-function list -- declared
//   here only as address-tagged function pointers, matching how types/sound.h treats the
//   sound_driver wrapper addresses.
// Phase-4 review (disassembly appended below): data_new takes the element size in EBX (0xb0 and
//   0xe4), which the draft dropped; everything else matched.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"
#include "fn_sound.h"

extern uint8_t sound_initialized;  // 0x00725200
extern uint8_t sound_enabled;      // 0x00725201
extern uint8_t sound_disabled;     // 0x007252b6
extern SoundEnvironment sound_environment; // 0x0072525c
extern const SoundEnvironment k_default_sound_environment; // 0x0065e508
extern float sound_ducking_gain;   // 0x007252a4
extern float sound_music_gain;     // 0x007252a8
extern float sound_master_gain;    // 0x007252ac
extern float sound_effects_gain;   // 0x007252b0
extern int16_t sound_permutation_limit; // 0x007252b8
extern sound_channel_parameters_proc sound_channel_parameters_proc_ptr; // 0x006e36cc
extern sound_driver *sound_drivers[2]; // 0x0069f508
extern sound_driver *current_sound_driver; // 0x00725208
extern data_array *sound_data;          // 0x007252c0
extern data_array *looping_sound_data;  // 0x00724a50
extern sound_driver_parameters driver_parameters; // 0x0069f514
extern uint16_t sound_channel_type_flag_table[4]; // 0x0069f528
extern int16_t sound_channel_count; // 0x007252b4
extern sound_channel sound_channels[k_maximum_sound_channels]; // 0x00724a60

extern void sound_cache_new(void); // 0x443ca0
extern data_array *data_new(int16_t element_size, char *name, int16_t maximum_count); // 0x4d0370, blam-cc: EBX -> element_size
extern void data_delete_all(data_array *array); // 0x4d0580


// Initializes the sound engine: resets state, allocates the sound cache, and (unless
// sound_disabled) resets the four gain sliders and reverb environment to their defaults, opens
// the configured DirectSound driver, allocates the "sounds"/"looping sounds" datum tables, and
// builds the logical sound_channels table from the driver's per-type slot counts.
void sound_initialize(void)
{
    sound_initialized = 0;
    sound_enabled = 1;
    sound_cache_new();

    if (sound_disabled == 0) {
        sound_environment = k_default_sound_environment;
        sound_ducking_gain = 1.0f;
        sound_music_gain = 1.0f;
        sound_master_gain = 1.0f;
        sound_effects_gain = 1.0f;
        sound_permutation_limit = 0;
        sound_channel_parameters_proc_ptr = sound_channel_parameters_proc_default;

        if (driver_parameters.driver_index >= 0 && driver_parameters.driver_index < 2) {
            sound_driver *driver = sound_drivers[driver_parameters.driver_index];

            if (driver != (sound_driver *)0 && driver->type == driver_parameters.driver_index) {
                current_sound_driver = driver;
                sound_data = data_new(sizeof(sound), "sounds", k_maximum_sounds); // EBX = 0xb0

                if (sound_data != (data_array *)0) {
                    looping_sound_data = data_new(sizeof(looping_sound), "looping sounds", k_maximum_looping_sounds); // EBX = 0xe4
                }

                if (sound_data != (data_array *)0 && looping_sound_data != (data_array *)0) {
                    uint8_t initialized;

                    driver->set_quality(0, 0, 1);
                    initialized = driver->initialize(&driver_parameters);

                    if (initialized != 0) {
                        int16_t index = 0;
                        int32_t type;

                        sound_data->valid = 1;
                        data_delete_all(sound_data);
                        looping_sound_data->valid = 1;
                        data_delete_all(looping_sound_data);

                        for (type = 0; type < 4; type++) {
                            int16_t slot_count = driver_parameters.slot_counts[type];

                            sound_channel_count += slot_count;

                            if (slot_count > 0) {
                                uint16_t type_flags = sound_channel_type_flag_table[type];
                                int16_t remaining = slot_count;

                                do {
                                    sound_channels[index].sound_index = (datum_index)0xffffffff;
                                    sound_channels[index].type_flags = type_flags;
                                    sound_channels[index].current_permutation = (SoundPermutation *)0;
                                    sound_channels[index].next_permutation = (SoundPermutation *)0;
                                    index++;
                                    remaining--;
                                } while (remaining != 0);
                            }
                        }

                        sound_initialized = 1;

                        if (driver->eax_available() != 0) {
                            sound_channel_parameters_proc_ptr = sound_channel_parameters_proc_eax;
                        }
                    }
                }
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x5492f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void sound_initialize(void)

{
  ushort uVar1;
  undefined2 uVar2;
  short *psVar3;
  char cVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  undefined4 *puVar8;
  int iVar9;
  short sVar10;
  undefined4 *puVar11;

  DAT_00725200 = 0;
  DAT_00725201 = 1;
  sound_cache_new();
  if (DAT_007252b6 == '\0') {
    puVar8 = &DAT_0065e508;
    puVar11 = &DAT_0072525c;
    for (iVar6 = 0x12; iVar6 != 0; iVar6 = iVar6 + -1) {
      *puVar11 = *puVar8;
      puVar8 = puVar8 + 1;
      puVar11 = puVar11 + 1;
    }
    _DAT_007252a4 = 0x3f800000;
    DAT_007252a8 = 0x3f800000;
    DAT_007252ac = 0x3f800000;
    DAT_007252b0 = 0x3f800000;
    DAT_007252b8 = 0;
    DAT_006e36cc = &LAB_0054ce50;
    if ((((-1 < DAT_0069f514) && (DAT_0069f514 < 2)) &&
        (psVar3 = (short *)(&PTR_DAT_0069f508)[DAT_0069f514], psVar3 != (short *)0x0)) &&
       (((*psVar3 == DAT_0069f514 &&
         (DAT_00725208 = psVar3, DAT_007252c0 = data_new("sounds",0x200), DAT_007252c0 != 0)) &&
        (DAT_00724a50 = data_new("looping sounds",0x80), DAT_00724a50 != 0)))) {
      (**(code **)(DAT_00725208 + 0x1c))(0,0,1);
      cVar4 = (**(code **)(DAT_00725208 + 2))(&DAT_0069f514);
      if (cVar4 != '\0') {
        sVar10 = 0;
        *(undefined1 *)(DAT_007252c0 + 0x24) = 1;
        data_delete_all();
        *(undefined1 *)(DAT_00724a50 + 0x24) = 1;
        data_delete_all();
        iVar9 = 0;
        iVar6 = 4;
        do {
          uVar1 = *(ushort *)((int)&DAT_0069f51e + iVar9);
          DAT_007252b4 = DAT_007252b4 + uVar1;
          if (0 < (short)uVar1) {
            uVar2 = *(undefined2 *)((int)&DAT_0069f528 + iVar9);
            uVar7 = (uint)uVar1;
            do {
              iVar5 = (int)sVar10;
              sVar10 = sVar10 + 1;
              uVar7 = uVar7 - 1;
              (&DAT_00724a60)[iVar5 * 6] = 0xffffffff;
              (&DAT_00724a64)[iVar5 * 0xc] = uVar2;
              (&DAT_00724a70)[iVar5 * 6] = 0;
              (&DAT_00724a74)[iVar5 * 6] = 0;
            } while (uVar7 != 0);
          }
          iVar9 = iVar9 + 2;
          iVar6 = iVar6 + -1;
        } while (iVar6 != 0);
        DAT_00725200 = 1;
        cVar4 = (**(code **)(DAT_00725208 + 0x1e))();
        if (cVar4 != '\0') {
          DAT_006e36cc = &LAB_0054cf80;
        }
      }
    }
  }
  return;
}

Disassembly (0x5492f0..0x549499, capstone; phase-4 review):

0x5492f0: push ebx
0x5492f1: xor ebx, ebx
0x5492f3: mov byte ptr [0x725200], bl
0x5492f9: mov byte ptr [0x725201], 1
0x549300: call 0x443ca0
0x549305: cmp byte ptr [0x7252b6], bl
0x54930b: jne 0x549497
0x549311: push esi
0x549312: push edi
0x549313: mov ecx, 0x12
0x549318: mov esi, 0x65e508
0x54931d: mov edi, 0x72525c
0x549322: rep movsd dword ptr es:[edi], dword ptr [esi]
0x549324: mov cx, word ptr [0x69f514]
0x54932b: cmp cx, bx
0x54932e: mov dword ptr [0x7252a4], 0x3f800000
0x549338: mov dword ptr [0x7252a8], 0x3f800000
0x549342: mov dword ptr [0x7252ac], 0x3f800000
0x54934c: mov dword ptr [0x7252b0], 0x3f800000
0x549356: mov word ptr [0x7252b8], bx
0x54935d: mov dword ptr [0x6e36cc], 0x54ce50
0x549367: jl 0x549495
0x54936d: cmp cx, 2
0x549371: jge 0x549495
0x549377: movsx eax, cx
0x54937a: mov eax, dword ptr [eax*4 + 0x69f508]
0x549381: cmp eax, ebx
0x549383: je 0x549495
0x549389: cmp word ptr [eax], cx
0x54938c: jne 0x549495
0x549392: push 0x200
0x549397: push 0x65fda0
0x54939c: mov ebx, 0xb0
0x5493a1: mov dword ptr [0x725208], eax
0x5493a6: call 0x4d0370
0x5493ab: add esp, 8
0x5493ae: test eax, eax
0x5493b0: mov dword ptr [0x7252c0], eax
0x5493b5: je 0x549495
0x5493bb: push ebp
0x5493bc: push 0x80
0x5493c1: push 0x6719a4
0x5493c6: mov ebx, 0xe4
0x5493cb: call 0x4d0370
0x5493d0: xor ebp, ebp
0x5493d2: add esp, 8
0x5493d5: cmp eax, ebp
0x5493d7: mov dword ptr [0x724a50], eax
0x5493dc: je 0x549494
0x5493e2: mov ecx, dword ptr [0x725208]
0x5493e8: push 1
0x5493ea: push ebp
0x5493eb: push ebp
0x5493ec: call dword ptr [ecx + 0x38]
0x5493ef: mov edx, dword ptr [0x725208]
0x5493f5: push 0x69f514
0x5493fa: call dword ptr [edx + 4]
0x5493fd: add esp, 0x10
0x549400: test al, al
0x549402: je 0x549494
0x549408: mov esi, dword ptr [0x7252c0]
0x54940e: xor edi, edi
0x549410: mov byte ptr [esi + 0x24], 1
0x549414: call 0x4d0580
0x549419: mov esi, dword ptr [0x724a50]
0x54941f: mov byte ptr [esi + 0x24], 1
0x549423: call 0x4d0580
0x549428: xor esi, esi
0x54942a: mov ebx, 4
0x54942f: nop 
0x549430: mov ax, word ptr [esi + 0x69f51e]
0x549437: add word ptr [0x7252b4], ax
0x54943e: cmp ax, bp
0x549441: jle 0x549471
0x549443: mov dx, word ptr [esi + 0x69f528]
0x54944a: movzx ecx, ax
0x54944d: lea ecx, [ecx]
0x549450: movsx eax, di
0x549453: lea eax, [eax + eax*2]
0x549456: lea eax, [eax*8 + 0x724a60]
0x54945d: inc edi
0x54945e: dec ecx
0x54945f: mov dword ptr [eax], 0xffffffff
0x549465: mov word ptr [eax + 4], dx
0x549469: mov dword ptr [eax + 0x10], ebp
0x54946c: mov dword ptr [eax + 0x14], ebp
0x54946f: jne 0x549450
0x549471: add esi, 2
0x549474: dec ebx
0x549475: jne 0x549430
0x549477: mov eax, dword ptr [0x725208]
0x54947c: mov byte ptr [0x725200], 1
0x549483: call dword ptr [eax + 0x3c]
0x549486: test al, al
0x549488: je 0x549494
0x54948a: mov dword ptr [0x6e36cc], 0x54cf80
0x549494: pop ebp
0x549495: pop edi
0x549496: pop esi
0x549497: pop ebx
0x549498: ret 
#endif
