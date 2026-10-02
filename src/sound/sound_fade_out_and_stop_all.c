// sound_fade_out_and_stop_all  (Ghidra: FUN_005495f0; earlier draft name sound_resume)
// address 0x5495f0, size 355 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: one caller, in the game module right after game_sound_revert_scripting_sounds
//   (0x543a90) during a game reset/revert. Globals: sound_paused/sound_initialized/
//   sound_enabled/sound_disabled/sound_driver/sound_time (types/sound.h). The 300 ms constant is
//   the float at 0x0067314c; the 0.3 s fade is the literal 0x3e99999a.
// Phase-4 review: rewritten from the disassembly appended below. The earlier draft called
//   datum_next(-1) on every iteration (an endless loop), passed the fade arguments in the wrong
//   slots and inverted the final paused test. Real flow: when not paused, every live sound gets a
//   0.3 s fade-out (sound_schedule_gain_fade with EBX = none, i.e. nothing fading in) and the
//   sound system is pumped (sound_idle_update) for 300 ms of wall time; then, if the device is
//   (or became) paused it is unpaused and the sound clock resynchronized; finally everything is
//   stopped and the looping sound table cleared.
// register convention: plain __cdecl, no parameters.
// time_query_performance_counter_ms (outside this module) returns QueryPerformanceCounter * 1000 / frequency, the
//   same millisecond clock as the wait loop (checked).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t sound_paused;        // 0x00725202
extern uint8_t sound_initialized;   // 0x00725200
extern uint8_t sound_enabled;       // 0x00725201
extern uint8_t sound_disabled;      // 0x007252b6
extern data_array *sound_data;      // 0x007252c0
extern data_array *looping_sound_data; // 0x00724a50
extern sound_driver *current_sound_driver; // 0x00725208
extern int32_t sound_time;          // 0x0072520c
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

extern int32_t time_query_performance_counter_ms(void); // 0x449210, outside this module: QueryPerformanceCounter * 1000 / frequency, i.e. the same millisecond clock as sound_update_clock (checked in the phase-4 review)
extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630, blam-cc: DX, EDI
extern void sound_schedule_gain_fade(datum_index fade_in_handle, int16_t fade_curve, float duration_seconds,
    datum_index fade_out_handle); // 0x54af60, blam-cc: EBX, stack
extern void sound_idle_update(void); // 0x549960
extern void sound_stop_all(void); // 0x54adb0
extern void data_delete_all(data_array *array); // 0x4d0580

