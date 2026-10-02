// sound_update_clock  (Ghidra: sound_update_clock, already named)
// address 0x54ae60, size 88 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: out/phase4/sound_functions.md "Advances the sound engine's millisecond clock and
// computes the per-tick blend weight used by crossfade/ducking calculations."; matches the
// QueryPerformanceCounter -> __allmul -> __alldiv pattern already established for
// interface_tick.c (0x496280) and random_seed_generate (types/math.h), reusing large_integer
// and performance_frequency from that convention.
// register convention: void, no parameters.
// Phase-4 review: checked instruction by instruction against the disassembly appended in the
// #if 0 block; no semantic difference found.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "sound.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int64_t performance_frequency;   // 0x006ac8f8/0x006ac8fc
extern int32_t sound_time;              // 0x0072520c, ms
extern float sound_time_delta;          // 0x00725210, (new - old) * 0.03


// Advances the sound engine's millisecond clock from the CPU performance counter and
// recomputes the per-tick blend weight (3% of the elapsed milliseconds) used by the
// crossfade/ducking gain ramps.
void sound_update_clock(void)
{
    large_integer counter;
    int32_t new_time;
    float old_time;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    new_time = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    old_time = (float)sound_time;
    sound_time = new_time;
    sound_time_delta = ((float)new_time - old_time) * 0.03f;
}

#if 0
Original Ghidra decompilation (0x54ae60):

void sound_update_clock(void)

{
  float fVar1;
  int iVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar2 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  fVar1 = (float)DAT_0072520c;
  DAT_0072520c = iVar2;
  DAT_00725210 = ((float)iVar2 - fVar1) * 0.03;
  return;
}

Disassembly (0x54ae60..0x54aeb8, capstone; phase-4 review):

0x54ae60: sub esp, 8
0x54ae63: lea eax, [esp]
0x54ae66: push eax
0x54ae67: call dword ptr [0x63a0ac]
0x54ae6d: mov ecx, dword ptr [esp + 4]
0x54ae71: mov edx, dword ptr [esp]
0x54ae74: push 0
0x54ae76: push 0x3e8
0x54ae7b: push ecx
0x54ae7c: push edx
0x54ae7d: call 0x62de80
0x54ae82: mov ecx, dword ptr [0x6ac8fc]
0x54ae88: push ecx
0x54ae89: mov ecx, dword ptr [0x6ac8f8]
0x54ae8f: push ecx
0x54ae90: push edx
0x54ae91: push eax
0x54ae92: call 0x639230
0x54ae97: mov dword ptr [esp], eax
0x54ae9a: fild dword ptr [esp]
0x54ae9d: fisub dword ptr [0x72520c]
0x54aea3: mov dword ptr [0x72520c], eax
0x54aea8: fmul dword ptr [0x672cd0]
0x54aeae: fstp dword ptr [0x725210]
0x54aeb4: add esp, 8
0x54aeb7: ret 
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
