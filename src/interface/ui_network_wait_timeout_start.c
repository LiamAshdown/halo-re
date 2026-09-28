// ui_network_wait_timeout_start  (Ghidra: FUN_0049c810, unnamed; named per functions.md)
// address 0x49c810, size 81 bytes
// name confidence: 0.45   rewrite confidence: 0.55
// evidence: functions.md: "Starts (or confirms already started) the timestamp used by
// ui_network_wait_timeout_check, and marks a network/UI wait as active." types/interface.h names
// 0x006927c4 as ui_network_wait_start_time and 0x00718fcd as ui_network_wait_active.
// register convention: none (void).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t ui_network_wait_start_time; // 0x006927c4, -1 when no wait is running
extern int64_t performance_frequency;      // 0x006ac8f8/0x006ac8fc
extern uint8_t ui_network_wait_active;     // 0x00718fcd


// If no wait is currently timed, samples the performance counter and converts it to milliseconds
// as the new wait start time. Always marks the wait as active.
void ui_network_wait_timeout_start(void)
{
    if (ui_network_wait_start_time == -1) {
        large_integer counter;

        QueryPerformanceCounter((LARGE_INTEGER *)&counter);
        ui_network_wait_start_time = (int32_t)((counter.quad_part * 1000) / performance_frequency);
    }
    ui_network_wait_active = 1;
}

#if 0
Original Ghidra decompilation (0x49c810):

void FUN_0049c810(void)

{
  undefined8 uVar1;
  LARGE_INTEGER local_8;

  if (DAT_006927c4 == -1) {
    QueryPerformanceCounter(&local_8);
    uVar1 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    DAT_006927c4 = __alldiv(uVar1,DAT_006ac8f8,DAT_006ac8fc);
  }
  DAT_00718fcd = 1;
  return;
}
#endif
