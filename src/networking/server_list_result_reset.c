// server_list_result_reset  (Ghidra: FUN_004ba7c0; named per this rewrite)
// address 0x4ba7c0, size 87 bytes
// name confidence: 0.4   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md summary ("Either resets a caller-supplied
// server-list entry's fields, or, with no entry supplied, clears the shared server-list result
// counters under the query-result mutex"); types/networking.h server_list_mutex /
// server_list_thread / server_list.result_count (0x007196c0 == server_list + 0x04).
// register convention: EAX -> entry (in_EAX, Ghidra-unresolved; NULL means "clear the shared
// counters instead"). // blam-cc: EAX -> entry
// UNSURE: `entry` is a GameSpy SDK server record (see types/networking.h's "server browser"
// section: "the queried-server type is not recoverable here and none is declared"), so its
// +0x04/+0x0c fields are accessed as raw offsets rather than through a named struct.
// UNSURE: 0x007196c8 sits 0x0c bytes past server_list (whose declared size is only 0x08), so it
// is a separate scalar global, not a server_list_globals field. master_server_ensure_list_connection (elsewhere in this
// module) sets the same global to 9999 when a query attempt is abandoned, which reads as an
// "unknown/very large" elapsed-time sentinel; named accordingly but not independently confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_thread_record *server_list_thread; // 0x007196ac
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern server_list_globals server_list;         // 0x007196bc
extern int32_t server_browser_query_elapsed_ms; // 0x007196c8, UNSURE: see file header

extern uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms); // Win32
extern int32_t ReleaseMutex(void *handle);                              // Win32

void server_list_result_reset(uint8_t *entry)
{
    if (entry != 0) {
        *(uint32_t *)(entry + 4) = 0;
        *(uint32_t *)(entry + 0xc) = 0;
        return;
    }

    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, 100);

        if (wait_result != 0 && wait_result != 0x80) {
            return;
        }
    }

    server_list.result_count = 0;
    server_browser_query_elapsed_ms = 0;

    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
}

#if 0
Original Ghidra decompilation (0x4ba7c0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004ba7c0(void)

{
  int in_EAX;
  DWORD DVar1;

  if (in_EAX != 0) {
    *(undefined4 *)(in_EAX + 4) = 0;
    *(undefined4 *)(in_EAX + 0xc) = 0;
    return;
  }
  if (((DAT_007196ac != 0) && (DVar1 = WaitForSingleObject((HANDLE)*DAT_007196a8,100), DVar1 != 0))
     && (DVar1 != 0x80)) {
    return;
  }
  DAT_007196c0 = 0;
  _DAT_007196c8 = 0;
  if (DAT_007196ac != 0) {
    ReleaseMutex((HANDLE)*DAT_007196a8);
  }
  return;
}
#endif
