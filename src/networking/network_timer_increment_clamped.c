// network_timer_increment_clamped  (Ghidra: FUN_004debb0; named per this rewrite)
// address 0x4debb0, size 32 bytes
// name confidence: 0.35   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md: "Register-based helper that advances the shared
// timer via network_timer_advance then adds an increment to *in_EAX, clamping the result to an upper
// bound." Uses the same [remaining_ms, last_tick_ms] layout as network_timer_advance.c; `timer`
// is passed straight through to that call.
// register convention: timer in EAX (in_EAX), increment in EDI (unaff_EDI), upper bound in EBX
// (unaff_EBX). blam-cc: EAX -> timer, EBX -> upper_bound, EDI -> increment

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_timer_advance(network_timer_pair *timer); // 0x4deb50, this batch

// blam-cc: EAX -> timer, EBX -> upper_bound, EDI -> increment
void network_timer_increment_clamped(network_timer_pair *timer, int32_t upper_bound, int32_t increment)
{
    int32_t sum;

    network_timer_advance(timer);
    sum = timer->remaining_ms + increment;
    if (sum < increment) {
        timer->remaining_ms = upper_bound;
        return;
    }
    timer->remaining_ms = sum;
    if (upper_bound < sum) {
        sum = upper_bound;
    }
    timer->remaining_ms = sum;
}

#if 0
Original Ghidra decompilation (0x4debb0):

void FUN_004debb0(void)

{
  int *in_EAX;
  int iVar1;
  int unaff_EBX;
  int unaff_EDI;

  FUN_004deb50();
  iVar1 = *in_EAX + unaff_EDI;
  if (iVar1 < unaff_EDI) {
    *in_EAX = unaff_EBX;
    return;
  }
  *in_EAX = iVar1;
  if (unaff_EBX < iVar1) {
    iVar1 = unaff_EBX;
  }
  *in_EAX = iVar1;
  return;
}
#endif
