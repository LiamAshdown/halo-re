// random_int_range  (Ghidra: random_int_range, already named)
// address 0x405320, size 49 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// evidence: out/phase4/math_functions.md; same LCG step as random_real_range @0x401050
//   (types/math.h random_seed section: seed = seed*k_random_multiplier+k_random_increment,
//   fraction taken from the high 16 bits of the updated seed). Already declared/called as
//   `random_int_range(int16_t min, int16_t max)` with the ECX->min/stack->max mapping in
//   src/units/unit_update_stance_and_jump.c (with an explicit UNSURE note there about the
//   same register split documented below); this file supplies the definition that extern
//   points at. A second, older extern in src/ai/actor_squad_action_execute.c models the
//   call sites there as a single `exclusive_max` stack argument with ECX/min left at its
//   caller's (unmodeled) live value -- consistent with this file's signature, just a
//   partial view of it from a caller this session did not re-derive.
// register convention, confirmed against objdump: `mov eax,[esp+0x4]` sign-extends a stack
//   short into `max`; `movsx edx,cx` reads only the low 16 bits of ECX for the subtraction
//   (`max - (short)min`), but the final `add eax,ecx` adds the *full* 32-bit ECX, not the
//   truncated copy. Declaring the parameter as int16_t reproduces both uses identically
//   under normal C integer promotion (the sign-extended 16-bit value IS the full register
//   contents for any well-formed 16-bit argument), matching the sibling declaration in
//   unit_update_stance_and_jump.c.
//   // blam-cc: ECX -> min, stack -> max
// review fix (phase 4 math gate): the first draft multiplied and shifted as int32_t, which
//   is an arithmetic shift; the binary uses shr (logical). Now done in uint32_t.
// UNSURE: if a caller ever left garbage in the upper 16 bits of ECX (rather than a proper
//   sign-extended 16-bit value), the disassembly's raw `add eax,ecx` would differ from this
//   int16_t parameter's promoted value; no such caller is evidenced in this module's slice.

#include "tags.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern random_seed random_seed_global; // 0x00719cd0

// Returns a pseudo-random int in the closed-open interval [min, max) using the engine's
// global LCG seed (max on the stack, min in ECX -- see the register-convention note above).
int32_t random_int_range(int16_t min, int16_t max)
{
    int32_t range;

    random_seed_global = random_seed_global * k_random_multiplier + k_random_increment;
    range = (int32_t)max - (int32_t)min;
    // `imul eax,edx` then `shr eax,0x10`: the product is shifted as an UNSIGNED 32-bit value
    // (logical shift), so it is computed in uint32_t here. A signed multiply + arithmetic shift
    // would sign-fill whenever range * (seed >> 16) reaches 2^31 (any range above 0x8000 can)
    // and for every max < min call.
    return (int32_t)(((uint32_t)range * (random_seed_global >> k_random_value_shift)) >> 16) + min;
}

#if 0
Original Ghidra decompilation (0x405320):

int random_int_range(short param_1)

{
  int in_ECX;

  DAT_00719cd0 = DAT_00719cd0 * 0x19660d + 0x3c6ef35f;
  return (((int)param_1 - (int)(short)in_ECX) * (DAT_00719cd0 >> 0x10) >> 0x10) + in_ECX;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
