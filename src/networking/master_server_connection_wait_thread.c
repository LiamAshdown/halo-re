// master_server_connection_wait_thread  (Ghidra: FUN_004b6070, still unnamed -> renamed)
// address 0x4b6070, size 221 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("waits for the master-server
// connection worker thread to finish, periodically pumping a UI update, then cleans up its
// handles"); the final cleanup writes exactly the fields master_server_connection_start.c
// documents for the same two globals (server_list_thread's thread handle at +0x00/+0x04,
// server_list_mutex's handle/name/in_use at +0x00/+0x04/+0x24).
// register convention: __cdecl, no arguments.
// UNSURE: this function dereferences server_list_thread (0x007196ac) as a
// network_thread_record*, confirming master_server_connection_start.c's note that
// network_thread_create's out-handle write there is a real pointer, not the plain int
// types/networking.h declares; declared with the pointer type actually used here.
// UNSURE: DAT_0072520c has no documented name; declared as an opaque millisecond timestamp.
// The QueryPerformanceCounter/__allmul/__alldiv shape is folded into plain int64_t arithmetic,
// matching network_update.c's precedent for the same idiom.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint32_t master_server_request_flags; // 0x0071969c
extern network_thread_record *server_list_thread; // 0x007196ac, see UNSURE
extern network_mutex_record *server_list_mutex;    // 0x007196a8
extern int32_t master_server_connection_last_tick_ms_0072520c; // 0x0072520c, see UNSURE
extern int64_t performance_frequency; // 0x006ac8f8/0x006ac8fc, owned by the timing/system module

extern void FUN_00549960(void); // foreign, outside this session's range
extern int32_t GetExitCodeThread(void *thread, uint32_t *exit_code);
extern void Sleep(uint32_t milliseconds);
extern int32_t QueryPerformanceCounter(large_integer *counter);
extern int32_t CloseHandle(void *object); // Win32

// blam-cc: __cdecl, no arguments
// Polls the master-server connection thread every 20ms until it exits, pumping FUN_00549960
// (roughly every 132ms) while waiting, then closes and clears both the thread handle and the
// server-list mutex.
void master_server_connection_wait_thread(void)
{
    int32_t exited;
    uint32_t exit_code;
    large_integer counter;
    int32_t now_ms;

    master_server_request_flags = master_server_request_flags | 2;
    while (1) {
        exited = GetExitCodeThread(server_list_thread->handle, &exit_code);
        if (exited != 0 && exit_code != 0x103) {
            break;
        }
        QueryPerformanceCounter(&counter);
        now_ms = (int32_t)((counter.quad_part * 1000) / performance_frequency);
        if (0x84 < (uint32_t)(now_ms - master_server_connection_last_tick_ms_0072520c)) {
            FUN_00549960();
        }
        master_server_request_flags = master_server_request_flags | 2;
        Sleep(0x14);
    }
    CloseHandle(server_list_thread->handle);
    server_list_thread->handle = 0;
    server_list_thread->in_use = 0;
    CloseHandle(server_list_mutex->handle);
    server_list_mutex->name[0] = 0;
    server_list_mutex->handle = 0;
    server_list_mutex->in_use = 0;
    server_list_thread = 0;
    server_list_mutex = 0;
}

#if 0
Original Ghidra decompilation (0x4b6070):

void FUN_004b6070(void)

{
  undefined4 *puVar1;
  BOOL BVar2;
  int iVar3;
  undefined8 uVar4;
  DWORD local_c;
  LARGE_INTEGER local_8;

  DAT_0071969c = DAT_0071969c | 2;
  while( true ) {
    BVar2 = GetExitCodeThread((HANDLE)*DAT_007196ac,&local_c);
    puVar1 = DAT_007196ac;
    if ((BVar2 != 0) && (local_c != 0x103)) break;
    QueryPerformanceCounter(&local_8);
    uVar4 = __allmul(local_8.s.LowPart,local_8.s.HighPart,1000,0);
    iVar3 = __alldiv(uVar4,DAT_006ac8f8,DAT_006ac8fc);
    if (0x84 < (uint)(iVar3 - DAT_0072520c)) {
      FUN_00549960();
    }
    DAT_0071969c = DAT_0071969c | 2;
    Sleep(0x14);
  }
  CloseHandle((HANDLE)*DAT_007196ac);
  *puVar1 = 0;
  *(undefined1 *)(puVar1 + 1) = 0;
  puVar1 = DAT_007196a8;
  CloseHandle((HANDLE)*DAT_007196a8);
  *(undefined1 *)(puVar1 + 1) = 0;
  *puVar1 = 0;
  *(undefined1 *)(puVar1 + 9) = 0;
  DAT_007196ac = (undefined4 *)0x0;
  DAT_007196a8 = (undefined4 *)0x0;
  return;
}
#endif
