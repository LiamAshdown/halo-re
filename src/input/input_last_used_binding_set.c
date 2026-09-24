// input_last_used_binding_set  (Ghidra: FUN_00490050)
// address 0x490050, size 57 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/input_functions.md summary "Low-level setter that writes a control's
// 'last used binding' record fields directly."; 0x007127d4 is
// input_globals.last_used_bindings[0] (0x00710328 + 0x24ac), a
// control_binding_descriptor per input_action (types/saved_games.h); the sibling accessor
// 0x48be50 (out of this batch's range) is documented as "Directly sets or clears the cached
// 'last used input device' record for a given game control."
// register convention: action index in AX (in_AX), device_type in CX (in_CX), device_index in
// DX (in_DX), then input_kind/input_index/direction on the stack (param_1/param_2/param_3)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

extern input_abstraction_globals input_globals; // 0x00710328

// blam-cc: action index in EAX, device_type in ECX, device_index in EDX, input_kind/input_index/
// direction on the stack
// Writes every field of last_used_bindings[action] directly, when action is a valid input
// action index (0 .. k_input_action_count - 1).
void input_last_used_binding_set(int16_t action, int16_t device_type, int16_t device_index,
                                  int16_t input_kind, int16_t input_index, int32_t direction)
{
    control_binding_descriptor *binding;

    if (action >= 0 && action < k_input_action_count) {
        binding = &input_globals.last_used_bindings[action];
        binding->device_type = device_type;
        binding->device_index = device_index;
        binding->input_kind = input_kind;
        binding->input_index = input_index;
        binding->direction = direction;
    }
}

#if 0
Original Ghidra decompilation (0x490050):

void FUN_00490050(undefined2 param_1,undefined2 param_2,undefined4 param_3)

{
  int iVar1;
  short in_AX;
  undefined2 in_CX;
  undefined2 in_DX;

  if ((-1 < in_AX) && (in_AX < 0x1b)) {
    iVar1 = in_AX * 0xc;
    *(undefined2 *)(&DAT_007127d4 + iVar1) = in_CX;
    *(undefined2 *)(&DAT_007127d6 + iVar1) = in_DX;
    *(undefined2 *)(&DAT_007127d8 + iVar1) = param_1;
    *(undefined2 *)(&DAT_007127da + iVar1) = param_2;
    *(undefined4 *)(&DAT_007127dc + iVar1) = param_3;
  }
  return;
}
#endif
