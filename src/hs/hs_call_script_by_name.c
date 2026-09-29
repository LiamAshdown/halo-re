// hs_call_script_by_name  (Ghidra: hs_call_script_by_name, already named)
// address 0x48a2d0, size 31 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/hs_functions.md summary "Finds a script's existing thread by name and
//   restarts it, implementing calling a named HS script like a function."; this module's
//   hs_thread_find_by_script_name and hs_thread_restart.
// register convention: name in EAX (implicit; this function takes no recognized stack parameters
//   at all in the decompile).
//   // blam-cc: EAX -> name
// UNSURE: the register carrying `name` is inferred (EAX, first in the blam-cc order), not
// observed -- Ghidra shows zero parameters of any kind for this function.

#include "tags.h"
#include "memory.h"
#include "hs.h"
#include "fn_hs.h"


// Finds the thread already running the script named `name` and restarts it, implementing a
// script "call" as a plain restart of its existing thread. Returns 1 if a thread was found,
// 0 otherwise.
char hs_call_script_by_name(char *name)
{
    datum_index thread_handle;

    thread_handle = hs_thread_find_by_script_name(name);
    if (thread_handle != k_datum_index_none) {
        hs_thread_restart(thread_handle);
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x48a2d0):

undefined4 hs_call_script_by_name(void)

{
  int iVar1;

  iVar1 = hs_thread_find_by_script_name();
  if (iVar1 != -1) {
    FUN_0048a790(iVar1);
    return 1;
  }
  return 0;
}
#endif
