// vector3d_positive_modulo  (orphan pass 4: FUN_004588e0, no Ghidra name)
// address 0x4588e0, size 168 bytes
// name confidence: 0.35 (out/phase4/effects_types_notes.md calls this "adds signed noise to a
//   vector", but the actual arithmetic per-component is `fmod(v[i], period) + (v[i] < 0 ?
//   period : 0)` -- a positive-result modulo/wrap, the textbook fix for C's `fmod` returning a
//   negative remainder when its first argument is negative. No noise/RNG callee is present.)
// rewrite confidence: 0.45 (confirmed against objdump, including the x87-stack-only call to
//   `_CIfmod`, which Ghidra's own decompile showed with zero visible arguments)
// evidence: src/math/README.md, "Misattributed functions" #5: "0x628cca is `_CIfmod` (`x` in
//   `ST(1)`, `y` in `ST(0)`)". objdump confirms `fld [esi+N]` (x) then `fld [esp+8]` (period,
//   the incoming stack argument) immediately before each call, matching that operand order.
// register convention: ESI -> v, EDI -> out, stack -> period.
// blam-cc: ESI -> v, EDI -> out, stack -> period
// FIXED (register inputs, objdump): the notes described ESI/EDI in prose
// ("ESI = const real_vector3d *v ...") that the checker could not parse as a register mapping,
// so both v (ESI, read at 0x4588e1, fld [esi]) and out (EDI, read/written at 0x45890e,
// fstp [edi]) showed up as missing; rewritten in the plain "REG -> name" form.

#include "tags.h"
#include "math.h"

extern double fmod(double x, double y); // CRT fmod (0x628cca: _CIfmod, x87 fprem; name entry "fmod" at 0x006844f0)

void vector3d_positive_modulo(const real_vector3d *v, real_vector3d *out, float period)
{
    out->i = (float)fmod(v->i, period) + (v->i < 0.0f ? period : 0.0f);
    out->j = (float)fmod(v->j, period) + (v->j < 0.0f ? period : 0.0f);
    out->k = (float)fmod(v->k, period) + (v->k < 0.0f ? period : 0.0f);
}

#if 0
Original Ghidra decompilation (0x4588e0):

void FUN_004588e0(float param_1)

{
  float *unaff_ESI;
  float *unaff_EDI;
  float10 fVar1;
  float local_4;

  if (0.0 <= *unaff_ESI) {
    local_4 = 0.0;
  }
  else {
    local_4 = param_1;
  }
  fVar1 = (float10)FUN_00628cca();
  *unaff_EDI = (float)(fVar1 + (float10)local_4);
  if (0.0 <= unaff_ESI[1]) {
    local_4 = 0.0;
  }
  else {
    local_4 = param_1;
  }
  fVar1 = (float10)FUN_00628cca();
  unaff_EDI[1] = (float)(fVar1 + (float10)local_4);
  if (unaff_ESI[2] < 0.0) {
    fVar1 = (float10)FUN_00628cca();
    unaff_EDI[2] = (float)(fVar1 + (float10)param_1);
    return;
  }
  fVar1 = (float10)FUN_00628cca();
  unaff_EDI[2] = (float)(fVar1 + (float10)0.0);
  return;
}
#endif
