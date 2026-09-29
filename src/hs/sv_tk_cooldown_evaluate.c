// sv_tk_cooldown_evaluate  (Ghidra: FUN_00482d50, renamed)
// address 0x482d50, size 77 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: HS external-function evaluate stub, identical shape to the already-named
// sv_name_evaluate (0x482ed0) and sv_password_evaluate (0x482f20): decodes one argument via
// hs_evaluate_variadic_arguments and forwards it straight to the native console setter
// sv_tk_cooldown (0x004e3d40).
// register convention: (function_index, thread, first) recognized as ordinary stack
// parameters by Ghidra; no register remapping needed for this function itself.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


    // blam-cc: EAX -> value, ECX -> thread_index; this module, 0x0048a640
extern void sv_tk_cooldown(uint32_t argument_count, int32_t *arguments); // 
    // blam-cc: EAX -> argument_count on all but the two *_matching_substring
    // handlers, which pass both on the stack

// Evaluate handler for the sv_tk_cooldown external HS function: decodes its single argument
// and forwards it straight to sv_tk_cooldown, then pops the evaluation frame.
void sv_tk_cooldown_evaluate(int16_t function_index, datum_index thread, char first)
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_tk_cooldown(argument_count, arguments);
        // `xor eax,eax / mov ecx,esi` before the call: the value is 0 and the thread
        // handle is carried in ECX. (Was declared as a one-argument function here.)
        hs_thread_return(0, thread);
    }
}

#if 0
Original Ghidra decompilation (0x482d50):

void FUN_00482d50(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0;
  local_8 = 0;
  cVar1 = hs_evaluate_variadic_arguments(param_2,param_3,&local_4,&local_8);
  if (cVar1 != '\0') {
    sv_tk_cooldown(local_8);
    FUN_0048a640();
  }
  return;
}
#endif
