// input_time_base_resync  (Ghidra: input_time_base_resync, already named)
// address 0x48b470, size 64 bytes
// name confidence: 0.55   rewrite confidence: 0.9
// evidence: identical QueryPerformanceCounter -> milliseconds computation as
//   input_state_initialize 0x48b3e0, storing only to input_abstraction_globals::time_base
//   (0x00712538) without touching the rest of the block.
// register convention: no parameters, no return value.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern input_abstraction_globals input_globals; // 0x00710328
extern int64_t performance_frequency;                         // 0x006ac8f8/0x006ac8fc

// Recomputes the millisecond input time base from QueryPerformanceCounter without touching the
// rest of the input abstraction state.
void input_time_base_resync(void)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    input_globals.time_base = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
}

#if 0
Original Ghidra decompilation (0x48b470), from tools/pack.py 0x48b470:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_time_base_resync(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_00712538 = __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
