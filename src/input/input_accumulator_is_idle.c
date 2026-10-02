// input_accumulator_is_idle  (Ghidra: FUN_0048fce0)
// address 0x48fce0, size 116 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/input_functions.md summary "Returns true when the input accumulator's
// axis values and press counters are unchanged from the previous snapshot (used to detect input
// idle state)."; types/game.h local_player_input_state matches every offset read (buttons[0x13]
// at +0x00, throttle_x/throttle_y/look_x/look_y at +0x14/+0x18/+0x1c/+0x20). The decompiled
// button loop only reads `current`'s bytes (never `previous`'s), so despite the summary saying
// "press counters ... unchanged from the previous snapshot", the actual condition is: the four
// axes are within 0.1 of `previous`'s, AND every one of `current`'s 19 button bytes is zero (no
// digital action currently held). Reproduced literally.
// register convention: current in ECX (in_ECX), previous in EDX (in_EDX)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern double fabs(double x); // FABS

// blam-cc: current in ECX, previous in EDX
// True when current's four axes are all within 0.1 of previous's and none of current's 19
// digital-button hold counts are nonzero.
uint8_t input_accumulator_is_idle(local_player_input_state *current, local_player_input_state *previous)
{
    uint8_t idle;
    int32_t i;

    idle = 1;
    // 0x48fce0..0x48fd32: each test is "fabs(d) < 0.1 (the double at 0x00672c00 is (double)0.1f)" and a false or
    // unordered compare (NaN) clears the result, so it is written as !(x < 0.1)
    if (!(fabs((double)(current->throttle_x - previous->throttle_x)) < (double)0.1f) ||
        !(fabs((double)(current->throttle_y - previous->throttle_y)) < (double)0.1f) ||
        !(fabs((double)(current->look_y - previous->look_y)) < (double)0.1f) ||
        !(fabs((double)(current->look_x - previous->look_x)) < (double)0.1f)) {
        idle = 0;
    }

    for (i = 0; i < k_input_digital_action_count; i++) {
        idle = (idle && current->buttons[i] == 0) ? 1 : 0;
    }
    return idle;
}

#if 0
Original Ghidra decompilation (0x48fce0):

void FUN_0048fce0(void)

{
  bool bVar1;
  char *in_ECX;
  int in_EDX;
  int iVar2;

  if ((((0.1 <= ABS(*(float *)(in_ECX + 0x14) - *(float *)(in_EDX + 0x14))) ||
       (0.1 <= ABS(*(float *)(in_ECX + 0x18) - *(float *)(in_EDX + 0x18)))) ||
      (0.1 <= ABS(*(float *)(in_ECX + 0x20) - *(float *)(in_EDX + 0x20)))) ||
     (0.1 <= ABS(*(float *)(in_ECX + 0x1c) - *(float *)(in_EDX + 0x1c)))) {
    bVar1 = false;
  }
  else {
    bVar1 = true;
  }
  iVar2 = 0x13;
  do {
    if ((bVar1) && (*in_ECX == '\0')) {
      bVar1 = true;
    }
    else {
      bVar1 = false;
    }
    in_ECX = in_ECX + 1;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
