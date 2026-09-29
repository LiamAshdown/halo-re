// game_engine_compute_time_scale  (Ghidra: FUN_00461550; renamed per its summary)
// address 0x461550, size 192 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Computes a time-scaling multiplier from the game
// speed option and the active game variant's callback overrides"); types/game.h
// game_variant::speed_scale (+0x54, "clamped to 0.25 .. 4.0"), game_engine_definition::
// time_scale_override (+0x88).
// register convention: two handles in EDX and ESI (in_EDX, unaff_ESI), both only ever tested
// against -1; their identity could not be recovered from this decompilation.
//   // blam-cc: EDX -> param_a, unaff_ESI -> param_b
// UNSURE: the vtable slot at +0x88 is invoked twice in a row with no visible arguments (matching
// Ghidra's own rendering), the first time gating a 1.5x slow-down and the second a 0.5x
// speed-up; both preserved exactly as separate calls rather than merged into one, since nothing
// here proves the callback is idempotent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern game_variant game_engine_variant;            // 0x006f1c88 (speed_scale aliased 0x006f1cdc)

// blam-cc: EDX -> param_a, unaff_ESI -> param_b
float game_engine_compute_time_scale(int32_t param_a, int32_t param_b)
{
    float scale = 1.0f;

    if (current_game_engine != 0) {
        float speed = game_engine_variant.health;
        if (speed < 0.25f) {
            speed = 0.25f;
        } else if (4.0f < speed) {
            speed = 4.0f;
        }
        scale = 1.0f / speed;
    }

    if (param_a != -1 && param_b != -1 && current_game_engine != 0) {
        if (current_game_engine->time_scale_override != 0) {
            char slow = ((char (*)(void))current_game_engine->time_scale_override)(); // UNSURE: real args
            if (slow != 0) {
                scale = scale * 1.5f;
            }
        }
        if (current_game_engine != 0 && current_game_engine->time_scale_override != 0) {
            char fast = ((char (*)(void))current_game_engine->time_scale_override)(); // UNSURE: real args
            if (fast != 0) {
                return scale * 0.5f;
            }
        }
    }
    return scale;
}

#if 0
Original Ghidra decompilation (0x461550), from tools/pack.py 0x461550:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

float10 FUN_00461550(void)

{
  char cVar1;
  int in_EDX;
  int unaff_ESI;
  float10 fVar2;
  undefined4 local_4;

  fVar2 = (float10)1.0;
  if (DAT_006f1d20 != 0) {
    if (0.25 <= _DAT_006f1cdc) {
      if (_DAT_006f1cdc <= 4.0) {
        fVar2 = (float10)_DAT_006f1cdc;
      }
      else {
        fVar2 = (float10)4.0;
      }
    }
    else {
      fVar2 = (float10)0.25;
    }
    fVar2 = (float10)1.0 / fVar2;
  }
  local_4 = (float)fVar2;
  if (((in_EDX != -1) && (unaff_ESI != -1)) && (DAT_006f1d20 != 0)) {
    if (*(code **)(DAT_006f1d20 + 0x88) != (code *)0x0) {
      cVar1 = (**(code **)(DAT_006f1d20 + 0x88))();
      fVar2 = (float10)local_4;
      if (cVar1 != '\0') {
        fVar2 = fVar2 * (float10)1.5;
        local_4 = (float)fVar2;
      }
    }
    if ((DAT_006f1d20 != 0) && (*(code **)(DAT_006f1d20 + 0x88) != (code *)0x0)) {
      cVar1 = (**(code **)(DAT_006f1d20 + 0x88))();
      if (cVar1 != '\0') {
        return (float10)local_4 * (float10)0.5;
      }
      fVar2 = (float10)local_4;
    }
  }
  return fVar2;
}
#endif
