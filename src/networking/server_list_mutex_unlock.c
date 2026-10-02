// server_list_mutex_unlock  (Ghidra: FUN_004ba7a0; named per this rewrite)
// address 0x4ba7a0, size 31 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary ("Clears a caller-held server-list
// pointer and releases the shared server-browser query-result mutex"); pairs with
// server_list_mutex_try_lock (0x4ba760), which hands out the pointer this clears.
// register convention: EAX -> caller's server_list_globals* slot (in_EAX, Ghidra-unresolved).
//   // blam-cc: EAX -> list_slot

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_thread_record *server_list_thread; // 0x007196ac, the worker record
    // network_thread_create parks here (push 0x7196ac at 0x4b601e); NULL until it starts
extern network_mutex_record *server_list_mutex; // 0x007196a8


void server_list_mutex_unlock(server_list_globals **list_slot)
{
    *list_slot = 0;
    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

#if 0
Original Ghidra decompilation (0x4ba7a0):

void FUN_004ba7a0(void)

{
  undefined4 *in_EAX;

  *in_EAX = 0;
  if (DAT_007196ac != 0) {
    ReleaseMutex((HANDLE)*DAT_007196a8);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
