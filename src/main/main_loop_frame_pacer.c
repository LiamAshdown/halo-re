// main_loop_frame_pacer  (Ghidra: main_loop_frame_pacer, already named)
// address 0x4c9f90, size 519 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: matches the given name exactly. Every 0x00719700..0x0071971c field is main_globals
// (types/main.h): frame_counter_low/high (0x000/0x004), frame_time_overflow (0x018),
// frame_delta_time (0x01c), game_connection (0x020), movie_frame_bitmap (0x024),
// movie_frame_delta_time (0x034), frame_time_ms (0x008). game_time_force_single_tick
// (0x007196d8), unknown_006894ba (0x006894ba) and cinematic_globals (0x006f187c) reuse
// established names from types/main.h and other modules. performance_counter_frequency
// (0x006ac8f8) reuses this module's own game_timer_reset.c naming.
// register convention: cdecl, no parameters.
// phase 4 review (disassembly 0x4c9f90..0x4ca196): control flow and every comparison checked;
// the forced steps are the doubles 1/15 (0x00672d80) and 1/30 (0x00672d88), now written as such.
// UNSURE: 0x00710301 (gates a 15fps vs 30fps local-game delta clamp) has no established name
// anywhere; declared as an opaque byte.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "main.h"

extern main_globals main_globals_data; // 0x00719700
extern int32_t game_time_force_single_tick; // 0x007196d8, foreign (main-owned global per main.h)
extern uint8_t unknown_006894ba;       // 0x006894ba, foreign (interface module)
extern uint8_t *cinematic_globals;     // 0x006f187c, foreign, TYPES-GAP
extern int64_t performance_counter_frequency; // 0x006ac8f8, foreign (math module)
extern uint8_t unknown_00710301;       // TYPES-GAP, UNSURE identity


// Paces the main loop to roughly 30 FPS: while capturing isn't running and either the video
// options' frame limiter is on or a cinematic is active, busy-waits (sleeping 10ms at a time
// once more than 12ms remains until the 1/30s mark, otherwise polling with no sleep) until at
// least 1/30s has elapsed since the last frame. Then computes this frame's delta time: clamped
// to 0..1s normally, but forced to a fixed 1/15s or 1/30s step whenever it would otherwise run
// slower than that (skipped entirely while networked, movie-capturing, or during a cinematic).
// Finally re-baselines the frame counter and frame_time_ms from the current timestamp -- or, if
// a timedemo single-tick is queued, advances the counter by exactly one 1/30s step instead of
// reading the clock, and forces the delta to exactly 1/30s.
void main_loop_frame_pacer(void)
{
    uint8_t pacing;
    int64_t now;
    double elapsed_seconds;
    float delta;

    pacing = (game_time_force_single_tick == 0 &&
              (unknown_006894ba != 0 || cinematic_globals[9] != 0))
                 ? 1
                 : 0;

    do {
        int64_t elapsed_ticks;
        uint32_t sleep_ms;

        QueryPerformanceCounter((LARGE_INTEGER *)&now);
        elapsed_ticks = now - (((int64_t)main_globals_data.frame_counter_high << 32) |
                                main_globals_data.frame_counter_low);
        elapsed_seconds = (double)elapsed_ticks / (double)performance_counter_frequency;

        sleep_ms = (!pacing || (0.03333333507180214 - elapsed_seconds <= 0.012)) ? 0 : 10;
        Sleep(sleep_ms);
    } while (pacing && elapsed_seconds < 0.03333333507180214);

    delta = main_globals_data.movie_frame_delta_time;
    if (main_globals_data.movie_frame_bitmap == 0) {
        main_globals_data.frame_time_overflow = elapsed_seconds > 1.0;
        if (elapsed_seconds >= 0.0) {
            if (elapsed_seconds > 1.0) {
                elapsed_seconds = 1.0;
            }
        } else {
            elapsed_seconds = 0.0;
        }

        if (main_globals_data.game_connection != 0 || cinematic_globals[9] != 0) {
            goto apply;
        }
        if (unknown_00710301 == 0) {
            if (elapsed_seconds <= 0.06666666666666667) {
                goto apply;
            }
            elapsed_seconds = 0.06666666666666667;   // double at 0x00672d80
            goto apply;
        } else {
            if (elapsed_seconds <= 0.03333333333333333) {
                goto apply;
            }
            elapsed_seconds = 0.03333333333333333;   // double at 0x00672d88
            goto apply;
        }
    }
    elapsed_seconds = (double)delta;

apply:
    if (game_time_force_single_tick != 0) {
        int64_t frequency;
        int64_t step;
        int64_t new_counter;

        QueryPerformanceFrequency((LARGE_INTEGER *)&frequency);
        step = frequency / 30;
        new_counter = step + (((int64_t)main_globals_data.frame_counter_high << 32) |
                               main_globals_data.frame_counter_low);
        main_globals_data.frame_counter_low = (uint32_t)new_counter;
        main_globals_data.frame_counter_high = (uint32_t)(new_counter >> 32);
        main_globals_data.frame_delta_time = 0.033333335f;
        main_globals_data.frame_time_ms = main_globals_data.frame_time_ms + 0x21;
        return;
    }

    main_globals_data.frame_counter_low = (uint32_t)now;
    main_globals_data.frame_counter_high = (uint32_t)(now >> 32);
    main_globals_data.frame_time_ms = (uint32_t)((now * 1000) / performance_counter_frequency);
    main_globals_data.frame_delta_time = (float)elapsed_seconds;
}

