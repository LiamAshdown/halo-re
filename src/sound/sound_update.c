// sound_update  (Ghidra: sound_update, already named)
// address 0x549810, size 336 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Per-tick sound system update: handles focus
//   loss/regain and, at a fixed cadence, updates the 3D listener, looping sounds, channel
//   assignment and instance gains."; game_time_globals.paused (0x02, types/game.h);
//   sound_paused/current_sound_driver/sound_time/sound_time_delta/sound_update_toggle
//   (types/sound.h) match by offset; sound_driver.begin_frame/end_frame (0x10/0x14); the >0x20ms
//   gate matches k_sound_update_interval_ms; reuses sound_update_clock (0x54ae60, already named)
//   and sound_class_update_gain_fade (0x545330, this batch). When paused, sound_update_clock's
//   effect on sound_time/sound_time_delta is rolled back after the cadence check runs (the old
//   values are saved before the call and restored unless the un-paused full-update path refreshes
//   them to the post-clock values instead).
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// 0x006f1d20 is the game module's current_game_engine (NULL in campaign); 0x006b7020 (a byte
//   that together with it gates pausing) has no established name. time_query_performance_counter_ms (outside this
//   module) returns the QueryPerformanceCounter millisecond clock. Phase-4 review: checked
//   against the disassembly; the three per-tick passes are sound_update_looping_states,
//   sound_update_range_and_ducking and sound_assign_channels, in that order.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "sound.h"

extern uint8_t sound_disabled;       // 0x007252b6
extern game_time_globals *game_time; // 0x006f1d6c
extern uint8_t unknown_6b7020;       // 0x006b7020, UNSURE, see file header
extern game_engine_definition *current_game_engine; // 0x006f1d20, game module (multiplayer engine, NULL in campaign)
extern uint8_t sound_paused;         // 0x00725202
extern sound_driver *current_sound_driver; // 0x00725208
extern int32_t sound_time;           // 0x0072520c
extern uint8_t sound_initialized;    // 0x00725200
extern uint8_t sound_enabled;        // 0x00725201
extern float sound_time_delta;       // 0x00725210
extern uint8_t sound_update_toggle;  // 0x00725214
extern struct cache *sound_cache;    // 0x006ac530

extern int32_t time_query_performance_counter_ms(void); // 0x449210, outside this module: QueryPerformanceCounter * 1000 / frequency, i.e. the same millisecond clock as sound_update_clock (checked in the phase-4 review)
extern void sound_class_update_gain_fade(int32_t ticks); // 0x545330
extern void sound_update_clock(void); // 0x54ae60
extern void sound_update_listener(void); // 0x54b970
extern void sound_update_range_and_ducking(void); // 0x54bd60
extern void sound_assign_channels(void); // 0x54c020
extern void sound_update_active_instances(void); // 0x54c900
extern void sound_update_looping_states(void); // 0x54d270

// Per-tick sound engine update: syncs sound_paused to the game-pause/focus state (telling the
// driver and, on resume, reseeding sound_time), then -- when initialized/enabled and at least
// k_sound_update_interval_ms has elapsed -- brackets a full update pass (gain fades, 3D listener,
// looping-sound maintenance, channel assignment, active-instance gains, and the double-buffer
// toggle) between the driver's begin_frame/end_frame, rolling back the clock advance if the
// engine ended up paused. Finally ages the sound cache by one tick while not paused.
void sound_update(void)
{
    if (sound_disabled != 0) {
        return;
    }

    if (game_time->paused == 0 && (unknown_6b7020 == 0 || current_game_engine != 0)) {
        if (sound_paused != 0) {
            sound_paused = 0;
            if (current_sound_driver != 0) {
                current_sound_driver->set_paused(0);
            }
            sound_time = time_query_performance_counter_ms(); // millisecond clock
        }
    } else if (sound_paused != 1) {
        sound_paused = 1;
        if (current_sound_driver != 0) {
            current_sound_driver->set_paused(1);
        }
    }

    {
        float saved_delta = sound_time_delta;
        int32_t saved_time = sound_time;

        if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
            uint8_t due;

            sound_update_clock();
            due = (uint32_t)(sound_time - saved_time) > 0x20;

            if (due) {
                current_sound_driver->begin_frame();
            }

            if (sound_paused == 0 && due) {
                sound_class_update_gain_fade((int32_t)sound_time_delta);
                sound_update_listener();
                sound_update_looping_states();
                sound_update_range_and_ducking();
                sound_assign_channels();
                sound_update_active_instances();
                sound_update_toggle = sound_update_toggle == 0;
                saved_time = sound_time;
                saved_delta = sound_time_delta;
            }

            sound_time_delta = saved_delta;
            sound_time = saved_time;

            if (due) {
                current_sound_driver->end_frame();
            }
        }
    }

    if (sound_paused == 0) {
        sound_cache->age += 1;
    }
}

#if 0
Original Ghidra decompilation (0x549810):

void sound_update(void)

