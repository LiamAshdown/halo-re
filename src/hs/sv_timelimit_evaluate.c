// sv_timelimit_evaluate  (Ghidra: FUN_00482fc0, renamed)
// address 0x482fc0, size 77 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: HS external-function evaluate stub, identical shape to the already-named
// sv_name_evaluate (0x482ed0) and sv_password_evaluate (0x482f20): decodes one argument via
// hs_evaluate_variadic_arguments and forwards it straight to the native console setter
// sv_timelimit (0x004e49a0).
// register convention: (function_index, thread, first) recognized as ordinary stack
// parameters by Ghidra; no register remapping needed for this function itself.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern char hs_evaluate_variadic_arguments(uint32_t thread_index, int32_t value,
    uint32_t *out_count, int32_t **out_values); // this module, 0x0048ad60
extern void hs_thread_return(int32_t value, uint32_t thread_index);
    // blam-cc: EAX -> value, ECX -> thread_index; this module, 0x0048a640
extern void sv_timelimit(uint32_t argument_count, int32_t *arguments); // 
    // blam-cc: EAX -> argument_count on all but the two *_matching_substring
    // handlers, which pass both on the stack

// Evaluate handler for the sv_timelimit external HS function: decodes its single argument
// and forwards it straight to sv_timelimit, then pops the evaluation frame.
void sv_timelimit_evaluate(int16_t function_index, datum_index thread, char first)
{
    uint32_t argument_count;
    int32_t *arguments;
    char ready;

    argument_count = 0;
    arguments = 0;
    ready = hs_evaluate_variadic_arguments(thread, first, &argument_count, &arguments);
    if (ready != 0) {
        sv_timelimit(argument_count, arguments);
        // `xor eax,eax / mov ecx,esi` before the call: the value is 0 and the thread
        // handle is carried in ECX. (Was declared as a one-argument function here.)
        hs_thread_return(0, thread);
    }
}

#if 0
Original Ghidra decompilation (0x482fc0):

void FUN_00482fc0(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  char cVar1;
  undefined4 local_8;
  undefined4 local_4;

  local_4 = 0;
  local_8 = 0;
  cVar1 = hs_evaluate_variadic_arguments(param_2,param_3,&local_4,&local_8);
  if (cVar1 != '\0') {
    sv_timelimit(local_8);
    FUN_0048a640();
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
