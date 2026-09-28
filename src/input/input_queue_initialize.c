// input_queue_initialize  (Ghidra: FUN_00492250)
// address 0x492250, size 94 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: types/input.h names this "input_queue_initialize 0x492250" and documents
// input_event_queue (0x10c bytes at 0x00712cc0, 0x43 dwords zeroed here); the store to
// DAT_00712cc0 = 1 last is input_event_queue_active.enabled.
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

extern input_event_queue input_event_queue_active;                    // 0x00712cc0
extern int64_t performance_frequency;                           // 0x006ac8f8/0x006ac8fc

// Zeroes the whole input event queue block, seeds its start_time and last_event_time from
// QueryPerformanceCounter, then enables it.
void input_queue_initialize(void)
{
    large_integer counter;
    uint32_t *cursor;
    int32_t count;

    cursor = (uint32_t *)&input_event_queue_active;
    for (count = 0x43; count != 0; count--) {
        *cursor = 0;
        cursor = cursor + 1;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    input_event_queue_active.last_event_time = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
    input_event_queue_active.start_time = input_event_queue_active.last_event_time;
    input_event_queue_active.enabled = 1;
}

#if 0
Original Ghidra decompilation (0x492250):

void FUN_00492250(void)

{
  int iVar1;
  undefined4 *puVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  puVar2 = (undefined4 *)&DAT_00712cc0;
  for (iVar1 = 0x43; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  QueryPerformanceCounter(&local_8);
  uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  DAT_00712cc4 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
  DAT_00712cc8 = DAT_00712cc4;
  DAT_00712cc0 = 1;
  return;
}
#endif
