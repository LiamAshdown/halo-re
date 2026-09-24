// render_rasterizer_dispatch_537800  (Ghidra: FUN_00512190; new name, evidence below)
// address 0x512190, size 15 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// evidence: phase4 one-liner: "Thin wrapper that unconditionally calls another (unnamed)
//   rasterizer function; purpose is only inferable from the callee, which is not in this batch."
//   0x537800 is outside this module's range and has not been named or rewritten yet.
// register convention: unknown; forwarded through unchanged (no register is touched between
//   entry and the tail call).
// UNSURE: this is a pure pass-through to 0x537800; its own purpose cannot be determined without
//   that callee's evidence.

#include "tags.h"

extern void rasterizer_lens_flare_occlusion_test_issue(void); // 0x537800, not yet rewritten (outside this module's range)

void render_rasterizer_dispatch_537800(void)
{
    rasterizer_lens_flare_occlusion_test_issue();
}

#if 0
Original Ghidra decompilation (0x512190):

void FUN_00512190(void)

{
  FUN_00537800();
  return;
}
#endif
