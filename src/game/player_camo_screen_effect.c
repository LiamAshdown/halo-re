// player_camo_screen_effect  (Ghidra: player_camo_screen_effect, already named)
// address 0x47c310, size 45 bytes
// name confidence: 0.6   rewrite confidence: 0.45
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x47c2d0
//   --stop-address=0x47c340): identical shape to the sibling player_examine_nearby_vehicle.c
//   (this batch) -- EAX is the calling hs_function_definition*, ECX is "first", `index`/
//   `thread_index` are stack parameters. Falls straight through from an unlisted sibling entry
//   at 0x47c300 that resolves the same definition from a function_index stack parameter (the
//   "player_set_action_result-shaped" variant of this same builtin); that entry is outside this
//   batch's address range and not one of the 64 targets, so only this one (the
//   already-resolved-EAX variant) is rewritten here.
// register convention: the hs_function_definition* dispatching this evaluate call in EAX
//   (in_EAX), the "first call" flag in ECX (in_ECX); `index` and `thread_index` are this
//   function's own two stack parameters (`index` is never read).
//   // blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
// UNSURE: unit_build_seat_occupant_zone_list's real signature and effect (elided by Ghidra beyond its one ECX
//   argument and EAX return, both inferred from the tail-call into hs_thread_return).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include "fn_hs.h"

extern int32_t unit_build_seat_occupant_zone_list(int32_t object_handle); // 0x56bbd0, not in this batch;
    // blam-cc: ECX -> object_handle, returns in EAX; UNSURE exact effect


// blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
// Evaluates this builtin's one object argument; once ready, forwards it to unit_build_seat_occupant_zone_list and
// returns its result from the HS thread.
void player_camo_screen_effect(int16_t index, uint32_t thread_index, hs_function_definition *definition,
    char first)
{
    int32_t *args = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);
    (void)index;

    if (args != 0) {
        int32_t result = unit_build_seat_occupant_zone_list(args[0]);
        hs_thread_return(result, thread_index);
    }
}

#if 0
Original Ghidra decompilation (0x47c310), from tools/pack.py 0x47c310:

void player_camo_screen_effect(undefined4 param_1,undefined4 param_2)

{
  int in_EAX;
  int iVar1;

  iVar1 = hs_evaluate_typed_arguments(param_2,(int)*(short *)(in_EAX + 0x1a),in_EAX + 0x1c);
  if (iVar1 != 0) {
    FUN_0056bbd0();
    hs_thread_return();
    return;
  }
  return;
}
#endif
