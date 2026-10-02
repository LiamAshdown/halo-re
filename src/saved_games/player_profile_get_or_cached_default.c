// player_profile_get_or_cached_default  (Ghidra: FUN_00539bc0, renamed)
// address 0x539bc0, size 39 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: out/phase4/saved_games_functions.md summary "Returns the cached default profile
// data directly, or delegates to player_profile_get for a real profile index." Disassembly at
// 0x539bc0 confirms out_buffer in EAX and index in ECX (both forwarded to player_profile_get's
// own (index stack, out_buffer ECX) convention on the delegate path), and that the fast path's
// return value is `xor al,al` executed unconditionally at entry, so it is always 0/false --
// Ghidra's `(uint)in_EAX & 0xffffff00` is the same fact (the low byte forced to 0) expressed as
// a masked pointer, not a real return value the original C likely intended to carry meaning in.
// register convention: out_buffer in EAX, index in ECX.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern saved_player_profile default_profile_data; // 0x0071d280

extern uint8_t player_profile_get(int32_t index, saved_player_profile *out_buffer); // 0x53a770

// blam-cc: out_buffer in EAX, index in ECX
uint8_t player_profile_get_or_cached_default(saved_player_profile *out_buffer, int32_t index)
{
    if (index == -1) {
        *out_buffer = default_profile_data;
        return 0;
    }
    return player_profile_get(index, out_buffer);
}

#if 0
Original Ghidra decompilation (0x539bc0):

uint FUN_00539bc0(void)

{
  undefined4 *in_EAX;
  uint uVar1;
  int in_ECX;
  int iVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;

  if (in_ECX == -1) {
    puVar3 = &DAT_0071d280;
    puVar4 = in_EAX;
    for (iVar2 = 0x7ff; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar4 = *puVar3;
      puVar3 = puVar3 + 1;
      puVar4 = puVar4 + 1;
    }
    return (uint)in_EAX & 0xffffff00;
  }
  uVar1 = player_profile_get();
  return uVar1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
