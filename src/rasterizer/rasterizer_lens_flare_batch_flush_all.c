// rasterizer_lens_flare_batch_flush_all  (Ghidra: rasterizer_lens_flare_batch_flush_all, already named)
// address 0x536c80, size 46 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: functions.md summary ("Flushes (draws) every non-empty batch slot of the screen-space
//   sprite rendering system, typically once per frame"); loop bound 0x007d7038 is exactly
//   k_lens_flare_batch_slots (5) slots past 0x00746fc0.
// register convention: none -- __cdecl, no arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "fn_rasterizer.h"

extern lens_flare_batch lens_flare_batches[k_lens_flare_batch_slots]; // 0x00746fc0


// Flushes (draws) every non-empty batch slot of the screen-space sprite rendering system,
// typically once per frame.
void rasterizer_lens_flare_batch_flush_all(void)
{
    int i;
    for (i = 0; i < k_lens_flare_batch_slots; i++) {
        if (lens_flare_batches[i].vertex_count != 0) {
            rasterizer_lens_flare_batch_draw_slot(i);
        }
    }
}

#if 0
Original Ghidra decompilation (0x536c80):

void __cdecl rasterizer_lens_flare_batch_flush_all(void)

{
  int *piVar1;

  piVar1 = &DAT_0075efc0;
  do {
    if (*piVar1 != 0) {
      FUN_00536c10();
    }
    piVar1 = piVar1 + 0x6006;
  } while ((int)piVar1 < 0x7d7038);
  return;
}
#endif