{
  int iVar1;
  undefined4 uVar2;
  bool bVar3;

  if (DAT_007252b6 == '\0') {
    if ((*(char *)(DAT_006f1d6c + 2) == '\0') && ((DAT_006b7020 == '\0' || (DAT_006f1d20 != 0)))) {
      if (DAT_00725202 != '\0') {
        DAT_00725202 = '\0';
        if (DAT_00725208 != 0) {
          (**(code **)(DAT_00725208 + 0x28))(0);
        }
        DAT_0072520c = FUN_00449210();
      }
    }
    else if ((DAT_00725202 != '\x01') && (DAT_00725202 = '\x01', DAT_00725208 != 0)) {
      (**(code **)(DAT_00725208 + 0x28))(1);
    }
    uVar2 = DAT_00725210;
    iVar1 = DAT_0072520c;
    if (((DAT_00725200 != '\0') && (DAT_00725201 != '\0')) && (DAT_007252b6 == '\0')) {
      sound_update_clock();
      bVar3 = 0x20 < (uint)(DAT_0072520c - iVar1);
      if (bVar3) {
        (**(code **)(DAT_00725208 + 0x10))();
      }
      if ((DAT_00725202 == '\0') && (bVar3)) {
        uVar2 = __ftol();
        sound_class_update_gain_fade(uVar2);
        sound_update_listener();
        FUN_0054d270();
        FUN_0054bd60();
        FUN_0054c020();
        sound_update_active_instances();
        DAT_00725214 = DAT_00725214 == '\0';
        iVar1 = DAT_0072520c;
        uVar2 = DAT_00725210;
      }
      DAT_00725210 = uVar2;
      DAT_0072520c = iVar1;
      if (bVar3) {
        (**(code **)(DAT_00725208 + 0x14))();
      }
    }
    if (DAT_00725202 == '\0') {
      *(int *)(DAT_006ac530 + 0x30) = *(int *)(DAT_006ac530 + 0x30) + 1;
    }
  }
  return;
}

Disassembly (0x549810..0x549960, capstone; phase-4 review):

0x549810: push ecx
0x549811: mov al, byte ptr [0x7252b6]
0x549816: test al, al
0x549818: jne 0x54995e
0x54981e: mov eax, dword ptr [0x6f1d6c]
0x549823: mov cl, byte ptr [eax + 2]
0x549826: test cl, cl
0x549828: push ebx
0x549829: mov ebx, 1
0x54982e: jne 0x54986f
0x549830: mov al, byte ptr [0x6b7020]
0x549835: test al, al
0x549837: je 0x549842
0x549839: mov eax, dword ptr [0x6f1d20]
0x54983e: test eax, eax
0x549840: je 0x54986f
0x549842: mov al, byte ptr [0x725202]
0x549847: test al, al
0x549849: je 0x54988d
0x54984b: mov eax, dword ptr [0x725208]
0x549850: test eax, eax
0x549852: mov byte ptr [0x725202], 0
0x549859: je 0x549863
0x54985b: push 0
0x54985d: call dword ptr [eax + 0x28]
0x549860: add esp, 4
0x549863: call 0x449210
0x549868: mov dword ptr [0x72520c], eax
0x54986d: jmp 0x54988d
0x54986f: cmp byte ptr [0x725202], bl
0x549875: je 0x54988d
0x549877: mov eax, dword ptr [0x725208]
0x54987c: test eax, eax
0x54987e: mov byte ptr [0x725202], bl
0x549884: je 0x54988d
0x549886: push ebx
0x549887: call dword ptr [eax + 0x28]
0x54988a: add esp, 4
0x54988d: mov al, byte ptr [0x725200]
0x549892: test al, al
0x549894: je 0x54994c
0x54989a: mov al, byte ptr [0x725201]
0x54989f: test al, al
0x5498a1: je 0x54994c
0x5498a7: mov al, byte ptr [0x7252b6]
0x5498ac: test al, al
0x5498ae: jne 0x54994c
0x5498b4: mov ecx, dword ptr [0x725210]
0x5498ba: push esi
0x5498bb: mov esi, dword ptr [0x72520c]
0x5498c1: mov dword ptr [esp + 8], ecx
0x5498c5: call 0x54ae60
0x5498ca: mov edx, dword ptr [0x72520c]
0x5498d0: sub edx, esi
0x5498d2: cmp edx, 0x21
0x5498d5: jb 0x5498e1
0x5498d7: mov eax, dword ptr [0x725208]
0x5498dc: call dword ptr [eax + 0x10]
0x5498df: jmp 0x5498e3
0x5498e1: xor bl, bl
0x5498e3: mov al, byte ptr [0x725202]
0x5498e8: test al, al
0x5498ea: jne 0x54992f
0x5498ec: test bl, bl
0x5498ee: je 0x54992f
0x5498f0: fld dword ptr [0x725210]
0x5498f6: call 0x6391b4
0x5498fb: push eax
0x5498fc: call 0x545330
0x549901: add esp, 4
0x549904: call 0x54b970
0x549909: call 0x54d270
0x54990e: call 0x54bd60
0x549913: call 0x54c020
0x549918: call 0x54c900
0x54991d: mov al, byte ptr [0x725214]
0x549922: test al, al
0x549924: sete cl
0x549927: mov byte ptr [0x725214], cl
0x54992d: jmp 0x54993f
0x54992f: mov edx, dword ptr [esp + 8]
0x549933: mov dword ptr [0x72520c], esi
0x549939: mov dword ptr [0x725210], edx
0x54993f: test bl, bl
0x549941: pop esi
0x549942: je 0x54994c
0x549944: mov eax, dword ptr [0x725208]
0x549949: call dword ptr [eax + 0x14]
0x54994c: mov al, byte ptr [0x725202]
0x549951: test al, al
0x549953: pop ebx
0x549954: jne 0x54995e
0x549956: mov eax, dword ptr [0x6ac530]
0x54995b: inc dword ptr [eax + 0x30]
0x54995e: pop ecx
0x54995f: ret 
#endif
