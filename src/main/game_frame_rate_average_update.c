// game_frame_rate_average_update  (Ghidra: game_frame_rate_average_update, already named)
// address 0x4c6e80, size 164 bytes
// name confidence: 0.5   rewrite confidence: 0.8
// evidence: types/main.h main_frame_rate_average (sample_time_ms, history[0x10], count); the
// struct comment already documents this exact algorithm ("the update returns the mean of the
// first count entries, shifts them up one slot and grows count to 16"); src/math/random_seed_generate.c
// for the QueryPerformanceCounter/performance_frequency -> milliseconds idiom that replaces
// __allmul/__alldiv.
// register convention: __cdecl, no arguments.
// phase 4 review (disassembly 0x4c6e80..0x4c6f23: no drift (unsigned divide, 16 bit shift counter).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "main.h"
#include "fn_main.h"

extern main_frame_rate_average frame_rate_average_data; // 0x00719ab0

extern int64_t performance_frequency; // 0x006ac8f8, foreign (math module)

// Returns the mean of the first `count` recorded frame times (or 1 ms if none have been recorded
// yet), shifts entries 0..count-2 up one slot into 1..count-1 (making room for a new
// newest sample at index 0, which the caller is expected to fill in), grows count towards a cap
// of 16, and refreshes sample_time_ms to the current time in milliseconds.
uint32_t game_frame_rate_average_update(void)
{
    uint32_t average;
    int32_t sum;
    int32_t i;
    int64_t counter;

    if (frame_rate_average_data.count < 1) {
        average = 1;
    } else {
        sum = frame_rate_average_data.history[0];
        for (i = frame_rate_average_data.count - 1; i >= 1; i--) {
            sum = sum + frame_rate_average_data.history[i];
            frame_rate_average_data.history[i] = frame_rate_average_data.history[i - 1];
        }
        average = (uint32_t)sum / frame_rate_average_data.count;
    }

    if (frame_rate_average_data.count + 1 < 0x11) {
        frame_rate_average_data.count = frame_rate_average_data.count + 1;
    } else {
        frame_rate_average_data.count = 0x10;
    }

    QueryPerformanceCounter((LARGE_INTEGER *)&counter);
    frame_rate_average_data.sample_time_ms = (int32_t)((counter * 1000) / performance_frequency);

    return average;
}

#if 0
Original Ghidra decompilation (0x4c6e80):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint __cdecl game_frame_rate_average_update(void)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  short sVar4;
  uint uVar5;
  undefined8 uVar6;
  LARGE_INTEGER local_8;

  if ((int)DAT_00719af8 < 1) {
    uVar2 = 1;
  }
  else {
    sVar4 = (short)(DAT_00719af8 - 1);
    uVar2 = DAT_00719ab8;
    if (0 < sVar4) {
      piVar3 = (int *)(&DAT_00719ab8 + sVar4);
      uVar5 = DAT_00719af8 - 1 & 0xffff;
      do {
        uVar2 = uVar2 + *piVar3;
        *piVar3 = piVar3[-1];
        piVar3 = piVar3 + -1;
        uVar5 = uVar5 - 1;
      } while (uVar5 != 0);
    }
    uVar2 = uVar2 / DAT_00719af8;
  }
  iVar1 = DAT_00719af8 + 1;
  DAT_00719af8 = 0x10;
  if (iVar1 < 0x11) {
    DAT_00719af8 = iVar1;
  }
  QueryPerformanceCounter(&local_8);
  uVar6 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
  _DAT_00719ab0 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  return uVar2;
}
#endif
