// render_rasterizer_dispatch_537800  (Ghidra: FUN_00512190; new name, evidence below)
// address 0x512190, size 15 bytes
// name confidence: 0.2   rewrite confidence: 0.9
// evidence: phase4 one-liner: "Thin wrapper that unconditionally calls another (unnamed)
//   rasterizer function; purpose is only inferable from the callee, which is not in this batch."
//   0x537800 is outside this module's range and has not been named or rewritten yet.
// register convention: ECX -> arg_ecx, EDI -> arg_edi; both forwarded through unchanged (no
//   register is touched between entry and the tail call).
//   // blam-cc: ECX -> arg_ecx, EDI -> arg_edi
// FIXED (register inputs, objdump): ECX (pushed at 0x512195) and EDI (read by the tail call
// itself at 0x512196) are both live-in and forwarded unchanged to 0x537800; only EDI was
// modeled before. Neither register's meaning is recoverable without that callee's own
// evidence (outside this module's range, not yet rewritten), so both are kept as opaque
// forwarded values.
// UNSURE: this is a pure pass-through to 0x537800; its own purpose cannot be determined without
//   that callee's evidence.

#include "tags.h"

extern void rasterizer_lens_flare_occlusion_test_issue(uint32_t arg_ecx, uint32_t arg_edi); // 0x537800, not yet rewritten (outside this module's range); blam-cc: ECX -> arg_ecx, EDI -> arg_edi

// blam-cc: ECX -> arg_ecx, EDI -> arg_edi
void render_rasterizer_dispatch_537800(uint32_t arg_ecx, uint32_t arg_edi)
{
    rasterizer_lens_flare_occlusion_test_issue(arg_ecx, arg_edi);
}

#if 0
Original Ghidra decompilation (0x512190):

void FUN_00512190(void)

{
  FUN_00537800();
  return;
}
#endif
