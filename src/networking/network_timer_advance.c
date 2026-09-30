// network_timer_advance  (Ghidra: FUN_004deb50; named per this rewrite)
// address 0x4deb50, size 93 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md: "Advances a millisecond timer pair
// (unaff_ESI[0..1]) using the high-resolution performance counter, subtracting elapsed time
// from a remaining-time field." A small, generic 2-dword [remaining_ms, last_tick_ms] record,
// used by network_timer_start.c (0x4debf0) and this batch's other timer helpers; not tied to any
// header-declared struct.
// register convention: timer in ESI (unaff_ESI). blam-cc: ESI -> timer

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "fn_networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc


// blam-cc: ESI -> timer
// Advances timer->last_tick_ms to now and, if time has actually elapsed since the previous
// tick, subtracts that elapsed time from timer->remaining_ms (floored at 0).
void network_timer_advance(network_timer_pair *timer)
{
    large_integer counter;
    int32_t now_ms;
    int32_t previous_tick_ms;
    int32_t elapsed;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    previous_tick_ms = timer->last_tick_ms;
    timer->last_tick_ms = now_ms;
    if (previous_tick_ms < now_ms) {
        elapsed = now_ms - previous_tick_ms;
        if (elapsed < timer->remaining_ms) {
            timer->remaining_ms = timer->remaining_ms - elapsed;
        } else {
            timer->remaining_ms = 0;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4deb50):

void FUN_004deb50(void)

{
  int iVar1;
  int iVar2;
  int *unaff_ESI;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar2 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  iVar1 = unaff_ESI[1];
  unaff_ESI[1] = iVar2;
  if (iVar1 < iVar2) {
    iVar2 = iVar2 - iVar1;
    if (iVar2 < *unaff_ESI) {
      *unaff_ESI = *unaff_ESI - iVar2;
      return;
    }
    *unaff_ESI = 0;
  }
  return;
}
#endif
