// time_query_performance_counter_ms  (Ghidra: FUN_00449210; renamed here, see below)
// address 0x449210, size 59 bytes
// name confidence: 0.8   rewrite confidence: 0.85  (reviewed line by line against objdump)
// evidence: out/phase2/cseries/00.md; calls QueryPerformanceCounter, multiplies the 64-bit
// counter by 1000 with __allmul, then divides by the cached frequency (0x006ac8f8/0x006ac8fc)
// with __alldiv -- the standard QPC-to-milliseconds conversion. Ghidra drops the EAX:EDI return
// of the trailing __alldiv call, but every one of the 52 call sites in the image (see
// src/interface/chimera__do_show_loading_screen.c, src/interface/virtual_keyboard_draw_text.c,
// src/main/game_scenario_session_begin.c, src/main/main_loop.c and others) treats it as a
// uint32_t millisecond clock read with no arguments, which is the name used consistently by
// those other rewrites (`time_query_performance_counter_ms`); this file adopts that name as the
// canonical one for 0x449210 instead of leaving it FUN_00449210.
// register convention: __cdecl, no arguments; returns EDX:EAX (only EAX is ever consumed).
//
// __allmul/__alldiv are the MSVC 7.1 64-bit multiply/divide helpers; folded here into plain
// int64_t arithmetic with identical results, the same technique already used at every inlined
// call site of this same computation (e.g. src/math/random_seed_generate.c, which documents the
// equivalence) and by src/cache/sound_cache_touch.c and src/cache/texture_cache_get.c, which
// each carry their own inlined copy of this exact function's body.

#include "tags.h"
#include "math.h"
#include "cseries.h"

extern int QueryPerformanceCounter(large_integer *counter); // 0x0063a0ac import thunk
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, QueryPerformanceFrequency()
                                       // result, owned by this module (see types/cseries.h)

// Reads the high-resolution performance counter and converts it to milliseconds using the
// previously cached counter frequency.
uint32_t time_query_performance_counter_ms(void)
{
    large_integer counter;

    QueryPerformanceCounter(&counter);
    return (uint32_t)((counter.quad_part * 1000) / performance_frequency);
}

#if 0
Original Ghidra decompilation (0x449210), from tools/pack.py 0x449210:

void FUN_00449210(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe):
  449210: sub esp,0x8
  449213: lea eax,[esp]
  449216: push eax
  449217: call DWORD PTR ds:0x63a0ac      ; QueryPerformanceCounter(&local_8)
  44921d: mov ecx,DWORD PTR [esp+0x4]     ; local_8.HighPart
  449221: mov edx,DWORD PTR [esp]         ; local_8.LowPart
  449224: push 0x0
  449226: push 0x3e8                      ; 1000
  44922b: push ecx
  44922c: push edx
  44922d: call 0x62de80                   ; __allmul(low, high, 1000, 0) -> EDX:EAX
  449232: mov ecx,DWORD PTR ds:0x6ac8fc   ; frequency high
  449238: push ecx
  449239: mov ecx,DWORD PTR ds:0x6ac8f8   ; frequency low
  44923f: push ecx
  449240: push edx
  449241: push eax
  449242: call 0x639230                   ; __alldiv(product_low, product_high, freq_low, freq_high) -> EDX:EAX
  449247: add esp,0x8
  44924a: ret
#endif
