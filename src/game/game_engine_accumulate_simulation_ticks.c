// game_engine_accumulate_simulation_ticks  (Ghidra: FUN_00470b30; renamed, no established name)
// address 0x470b30, size 189 bytes
// name confidence: 0.35   rewrite confidence: 0.55
// evidence: out/phase4/game_functions.md ("Implements a fixed-30Hz-timestep accumulator,
// converting a frame's elapsed time into a whole number of simulation ticks while carrying the
// fractional remainder forward"); types/game.h game_time_globals (speed +0x18, leftover_time
// +0x1c); network_game_mode (0x00719720, 1 client / 2 host forced to speed 1.0);
// src/game/game_engine_update_netgame_equipment.c and random_advance_draws.c for the established
// `__ftol` convention; camera_observer_target_direction.c for the established pattern of
// externing a bare CRT call by address instead of including <math.h> (which would collide with
// this module's own "math.h").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "fn_game.h"

extern game_time_globals *game_time; // 0x006f1d6c
extern int16_t network_game_mode;     // 0x00719720

extern double floor(double x); // 0x623e40, MSVC CRT
// (__ftol: the cast below) // 0x6391b4, MSVC runtime; UNSURE: real argument is on the x87 stack

// Converts `elapsed_seconds` into a whole number of fixed 30Hz simulation ticks, using
// game_time->speed as the time scale outside a networked client/host connection (where the scale
// is forced to 1.0), and carries the fractional tick remainder forward in game_time->leftover_time
// unless `keep_remainder` is set. Returns the tick count.
int32_t game_engine_accumulate_simulation_ticks(float elapsed_seconds, char keep_remainder)
{
    float scale;
    double floor_result;
    int32_t tick_count;

    if (network_game_mode == 1 || network_game_mode == 2) {
        scale = 1.0f;
    } else {
        scale = game_time->speed;
    }
    scale = scale * 30.0f;

    elapsed_seconds = elapsed_seconds + game_time->leftover_time;
    floor_result = (float)floor((double)(elapsed_seconds * scale)); // 0x470b72: fst to a float slot
    // 0x470b7c..0x470b9a: __ftol of the floored value, clamped to at most 1000
    tick_count = (int32_t)((float)floor_result > 1000.0f ? 1000.0f : (float)floor_result);

    if (0.0f < scale && keep_remainder == 0) {
        elapsed_seconds = elapsed_seconds - (float)floor_result / scale;
        game_time->leftover_time = elapsed_seconds;
        if (elapsed_seconds < 0.0f) {
            game_time->leftover_time = 0.0f;
        }
    }
    return tick_count;
}

#if 0
Original Ghidra decompilation (0x470b30), from tools/pack.py 0x470b30:

undefined4 FUN_00470b30(float param_1,char param_2)

{
  float fVar1;
  int iVar2;
  undefined4 uVar3;
  float10 fVar4;

  if ((DAT_00719720 == 1) || (DAT_00719720 == 2)) {
    fVar1 = 1.0;
  }
  else {
    fVar1 = *(float *)(DAT_006f1d6c + 0x18);
  }
  fVar1 = fVar1 * 30.0;
  param_1 = param_1 + *(float *)(DAT_006f1d6c + 0x1c);
  fVar4 = (float10)FUN_00623e40((double)(param_1 * fVar1));
  uVar3 = __ftol();
  iVar2 = DAT_006f1d6c;
  if ((0.0 < fVar1) && (param_2 == '\0')) {
    param_1 = param_1 - (float)fVar4 / fVar1;
    *(float *)(DAT_006f1d6c + 0x1c) = param_1;
    if (param_1 < 0.0) {
      *(undefined4 *)(iVar2 + 0x1c) = 0;
      return uVar3;
    }
  }
  return uVar3;
}
#endif
