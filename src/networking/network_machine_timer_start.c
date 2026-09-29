// network_machine_timer_start  (Ghidra: FUN_004df090; named per this rewrite)
// address 0x4df090, size 75 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: out/phase4/networking_functions.md: "Starts a timer on the object at unaff_ESI:
// records the current time, marks it active, and computes its expiry as now plus the given
// duration." Fields +0x10/+0x14/+0x18 match network_machine's unknown_10/timer_14/timer_18
// exactly (same object network_machine_reset.c clears).
// register convention: machine in ESI (unaff_ESI). blam-cc: ESI -> machine, stack -> duration_ms

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc

// blam-cc: ESI -> machine
void network_machine_timer_start(network_machine *machine, int32_t duration_ms)
{
    large_integer counter;
    int32_t now_ms;

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    machine->timer_14 = now_ms;
    machine->disconnect_timer_active = 1;
    machine->timer_18 = now_ms + duration_ms;
}

#if 0
Original Ghidra decompilation (0x4df090):

void FUN_004df090(int param_1)

{
  int iVar1;
  int unaff_ESI;
  undefined8 uVar2;
  LARGE_INTEGER local_8;

  QueryPerformanceCounter(&local_8);
  uVar2 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  iVar1 = __alldiv(uVar2,DAT_006ac8f8,DAT_006ac8fc);
  *(int *)(unaff_ESI + 0x14) = iVar1;
  *(undefined1 *)(unaff_ESI + 0x10) = 1;
  *(int *)(unaff_ESI + 0x18) = iVar1 + param_1;
  return;
}
#endif
