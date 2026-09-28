// server_list_result_count_get  (Ghidra: server_list_result_count_get, already named)
// address 0x4ba820, size 73 bytes
// name confidence: 0.5   rewrite confidence: 0.85
// evidence: types/networking.h server_list_globals.result_count (0x007196c0 == server_list +
// 0x04) and server_list_mutex / server_list_thread.
// register convention: __cdecl, no parameters.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern network_thread_record *server_list_thread; // 0x007196ac
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern server_list_globals server_list;         // 0x007196bc


// Thread-safely reads server_list.result_count, returning 0 if the mutex could not be acquired.
uint32_t server_list_result_count_get(void)
{
    uint32_t result;

    if (server_list_thread != 0) {
        uint32_t wait_result = WaitForSingleObject(server_list_mutex->handle, 100);

        if (wait_result != 0 && wait_result != 0x80) {
            return 0;
        }
    }

    result = (uint32_t)server_list.result_count;

    if (server_list_thread != 0) {
        ReleaseMutex(server_list_mutex->handle);
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x4ba820):

uint __cdecl server_list_result_count_get(void)

{
  uint uVar1;
  DWORD DVar2;

  if (((DAT_007196ac != 0) && (DVar2 = WaitForSingleObject((HANDLE)*DAT_007196a8,100), DVar2 != 0))
     && (DVar2 != 0x80)) {
    return 0;
  }
  uVar1 = DAT_007196c0;
  if (DAT_007196ac != 0) {
    ReleaseMutex((HANDLE)*DAT_007196a8);
  }
  return uVar1;
}
#endif
