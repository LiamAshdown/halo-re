// game_timer_reset  (Ghidra: game_timer_reset, already named)
// address 0x4c9f30, size 86 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: matches the given name exactly; every field is main_globals (types/main.h):
// frame_counter_low/high (0x000/0x004), render_counter_low/high (0x010/0x014), frame_time_ms
// (0x008). performance_counter_frequency (0x006ac8f8) reuses
// src/game/camera_debug_start.c-adjacent naming already used elsewhere in this codebase for the
// same global (math module).
// register convention: cdecl, no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "main.h"

extern main_globals main_globals_data; // 0x00719700
extern int64_t performance_counter_frequency; // 0x006ac8f8, foreign (math module)


// Re-baselines the frame-timing globals (frame and render counters, plus frame_time_ms) to the
// current high-resolution timestamp.
void game_timer_reset(void)
{
    int64_t counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    main_globals_data.frame_counter_low = (uint32_t)counter;
    main_globals_data.frame_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.render_counter_low = (uint32_t)counter;
    main_globals_data.render_counter_high = (uint32_t)(counter >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((counter * 1000) / performance_counter_frequency);
}

#if 0
Original Ghidra decompilation (0x4c9f30):

void __cdecl game_timer_reset(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  DAT_00719700 = local_8.s.LowPart;
  DAT_00719704 = local_8.s.HighPart;
  DAT_00719710 = local_8.s.LowPart;
  DAT_00719714 = local_8.s.HighPart;
  uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DAT_00719708 = __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  return;
}
#endif