static int32_t sound_fade_now_ms(void)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    return (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

// Fades every playing sound out over 0.3 s (letting the mixer run for 300 ms), makes sure the
// device is unpaused, then stops all sounds and deletes all looping sounds.
void sound_fade_out_and_stop_all(void)
{
    if (sound_paused == 0) {
        float deadline;
        datum_index index;

        if (sound_initialized == 0 || sound_enabled == 0 || sound_disabled != 0) {
            goto stop;
        }

        deadline = (float)time_query_performance_counter_ms();
        index = datum_next(-1, sound_data);
        if (index != k_datum_index_none) {
            do {
                sound_schedule_gain_fade(k_datum_index_none, _sound_fade_linear, 0.3f, index);
                index = datum_next((int16_t)index, sound_data);
            } while (index != k_datum_index_none);

            deadline += 300.0f;
            for (;;) {
                int32_t now = sound_fade_now_ms();
                float now_unsigned = (float)now;

                if (now < 0) {
                    now_unsigned += 4294967296.0f;
                }
                if (!(now_unsigned < deadline)) {
                    break;
                }
                sound_idle_update();
            }
        }

        if (sound_paused == 0) {
            goto stop;
        }
    }

    sound_paused = 0;
    if (current_sound_driver != 0) {
        current_sound_driver->set_paused(0);
    }
    sound_time = sound_fade_now_ms();

stop:
    sound_stop_all();
    if (looping_sound_data != 0 && looping_sound_data->valid != 0) {
        data_delete_all(looping_sound_data);
    }
}

#if 0
Original Ghidra decompilation (0x5495f0):

void FUN_005495f0(void)

{
  float fVar1;
  int iVar2;
  int iVar3;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  if (DAT_00725202 == '\0') {
    if (((DAT_00725200 == '\0') || (DAT_00725201 == '\0')) || (DAT_007252b6 != '\0'))
    goto LAB_00549732;
    iVar2 = FUN_00449210();
    iVar3 = datum_next();
    if (iVar3 != -1) {
      do {
        FUN_0054af60(0,0x3e99999a,iVar3);
        iVar3 = datum_next();
      } while (iVar3 != -1);
      while( true ) {
        QueryPerformanceCounter(&local_8);
        uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
        iVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
        fVar1 = (float)iVar3;
        if (iVar3 < 0) {
          fVar1 = fVar1 + 4.2949673e+09;
        }
        if ((float)iVar2 + 300.0 <= fVar1) break;
        FUN_00549960();
      }
    }
    if (DAT_00725202 == '\0') goto LAB_00549732;
  }
  DAT_00725202 = '\0';
  if (DAT_00725208 != 0) {
    (**(code **)(DAT_00725208 + 0x28))(0);
  }
  QueryPerformanceCounter(&local_8);
  uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DAT_0072520c = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
LAB_00549732:
  FUN_0054adb0();
  if ((DAT_00724a50 != 0) && (*(char *)(DAT_00724a50 + 0x24) != '\0')) {
    data_delete_all();
  }
  return;
}

Disassembly (0x5495f0..0x549753, capstone; phase-4 review):

0x5495f0: mov al, byte ptr [0x725202]
0x5495f5: sub esp, 0x10
0x5495f8: test al, al
0x5495fa: push ebp
0x5495fb: mov ebp, dword ptr [0x63a0ac]
0x549601: push esi
0x549602: jne 0x5496e3
0x549608: mov al, byte ptr [0x725200]
0x54960d: test al, al
0x54960f: je 0x549732
0x549615: mov al, byte ptr [0x725201]
0x54961a: test al, al
0x54961c: je 0x549732
0x549622: mov al, byte ptr [0x7252b6]
0x549627: test al, al
0x549629: jne 0x549732
0x54962f: push edi
0x549630: call 0x449210
0x549635: mov edi, dword ptr [0x7252c0]
0x54963b: or edx, 0xffffffff
0x54963e: mov dword ptr [esp + 0xc], eax
0x549642: call 0x4d0630
0x549647: mov esi, eax
0x549649: cmp esi, -1
0x54964c: je 0x5496d9
0x549652: push ebx
0x549653: push esi
0x549654: push 0x3e99999a
0x549659: push 0
0x54965b: or ebx, 0xffffffff
0x54965e: call 0x54af60
0x549663: add esp, 0xc
0x549666: mov edx, esi
0x549668: call 0x4d0630
0x54966d: mov esi, eax
0x54966f: cmp esi, -1
0x549672: jne 0x549653
0x549674: fild dword ptr [esp + 0x10]
0x549678: pop ebx
0x549679: fadd dword ptr [0x67314c]
0x54967f: fstp dword ptr [esp + 0xc]
0x549683: lea eax, [esp + 0x14]
0x549687: push eax
0x549688: call ebp
0x54968a: mov ecx, dword ptr [esp + 0x18]
0x54968e: mov edx, dword ptr [esp + 0x14]
0x549692: push 0
0x549694: push 0x3e8
0x549699: push ecx
0x54969a: push edx
0x54969b: call 0x62de80
0x5496a0: mov ecx, dword ptr [0x6ac8fc]
0x5496a6: push ecx
0x5496a7: mov ecx, dword ptr [0x6ac8f8]
0x5496ad: push ecx
0x5496ae: push edx
0x5496af: push eax
0x5496b0: call 0x639230
0x5496b5: test eax, eax
0x5496b7: mov dword ptr [esp + 0x10], eax
0x5496bb: fild dword ptr [esp + 0x10]
0x5496bf: jge 0x5496c7
0x5496c1: fadd dword ptr [0x672bc0]
0x5496c7: fcomp dword ptr [esp + 0xc]
0x5496cb: fnstsw ax
0x5496cd: test ah, 5
0x5496d0: jp 0x5496d9
0x5496d2: call 0x549960
0x5496d7: jmp 0x549683
0x5496d9: mov al, byte ptr [0x725202]
0x5496de: test al, al
0x5496e0: pop edi
0x5496e1: je 0x549732
0x5496e3: mov eax, dword ptr [0x725208]
0x5496e8: test eax, eax
0x5496ea: mov byte ptr [0x725202], 0
0x5496f1: je 0x5496fb
0x5496f3: push 0
0x5496f5: call dword ptr [eax + 0x28]
0x5496f8: add esp, 4
0x5496fb: lea edx, [esp + 0x10]
0x5496ff: push edx
0x549700: call ebp
0x549702: mov eax, dword ptr [esp + 0x14]
0x549706: mov ecx, dword ptr [esp + 0x10]
0x54970a: push 0
0x54970c: push 0x3e8
0x549711: push eax
0x549712: push ecx
0x549713: call 0x62de80
0x549718: mov ecx, dword ptr [0x6ac8fc]
0x54971e: push ecx
0x54971f: mov ecx, dword ptr [0x6ac8f8]
0x549725: push ecx
0x549726: push edx
0x549727: push eax
0x549728: call 0x639230
0x54972d: mov dword ptr [0x72520c], eax
0x549732: call 0x54adb0
0x549737: mov esi, dword ptr [0x724a50]
0x54973d: test esi, esi
0x54973f: je 0x54974d
0x549741: mov al, byte ptr [esi + 0x24]
0x549744: test al, al
0x549746: je 0x54974d
0x549748: call 0x4d0580
0x54974d: pop esi
0x54974e: pop ebp
0x54974f: add esp, 0x10
0x549752: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
