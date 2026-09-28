// input_key_block_timers_expire  (Ghidra: FUN_00490ca0)
// address 0x490ca0, size 108 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/input_types_notes.md names this "input_key_block_timers_expire 0x490ca0
// (pop: ...)" -- see the key_block_timer struct notes: "0x490ca0 expires entries" once their
// deadline passes.
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern key_block_timer key_block_timers[k_input_key_block_timer_count]; // 0x006b1600
extern int64_t performance_frequency;                                   // 0x006ac8f8/0x006ac8fc
extern int32_t __stdcall QueryPerformanceCounter(large_integer *counter);         // 0x0063a0ac IAT

// Frees any key_block_timer entry whose deadline has passed.
void input_key_block_timers_expire(void)
{
    int32_t i;
    large_integer counter;
    uint32_t now;

    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (key_block_timers[i].deadline != 0xffffffff) {
            QueryPerformanceCounter(&counter);
            now = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
            if (key_block_timers[i].deadline <= now) {
                key_block_timers[i].deadline = 0xffffffff;
                key_block_timers[i].key = -1;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x490ca0):

void FUN_00490ca0(void)

{
  uint uVar1;
  uint *puVar2;
  undefined8 uVar3;
  LARGE_INTEGER local_8;

  puVar2 = &DAT_006b1600;
  do {
    if ((puVar2 != (uint *)0x0) && (*puVar2 != 0xffffffff)) {
      QueryPerformanceCounter(&local_8);
      uVar3 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
      uVar1 = __alldiv(uVar3,DAT_006ac8f8,DAT_006ac8fc);
      if (*puVar2 <= uVar1) {
        *puVar2 = 0xffffffff;
        *(undefined2 *)(puVar2 + 1) = 0xffff;
      }
    }
    puVar2 = puVar2 + 2;
  } while ((int)puVar2 < 0x6b1620);
  return;
}
#endif
