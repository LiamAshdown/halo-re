// actor_order_code_is_grenade_throw  (Ghidra: actor_order_code_is_grenade_throw, already named)
// address 0x404340, size 21 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// evidence: types/ai.h actor_order_code (_actor_order_code_grenade_first/_last = 9..12).
// register convention: order code in AX (in_AX, the sole parameter).
//   // blam-cc: AX -> order_code

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

// Returns whether order_code falls in the grenade-throw range 9..12 inclusive.
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t actor_order_code_is_grenade_throw(int16_t order_code)
{
    if (order_code > _actor_order_code_grenade_first - 1 && order_code < _actor_order_code_grenade_last + 1) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x404340):

undefined4 actor_order_code_is_grenade_throw(void)

{
  short in_AX;

  if ((8 < in_AX) && (in_AX < 0xd)) {
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
