// network_random_offset  (Ghidra: FUN_004403b0, still unnamed -> renamed)
// address 0x4403b0, size 99 bytes
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md summary ("lazily seeds the C runtime random
// number generator from the current time, then returns a random value offset by a
// caller-supplied base held in ESI"); `python tools/pack.py 0x4403b0`.
// register convention: base offset in ESI (unaff_ESI), no stack arguments.
// UNSURE: `__ftol(iVar2)` is preserved as decompiled, with `rand()`'s int result passed
// straight through it. The real MSVC `__ftol` helper truncates the float already sitting on
// the x87 stack and normally shows with zero visible arguments (see
// src/math/random_seed_generate.c and src/ai/actor_reseed_movement_pause_timer.c for that
// pattern in this codebase); here Ghidra attributed it a visible int argument instead, which
// suggests an intermediate int-to-double-to-int round trip that optimized away. The net
// effect (rand() truncated back to an int) is preserved either way.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t network_random_seeded; // 0x006f0ca8, one-time seed flag

extern int32_t __ftol(int32_t value);      // 0x006391b4, see UNSURE note above

// blam-cc: base offset in ESI (unaff_ESI)
int32_t network_random_offset(int32_t base)
{
    int32_t value;

    if (network_random_seeded == 0) {
        srand((uint32_t)_time32(0));
        network_random_seeded = 1;
    }
    value = rand();
    value = __ftol(value);
    return value + base;
}

#if 0
Original Ghidra decompilation (0x4403b0):

int FUN_004403b0(void)

{
  __time32_t _Var1;
  int iVar2;
  int unaff_ESI;

  if (DAT_006f0ca8 == '\0') {
    _Var1 = FID_conflict___time32((__time32_t *)0x0);
    FUN_006240c2(_Var1);
    DAT_006f0ca8 = '\x01';
  }
  iVar2 = _rand();
  iVar2 = __ftol(iVar2);
  return iVar2 + unaff_ESI;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
