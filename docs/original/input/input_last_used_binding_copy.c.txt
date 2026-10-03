// input_last_used_binding_copy  (Ghidra: FUN_0048be50; renamed -- distinct from the already
//   rewritten sibling accessor input_last_used_binding_set 0x490050, which takes the five
//   binding fields individually rather than a source pointer)
// address 0x48be50, size 69 bytes
// name confidence: 0.45   rewrite confidence: 0.75
// evidence: writes the same 12-byte last_used_bindings[action] record (0x007127d4 + action*0xc)
//   as input_last_used_binding_set 0x490050 and input_refresh_last_used_binding 0x48bae0, either
//   copying it from *source or zeroing it when source is NULL. objdump at the one call site
//   (0x4b524e: `mov eax,esi; mov ecx,edi; call 0x48be50`) confirms EAX is the source pointer and
//   ECX the action index (only its low 16 bits, CX, are tested by this function).
// register convention: EAX -> source (control_binding_descriptor * or NULL), ECX -> action.
//   // blam-cc: EAX -> source, ECX -> action

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern input_abstraction_globals input_globals; // 0x00710328

// Directly sets (source != 0) or clears (source == 0) the cached last-used-binding record for
// game control action, when action is a valid input action index. No range checks against the
// source device's reported capabilities are performed here.
void input_last_used_binding_copy(int16_t action, control_binding_descriptor *source)
{
    if (action >= 0 && action < k_input_action_count) {
        if (source == 0) {
            input_globals.last_used_bindings[action].device_type = 0;
            input_globals.last_used_bindings[action].device_index = 0;
            input_globals.last_used_bindings[action].input_kind = 0;
            input_globals.last_used_bindings[action].input_index = 0;
            input_globals.last_used_bindings[action].direction = 0;
        } else {
            input_globals.last_used_bindings[action] = *source;
        }
    }
}

#if 0
Original Ghidra decompilation (0x48be50), from tools/pack.py 0x48be50:

void FUN_0048be50(void)

{
  int iVar1;
  undefined4 *in_EAX;
  short in_CX;

  if ((-1 < in_CX) && (in_CX < 0x1b)) {
    if (in_EAX == (undefined4 *)0x0) {
      iVar1 = in_CX * 0xc;
      *(undefined4 *)(&DAT_007127d4 + iVar1) = 0;
      *(undefined4 *)(&DAT_007127d8 + iVar1) = 0;
      *(undefined4 *)(&DAT_007127dc + iVar1) = 0;
      return;
    }
    iVar1 = in_CX * 0xc;
    *(undefined4 *)(&DAT_007127d4 + iVar1) = *in_EAX;
    *(undefined4 *)(&DAT_007127d8 + iVar1) = in_EAX[1];
    *(undefined4 *)(&DAT_007127dc + iVar1) = in_EAX[2];
  }
  return;
}

objdump call-site evidence (the only statically resolvable caller):
  004b524a: mov eax,esi                 ; source
  004b524c: mov ecx,edi                 ; action
  004b524e: call 0x48be50
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