#if 0
Original Ghidra decompilation (0x4c9f90):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __cdecl main_loop_frame_pacer(void)

{
  bool bVar1;
  float fVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  longlong lVar5;
  undefined8 uVar6;
  DWORD dwMilliseconds;
  double local_1c;
  LARGE_INTEGER local_14;
  LARGE_INTEGER local_c;

  if ((DAT_007196d8 == 0) && ((DAT_006894ba != '\0' || (*(char *)(DAT_006f187c + 9) != '\0')))) {
    bVar1 = true;
  }
  else {
    bVar1 = false;
  }
  do {
    QueryPerformanceCounter(&local_14);
    uVar4 = local_14.s.HighPart;
    uVar3 = local_14.s.LowPart;
    local_c.s.LowPart = local_14.s.LowPart - DAT_00719700;
    local_c.s.HighPart =
         (local_14.s.HighPart - DAT_00719704) - (uint)(local_14.s.LowPart < DAT_00719700);
    local_1c = (double)CONCAT44(local_c.s.HighPart,local_c.s.LowPart) /
               (double)CONCAT44(DAT_006ac8fc,DAT_006ac8f8);
    if ((!bVar1) || (0.03333333507180214 - local_1c <= 0.012)) {
      dwMilliseconds = 0;
    }
    else {
      dwMilliseconds = 10;
    }
    Sleep(dwMilliseconds);
  } while ((bVar1) && (local_1c < 0.03333333507180214));
  fVar2 = _DAT_00719734;
  if (DAT_00719724 == 0) {
    DAT_00719718 = 1.0 < local_1c;
    if (0.0 <= local_1c) {
      if (1.0 < local_1c) {
        local_1c = 1.0;
      }
    }
    else {
      local_1c = 0.0;
    }
    if ((DAT_00719720 != 0) || (*(char *)(DAT_006f187c + 9) != '\0')) goto LAB_004ca0ef;
    if (DAT_00710301 == '\0') {
      if (local_1c <= 0.06666666666666667) goto LAB_004ca0ef;
      fVar2 = 0.06666667;
    }
    else {
      if (local_1c <= 0.03333333333333333) goto LAB_004ca0ef;
      fVar2 = 0.033333335;
    }
  }
  local_1c = (double)fVar2;
LAB_004ca0ef:
  if (DAT_007196d8 != 0) {
    QueryPerformanceFrequency(&local_c);
    lVar5 = __alldiv(local_c.s.LowPart,local_c.s.HighPart,0x1e,0);
    lVar5 = lVar5 + CONCAT44(DAT_00719704,DAT_00719700);
    DAT_00719700 = (int)lVar5;
    DAT_00719704 = (int)((ulonglong)lVar5 >> 0x20);
    _DAT_0071971c = 0.033333335;
    DAT_00719708 = DAT_00719708 + 0x21;
    return;
  }
  DAT_00719700 = uVar3;
  DAT_00719704 = uVar4;
  uVar6 = __allmul(uVar3,uVar4,1000,0);
  DAT_00719708 = __alldiv(uVar6,DAT_006ac8f8,DAT_006ac8fc);
  _DAT_0071971c = (float)local_1c;
  return;
}
#endif
