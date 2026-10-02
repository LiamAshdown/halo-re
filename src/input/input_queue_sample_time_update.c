// input_queue_sample_time_update  (Ghidra: FUN_00492210)
// address 0x492210, size 64 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: out/phase4/input_functions.md summary "Samples the high-resolution performance
// counter and stores the current time in milliseconds into a global timestamp used by the
// nearby input-queue routines."; types/input.h globals list: "global 0x00712c34: uint32_t
// input_queue_sample_time  milliseconds, written by 0x492210 (called by interface 0x498a3d);
// never read in the image." Same millisecond computation as input_time_base_resync 0x48b470.
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
extern uint32_t input_queue_sample_time;                      // 0x00712c34
extern int64_t performance_frequency;                          // 0x006ac8f8/0x006ac8fc

// Refreshes input_queue_sample_time from QueryPerformanceCounter. Nothing in this image reads
// the value back.
void input_queue_sample_time_update(void)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    input_queue_sample_time = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
}

#if 0
Original Ghidra decompilation (0x492210):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00492210(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_00712c34 = __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
