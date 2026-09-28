// server_list_mutex_try_lock  (Ghidra: server_list_mutex_try_lock, already named)
// address 0x4ba760, size 54 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/networking.h server_list_globals / server_list_mutex / server_list_thread
// (0x616 "server browser" section); network_mutex_record::handle at offset 0, matching the
// dereference of server_list_mutex here.
// register convention: __cdecl, timeout_ms on the stack (Ghidra-recognized param).

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_thread_record *server_list_thread; // 0x007196ac, the worker record
    // network_thread_create parks here (push 0x7196ac at 0x4b601e); NULL until it starts
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern server_list_globals server_list;      // 0x007196bc


// Waits (with a timeout) on the shared server-browser query-result mutex, unless the mutex has
// never been created, and returns a pointer to the shared server_list object on success (either
// the wait was not needed, it completed normally, or it timed out after abandonment -- 0x80 is
// WAIT_ABANDONED_0), or NULL if the wait failed or genuinely timed out.
server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms)
{
    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, timeout_ms);

        if (wait_result != 0 && wait_result != 0x80) {
            return 0;
        }
    }
    return &server_list;
}

#if 0
Original Ghidra decompilation (0x4ba760):

void * __cdecl server_list_mutex_try_lock(uint timeout_ms)

{
  DWORD DVar1;

  if (DAT_007196ac != 0) {
    DVar1 = WaitForSingleObject((HANDLE)*DAT_007196a8,timeout_ms);
    if ((DVar1 != 0) && (DVar1 != 0x80)) {
      return (void *)0x0;
    }
  }
  return &DAT_007196bc;
}
#endif
