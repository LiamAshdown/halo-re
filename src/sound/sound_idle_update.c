// sound_idle_update  (Ghidra: FUN_00549960)
// address 0x549960, size 151 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md summary "Lightweight per-tick sound update (clock +
//   instance gain pass only) used while waiting for the device to resume."; types/sound.h
//   documents sound_idle_update_active (0x00725203) as "reentrancy guard of 0x549960", i.e. this
//   exact address; otherwise a trimmed-down copy of src/sound/sound_update.c's cadence/rollback
//   shape, omitting the gain-fade/listener/looping/channel-assignment calls. Called from
//   src/sound/sound_fade_out_and_stop_all.c's wait loop.
// register convention: plain __cdecl, no parameters.
// blam-cc: (no arguments)
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "sound.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t sound_initialized;    // 0x00725200
extern uint8_t sound_idle_update_active; // 0x00725203
extern uint8_t sound_enabled;        // 0x00725201
extern uint8_t sound_disabled;       // 0x007252b6
extern float sound_time_delta;       // 0x00725210
extern int32_t sound_time;           // 0x0072520c
extern sound_driver *current_sound_driver; // 0x00725208
extern uint8_t sound_paused;         // 0x00725202
extern struct cache *sound_cache;    // 0x006ac530

extern void sound_update_clock(void); // 0x54ae60
extern void sound_update_active_instances(void); // 0x54c900

// Reentrant-guarded lightweight tick: advances the sound clock and, once per
// k_sound_update_interval_ms while not paused, refreshes every active instance's gain between the
// driver's begin_frame/end_frame (rolling the clock back if paused by the time it matters, exactly
// as src/sound/sound_update.c does), then ages the sound cache.
void sound_idle_update(void)
{
    float saved_delta = sound_time_delta;
    int32_t saved_time = sound_time;

    sound_idle_update_active = 1;

    if (sound_initialized != 0 && sound_enabled != 0 && sound_disabled == 0) {
        uint8_t due;

        sound_update_clock();
        due = (uint32_t)(sound_time - saved_time) > 0x20;

        if (due) {
            current_sound_driver->begin_frame();
        }

        if (sound_paused == 0 && due) {
            sound_update_active_instances();
            saved_time = sound_time;
            saved_delta = sound_time_delta;
        }

        sound_time_delta = saved_delta;
        sound_time = saved_time;

        if (due) {
            current_sound_driver->end_frame();
        }
    }

    sound_cache->age += 1;
    sound_idle_update_active = 0;
}

#if 0
Original Ghidra decompilation (0x549960):

void FUN_00549960(void)

{
  int iVar1;
  undefined4 uVar2;
  bool bVar3;

  uVar2 = DAT_00725210;
  iVar1 = DAT_0072520c;
  DAT_00725203 = 1;
  if (((DAT_00725200 != '\0') && (DAT_00725201 != '\0')) && (DAT_007252b6 == '\0')) {
    sound_update_clock();
    bVar3 = 0x20 < (uint)(DAT_0072520c - iVar1);
    if (bVar3) {
      (**(code **)(DAT_00725208 + 0x10))();
    }
    if ((DAT_00725202 == '\0') && (bVar3)) {
      sound_update_active_instances();
      iVar1 = DAT_0072520c;
      uVar2 = DAT_00725210;
    }
    DAT_00725210 = uVar2;
    DAT_0072520c = iVar1;
    if (bVar3) {
      (**(code **)(DAT_00725208 + 0x14))();
    }
  }
  *(int *)(DAT_006ac530 + 0x30) = *(int *)(DAT_006ac530 + 0x30) + 1;
  DAT_00725203 = 0;
  return;
}

Disassembly (0x549960..0x5499f7, capstone; phase-4 review):

0x549960: push ecx
0x549961: mov al, byte ptr [0x725200]
0x549966: test al, al
0x549968: push ebx
0x549969: mov bl, 1
0x54996b: mov byte ptr [0x725203], bl
0x549971: je 0x5499e5
0x549973: mov al, byte ptr [0x725201]
0x549978: test al, al
0x54997a: je 0x5499e5
0x54997c: mov al, byte ptr [0x7252b6]
0x549981: test al, al
0x549983: jne 0x5499e5
0x549985: mov eax, dword ptr [0x725210]
0x54998a: push esi
0x54998b: mov esi, dword ptr [0x72520c]
0x549991: mov dword ptr [esp + 8], eax
0x549995: call 0x54ae60
0x54999a: mov ecx, dword ptr [0x72520c]
0x5499a0: sub ecx, esi
0x5499a2: cmp ecx, 0x21
0x5499a5: jb 0x5499b2
0x5499a7: mov edx, dword ptr [0x725208]
0x5499ad: call dword ptr [edx + 0x10]
0x5499b0: jmp 0x5499b4
0x5499b2: xor bl, bl
0x5499b4: mov al, byte ptr [0x725202]
0x5499b9: test al, al
0x5499bb: jne 0x5499c8
0x5499bd: test bl, bl
0x5499bf: je 0x5499c8
0x5499c1: call 0x54c900
0x5499c6: jmp 0x5499d7
0x5499c8: mov eax, dword ptr [esp + 8]
0x5499cc: mov dword ptr [0x72520c], esi
0x5499d2: mov dword ptr [0x725210], eax
0x5499d7: test bl, bl
0x5499d9: pop esi
0x5499da: je 0x5499e5
0x5499dc: mov ecx, dword ptr [0x725208]
0x5499e2: call dword ptr [ecx + 0x14]
0x5499e5: mov eax, dword ptr [0x6ac530]
0x5499ea: inc dword ptr [eax + 0x30]
0x5499ed: mov byte ptr [0x725203], 0
0x5499f4: pop ebx
0x5499f5: pop ecx
0x5499f6: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
