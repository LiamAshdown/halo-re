// input_bind_capture_reset  (Ghidra: input_bind_capture_reset, already named)
// address 0x48b5f0, size 83 bytes
// name confidence: 0.55   rewrite confidence: 0.85
// evidence: clears exactly the same 0x28-byte local_player_input_state[0] block
//   (0x00712498..0x007124bc) as input_update_tick's keyboard-capture case, plus
//   system_key_states/pad_24ab (0x007127d0..0x007127d3) and sets idle. types/input.h documents
//   idle as set by both this function and input_update_tick.
// register convention: no parameters, no return value.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"
#include <string.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern input_abstraction_globals input_globals; // 0x00710328

// Clears the local player's current-frame input accumulator (action state 0) and the cached
// system-key hold states, and marks the frame idle so the bind-capture UI starts from a clean
// state.
void input_bind_capture_reset(void)
{
    memset(&input_globals.states[0], 0, sizeof(input_globals.states[0]));

    input_globals.system_key_states[0] = 0;
    input_globals.system_key_states[1] = 0;
    input_globals.system_key_states[2] = 0;
    input_globals.pad_24ab = 0; // 0x007127d3, covered by the same dword clear
                                            // as system_key_states in the binary; the binary
                                            // also re-clears the last two of these four bytes
                                            // a second time (redundant), UNSURE why

    input_globals.idle = 1;
}

#if 0
Original Ghidra decompilation (0x48b5f0), from tools/pack.py 0x48b5f0:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void input_bind_capture_reset(void)

{
  DAT_00712498 = 0;
  DAT_0071249c = 0;
  _DAT_007124a0 = 0;
  _DAT_007124a4 = 0;
  _DAT_007124a8 = 0;
  _DAT_007124ac = 0;
  _DAT_007124b0 = 0;
  _DAT_007124b4 = 0;
  _DAT_007127d0 = 0;
  _DAT_007124b8 = 0;
  DAT_00712540 = 1;
  DAT_007127d2 = 0;
  _DAT_007124bc = 0;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
