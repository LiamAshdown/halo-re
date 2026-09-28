// network_timer_start  (Ghidra: FUN_004debf0; named per this rewrite)
// address 0x4debf0, size 68 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md: "Initialises a timer pair pointed to by
// unaff_ESI with the current performance-counter time and the requested duration parameter."
// Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
// register convention: timer in ESI (unaff_ESI). blam-cc: ESI -> timer, stack -> duration_ms

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc


// blam-cc: ESI -> timer
void network_timer_start(network_timer_pair *timer, int32_t duration_ms)
{
    large_integer counter;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    timer->remaining_ms = duration_ms;
    timer->last_tick_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
}

#if 0
Original Ghidra decompilation (0x4debf0):

void FUN_004debf0(undefined4 param_1)

{
  undefined4 uVar1;
  undefined4 *unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  uVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *unaff_ESI = param_1;
  unaff_ESI[1] = uVar1;
  return;
}
#endif
