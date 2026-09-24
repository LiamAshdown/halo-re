// cinematic_screen_effect_get_script_value  (Ghidra: FUN_005121a0; new name, evidence below)
// address 0x5121a0, size 37 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// evidence: types/render.h cinematic_screen_effect_globals.script_values[4] at +0x64
//   ("1.0 after a reset; 0x5121a0 reads them"); matches script_screen_effect_set_value 0x4810f0's
//   writer per the same field comment.
// register convention: EAX = index (int16, in_AX), no other arguments.
//   // blam-cc: EAX -> index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "render.h"

extern cinematic_screen_effect_globals *cinematic_screen_effect_state; // 0x0071cfc4 (named _state;
                                             // a variable cannot share the typedef's own name in C)

// Returns cinematic_screen_effect_globals.script_values[index], or 0.0 if there is no active
// effect block or the index is out of range.
float cinematic_screen_effect_get_script_value(int16_t index) // blam-cc: EAX -> index
{
    cinematic_screen_effect_globals *g = cinematic_screen_effect_state;

    if (g != 0 && index >= 0 && index < 4) {
        return g->script_values[index];
    }
    return 0.0f;
}

#if 0
Original Ghidra decompilation (0x5121a0):

float10 FUN_005121a0(void)

{
  short in_AX;
  float10 fVar1;

  fVar1 = (float10)0.0;
  if (((DAT_0071cfc4 != 0) && (-1 < in_AX)) && (in_AX < 4)) {
    fVar1 = (float10)*(float *)(DAT_0071cfc4 + 100 + in_AX * 4);
  }
  return fVar1;
}
#endif
