// network_timer_decrement_floored  (Ghidra: FUN_004debd0; named per this rewrite)
// address 0x4debd0, size 28 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Register-based helper that advances the shared
// timer via network_timer_advance then decrements *in_EAX by unaff_EDI, floored at zero." Uses the same
// [remaining_ms, last_tick_ms] layout as network_timer_advance.c.
// register convention: timer in EAX (in_EAX), decrement in EDI (unaff_EDI). blam-cc: EAX ->
// timer, EDI -> decrement

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"


extern void network_timer_advance(network_timer_pair *timer); // 0x4deb50, this batch

// blam-cc: EAX -> timer, EDI -> decrement
void network_timer_decrement_floored(network_timer_pair *timer, int32_t decrement)
{
    network_timer_advance(timer);
    if (decrement < timer->remaining_ms) {
        timer->remaining_ms = timer->remaining_ms - decrement;
        return;
    }
    timer->remaining_ms = 0;
}

#if 0
Original Ghidra decompilation (0x4debd0):

void FUN_004debd0(void)

{
  int *in_EAX;
  int unaff_EDI;

  FUN_004deb50();
  if (unaff_EDI < *in_EAX) {
    *in_EAX = *in_EAX - unaff_EDI;
    return;
  }
  *in_EAX = 0;
  return;
}
#endif
