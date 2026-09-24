// input_key_block_timer_set  (Ghidra: FUN_00490bf0)
// address 0x490bf0, size 161 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/input_types_notes.md names this "key block timers (a key reads as up
// until the deadline), not key repeat", matching types/input.h's key_block_timer struct
// (deadline uint32 free==0xffffffff, key int16 free==-1) and system_keys[3]/system_key_states[3]
// (0x0068e40c / input_globals.system_key_states).
// register convention: key in EDI (unaff_DI); duration_ms as the one stack parameter

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern key_block_timer key_block_timers[k_input_key_block_timer_count]; // 0x006b1600
extern int16_t system_keys[k_input_system_key_count];                   // 0x0068e40c
extern input_abstraction_globals input_globals;                         // 0x00710328
extern int64_t performance_frequency;                                   // 0x006ac8f8/0x006ac8fc
extern int32_t QueryPerformanceCounter(large_integer *counter);         // 0x0063a0ac IAT

// blam-cc: key in EDI, duration_ms on the stack
// Schedules key to read as up (via input_get_key_state) for duration_ms milliseconds: reuses a
// free key_block_timer entry, or the one with the earliest deadline if none is free. If key is
// one of the three system keys (grave/escape/print screen), also clears its cached
// system_key_states hold count.
void input_key_block_timer_set(int16_t key, int32_t duration_ms)
{
    key_block_timer *chosen;
    int32_t i;
    large_integer counter;
    uint32_t now;

    chosen = (key_block_timer *)0;
    for (i = 0; i < k_input_key_block_timer_count; i++) {
        if (key_block_timers[i].key == -1) {
            chosen = &key_block_timers[i];
            break;
        }
        if (chosen == (key_block_timer *)0 || key_block_timers[i].deadline < chosen->deadline) {
            chosen = &key_block_timers[i];
        }
    }

    if (chosen != (key_block_timer *)0) {
        QueryPerformanceCounter(&counter);
        now = (uint32_t)((counter.quad_part * 1000) / performance_frequency);
        chosen->deadline = now + duration_ms;
        chosen->key = key;

        for (i = 0; i < k_input_system_key_count; i++) {
            if (system_keys[i] == key) {
                input_globals.system_key_states[i] = 0;
                return;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x490bf0):

void FUN_00490bf0(int param_1)

{
  uint *puVar1;
  int iVar2;
  uint *puVar3;
  short unaff_DI;
  undefined8 uVar4;
  LARGE_INTEGER local_8;

  puVar3 = (uint *)0x0;
  iVar2 = 0;
  puVar1 = &DAT_006b1600;
  do {
    if ((short)puVar1[1] == -1) {
      puVar3 = &DAT_006b1600 + iVar2 * 2;
      break;
    }
    if ((puVar3 == (uint *)0x0) || (*puVar1 < *puVar3)) {
      puVar3 = puVar1;
    }
    puVar1 = puVar1 + 2;
    iVar2 = iVar2 + 1;
  } while ((int)puVar1 < 0x6b1620);
  if (puVar3 != (uint *)0x0) {
    QueryPerformanceCounter(&local_8);
    uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar2 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
    *puVar3 = iVar2 + param_1;
    *(short *)(puVar3 + 1) = unaff_DI;
    iVar2 = 0;
    while ((&DAT_0068e40c)[iVar2] != unaff_DI) {
      iVar2 = iVar2 + 1;
      if (2 < iVar2) {
        return;
      }
    }
    (&DAT_007127d0)[iVar2] = 0;
  }
  return;
}
#endif
