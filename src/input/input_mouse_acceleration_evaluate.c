// input_mouse_acceleration_evaluate  (Ghidra: FUN_0048cb60; renamed per its behavior)
// address 0x48cb60, size 306 bytes
// name confidence: 0.45   rewrite confidence: 0.6
// evidence: matches types/input.h's mouse_acceleration_point documentation exactly ("When
//   mouse_acceleration ... differs from the cached value ... the defaults are copied over the
//   working table and every working magnitude is rebuilt as magnitude_slow - (magnitude_slow -
//   magnitude_fast) * acceleration. A delta m then finds the first point i >= 1 with magnitude
//   >= m and returns (lerp(rate[i-1], rate[i]) * (sensitivity * boost[i] + 1)) * m."). __ftol is
//   the float-to-int truncation the working-magnitude rebuild loop uses; Ghidra dropped its
//   floating point input operand, reconstructed here from the header's formula.
// register convention: sensitivity and magnitude as two ordinary stack parameters (matches the
//   two-float/int cdecl-style shape of every other evaluator in this file).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern float mouse_acceleration;                                  // 0x006894d0
extern float mouse_acceleration_cached;                            // 0x0068e418
extern mouse_acceleration_point mouse_acceleration_defaults[k_input_mouse_acceleration_point_count]; // 0x0068e41c
extern mouse_acceleration_point mouse_acceleration_points[k_input_mouse_acceleration_point_count];   // 0x0068e48c

// Evaluates the mouse-sensitivity/acceleration response curve for raw movement magnitude
// `magnitude`, given `sensitivity` (typically settings.mouse_look_x/y_sensitivity). Rebuilds
// mouse_acceleration_points from mouse_acceleration_defaults whenever the (0..1 clamped, and
// clamped in place) mouse_acceleration setting has changed since the last call. Returns 0 for a
// zero magnitude or one at or beyond the last point's magnitude (in practice unreachable, since
// the last default point's magnitude is a 10000 sentinel).
float input_mouse_acceleration_evaluate(float sensitivity, int32_t magnitude)
{
    int32_t i;
    float fraction;
    float rate;

    if (magnitude == 0) {
        return 0.0f;
    }

    if (mouse_acceleration != mouse_acceleration_cached) {
        if (mouse_acceleration < 0.0f) {
            mouse_acceleration = 0.0f;
        } else if (1.0f < mouse_acceleration) {
            mouse_acceleration = 1.0f;
        }
        mouse_acceleration_cached = mouse_acceleration;

        memcpy(mouse_acceleration_points, mouse_acceleration_defaults, sizeof(mouse_acceleration_points));
        for (i = 0; i < k_input_mouse_acceleration_point_count; i++) {
            mouse_acceleration_points[i].magnitude = (int32_t)(
                (float)mouse_acceleration_defaults[i].magnitude_slow -
                (float)(mouse_acceleration_defaults[i].magnitude_slow - mouse_acceleration_defaults[i].magnitude) *
                    mouse_acceleration_cached);
        }
    }

    for (i = 1; i < k_input_mouse_acceleration_point_count; i++) {
        if (magnitude <= mouse_acceleration_points[i].magnitude) {
            break;
        }
    }
    if (i >= k_input_mouse_acceleration_point_count) {
        return 0.0f;
    }

    fraction = (float)(magnitude - mouse_acceleration_points[i - 1].magnitude) /
               (float)(mouse_acceleration_points[i].magnitude - mouse_acceleration_points[i - 1].magnitude);
    rate = mouse_acceleration_points[i - 1].rate +
           (mouse_acceleration_points[i].rate - mouse_acceleration_points[i - 1].rate) * fraction;

    return (sensitivity * mouse_acceleration_points[i].boost + 1.0f) * rate * (float)magnitude;
}

#if 0
Original Ghidra decompilation (0x48cb60), from tools/pack.py 0x48cb60:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_0048cb60(float param_1,int param_2)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  int *piVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  float10 fVar7;
  float10 extraout_ST0;

  fVar7 = (float10)0.0;
  if (param_2 != 0) {
    if (DAT_006894d0 != _DAT_0068e418) {
      if (0.0 <= DAT_006894d0) {
        if (1.0 < DAT_006894d0) {
          DAT_006894d0 = 1.0;
        }
      }
      else {
        DAT_006894d0 = 0.0;
      }
      _DAT_0068e418 = DAT_006894d0;
      puVar5 = &DAT_0068e41c;
      puVar6 = &DAT_0068e48c;
      for (iVar3 = 0x1c; iVar3 != 0; iVar3 = iVar3 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
      puVar5 = &DAT_0068e420;
      do {
        uVar1 = __ftol();
        puVar5[0x1b] = uVar1;
        puVar5 = puVar5 + 4;
        fVar7 = extraout_ST0;
      } while ((int)puVar5 < 0x68e490);
    }
    iVar3 = 1;
    piVar4 = &DAT_0068e49c;
    while (*piVar4 < param_2) {
      piVar4 = piVar4 + 4;
      iVar3 = iVar3 + 1;
      if (0x68e4fb < (int)piVar4) {
        return fVar7;
      }
    }
    iVar2 = iVar3 * 0x10;
    fVar7 = ((float10)param_1 * (float10)*(float *)(iVar2 + 0x68e498) + (float10)1.0) *
            (((float10)*(float *)(iVar2 + 0x68e494) - (float10)*(float *)(iVar2 + 0x68e484)) *
             ((float10)(param_2 - *(int *)(iVar2 + 0x68e47c)) /
             (float10)((&DAT_0068e48c)[iVar3 * 4] - *(int *)(iVar2 + 0x68e47c))) +
            (float10)*(float *)(iVar2 + 0x68e484)) * (float10)param_2;
  }
  return fVar7;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
