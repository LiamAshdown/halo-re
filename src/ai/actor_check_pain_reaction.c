// actor_check_pain_reaction  (Ghidra: actor_check_pain_reaction, renamed)
// address 0x40de20, size 75 bytes
// name confidence: 0.3   rewrite confidence: 0.85 (VERIFIED against objdump 0x40de20..0x40de6a (build order: EAX target, EDX passed through as use_alt_base, stack (actor, order code, 0, 0, &order); on success actor_set_mode(actor, 4, &order) and AL 1))
// evidence: phase-4 summary "checks a pain/reaction condition and transitions the actor
// into the associated mode if satisfied."
// register convention: no parameters visible in the decompiled C; actor_build_order_grenade_or_melee and
// actor_set_mode are both called with zero visible arguments, so the actor_index this
// function operates on is presumably a register the caller already set (as with the other
// zero-argument helpers in this module).
//   // blam-cc: ECX -> order_code, EDX -> use_alt_base, ESI -> actor_index, stack ->
//   resolved_target
// FIXED (register inputs, objdump): resolved disassembly-verified per-argument roles for both
// calls (0x40de20..0x40de54). actor_build_order_grenade_or_melee (0x403630) is called with
// EAX = this function's own stack parameter reloaded (resolved_target), DL = this function's
// live-in EDX unchanged (use_alt_base), and 5 stack args in its documented order: ESI
// (actor_index), ECX (order_code), 0 (byte_a), 0 (byte_b), and a pointer into this function's
// own 0x84-byte local buffer (order). actor_set_mode is then called with that same ESI
// (actor_index), mode=4 (push 0x4 at 0x40de4c, not 0 as the old placeholder had it), and the
// same local buffer pointer as mode_data (not NULL). ECX, EDX and ESI were previously modeled
// as a single unmapped stack parameter that the old body passed, wrongly, as if it were
// actor_index to a zero-argument callee.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern int32_t actor_build_order_grenade_or_melee(uint32_t resolved_target, uint8_t use_alt_base,
    uint32_t actor_index, uint16_t order_code, uint8_t byte_a, uint8_t byte_b, uint16_t *order); // 0x403630, this module
extern void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data); // 0x40d8d0, this module

// VERIFIED against disassembly 0x40de20..0x40de6a (2026-09-30): EAX = the stack argument (resolved target), EDX passed through,
//   the six/five argument build call, actor_set_mode(actor, 4, buffer) and the 0/1 return match. The in-game crash recorded in
//   known_bad is inside actor_build_order_grenade_or_melee (0x403630), not in this wrapper.
// blam-cc: ECX -> order_code, EDX -> use_alt_base, ESI -> actor_index, stack -> resolved_target
uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base,
    uint16_t order_code, datum_index actor_index)
{
    uint16_t order_buffer[66]; // matches the 0x84-byte local reservation at 0x40de20

    if (actor_build_order_grenade_or_melee(resolved_target, use_alt_base, actor_index, order_code,
            0, 0, order_buffer) != 0) {
        actor_set_mode(actor_index, 4, order_buffer);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40de20):

undefined4 FUN_0040de20(void)

{
  char cVar1;

  cVar1 = FUN_00403630();
  if (cVar1 != '\0') {
    actor_set_mode();
    return 1;
  }
  return 0;
}
#endif
