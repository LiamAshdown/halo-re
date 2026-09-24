// master_server_process_pending_requests  (Ghidra: FUN_004b5d70, still unnamed -> renamed)
// address 0x4b5d70, size 508 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md summary ("background master-server processing
// tick: services pending server-list queries, heartbeats, and single-server refresh requests
// indicated by a request flag bitmask"); server_list_mutex/server_list_thread/
// server_list (types/networking.h "server browser" section) match DAT_007196a8/ac/bc/c0
// exactly; server_list_reset and server_list_mutex_try_lock are this module's own rewrites.
// register convention: __cdecl, no arguments.
// UNSURE: DAT_0071946c (the GameSpy master-server query engine object) is not documented
// anywhere in networking_types_notes.md; declared as an opaque `void *` extern.
// FIXED in the review pass: an earlier draft read the bit-0x10 loop as indexing the address
// of server_list. The disassembly at 0x4b5e00 is `mov edx,ds:0x7196bc; mov eax,[edx+esi*4]`,
// i.e. it loads the pointer stored there and indexes that -- server_list.list[i], exactly
// like the bit-0x20 branch does through the mutex_try_lock result.
// UNSURE: DAT_00695424 (buffer) and DAT_007227b8 (value used both whole and masked to its low
// 16 bits) have no documented names or types; declared as opaque externs.
// UNSURE: DAT_006953f4, used as a dword index into the object server_list_mutex_try_lock's
// result points at, is declared here as `server_browser_selected_index`; the "server
// browser" note in networking_types_notes.md explicitly lists the sort/scroll/selection
// globals as unresolved, and this may be one of them.
// UNSURE: FUN_00616f80/fa0/fc0, FUN_006171a0/d0 and FUN_00617290 are foreign GameSpy library
// calls; only their observed argument shapes are kept, not real names or full signatures.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern void *master_server_query_engine;        // 0x0071946c, see UNSURE
extern uint32_t master_server_request_flags;    // 0x0071969c
extern int32_t master_server_last_result;       // 0x007196a4
extern network_thread_record *server_list_thread; // 0x007196ac
extern network_mutex_record *server_list_mutex; // 0x007196a8
extern server_list_globals server_list;         // 0x007196bc
extern uint8_t server_browser_require_valid_entry; // 0x006953f0
extern uint8_t DAT_00695424[10];                // see UNSURE
extern uint32_t DAT_007227b8;                   // see UNSURE
extern int32_t server_browser_selected_index; // 0x006953f4, see UNSURE

extern server_list_globals *server_list_mutex_try_lock(uint32_t timeout_ms); // 0x4ba760, this module
extern void server_list_reset(void); // 0x4b65f0, this module

extern void FUN_00616fa0(void *engine); // foreign, GameSpy library
extern int32_t FUN_00616f80(void *engine); // foreign, GameSpy library
extern void FUN_00616fc0(void *engine); // foreign, GameSpy library
extern int32_t FUN_006171d0(void *engine, int32_t flag, uint32_t address, uint16_t port); // foreign
extern int32_t FUN_006171a0(void *engine, int32_t flag, int32_t unused_a, void *buffer,
                              int32_t buffer_length, int32_t unused_b); // foreign
extern int32_t FUN_00617290(void *engine, void *server_record, int32_t flag_a, int32_t flag_b); // foreign
extern uint32_t WaitForSingleObject(void *handle, uint32_t timeout_ms); // Win32
extern int32_t ReleaseMutex(void *handle); // Win32

// blam-cc: __cdecl, no arguments
void master_server_process_pending_requests(void)
{
    uint32_t flags;
    int32_t last_result;
    uint32_t wait_result;
    int32_t i;
    int32_t index;

    flags = master_server_request_flags;
    master_server_request_flags = 0;
    if (master_server_query_engine != 0) {
        last_result = 0;
        if ((flags & 4) != 0) {
            FUN_00616fa0(master_server_query_engine);
        }
        master_server_last_result = FUN_00616f80(master_server_query_engine);
        if ((flags & 0x10) != 0) {
            if (server_list_thread == 0 ||
                (wait_result = WaitForSingleObject(server_list_mutex->handle, 100),
                 wait_result == 0) || wait_result == 0x80) {
                if (server_list.result_count < 1) {
                    flags = flags | 8;
                } else {
                    i = 0;
                    if (0 < server_list.result_count) {
                        do {
                            last_result = FUN_00617290(master_server_query_engine,
                                                        server_list.list[i], 1, 1);
                            if (last_result != 0) {
                                break;
                            }
                            i = i + 1;
                        } while (i < server_list.result_count);
                    }
                    server_list_reset();
                }
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        if ((flags & 8) != 0) {
            if (server_list_thread == 0 ||
                (wait_result = WaitForSingleObject(server_list_mutex->handle, 100),
                 wait_result == 0) || wait_result == 0x80) {
                FUN_00616fc0(master_server_query_engine);
                server_list_reset();
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
                if (server_browser_require_valid_entry == 0) {
                    last_result = FUN_006171d0(master_server_query_engine, 1, DAT_007227b8,
                                                (uint16_t)DAT_007227b8);
                } else {
                    last_result = FUN_006171a0(master_server_query_engine, 1, 0, DAT_00695424, 10, 0);
                }
            } else {
                master_server_request_flags = master_server_request_flags | 0x10;
            }
        }
        index = server_browser_selected_index;
        if ((flags & 0x20) != 0 && server_browser_selected_index != -1) {
            server_list_globals *locked = server_list_mutex_try_lock(100);
            if (locked == 0) {
                master_server_request_flags = master_server_request_flags | 0x20;
            } else {
                last_result = FUN_00617290(master_server_query_engine,
                                            locked->list[index], 1, 1);
                if (server_list_thread != 0) {
                    ReleaseMutex(server_list_mutex->handle);
                }
            }
        }
        if (master_server_last_result == 0 && last_result != 0) {
            master_server_last_result = last_result;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4b5d70):

void FUN_004b5d70(void)

{
  DWORD DVar1;
  int *piVar2;
  uint uVar3;
  int iVar4;
  int iVar5;

  uVar3 = DAT_0071969c;
  DAT_0071969c = 0;
  if (DAT_0071946c != 0) {
    iVar5 = 0;
    if ((uVar3 & 4) != 0) {
      FUN_00616fa0(DAT_0071946c);
    }
    DAT_007196a4 = FUN_00616f80(DAT_0071946c);
    if ((uVar3 & 0x10) != 0) {
      if (((DAT_007196ac == 0) ||
          (DVar1 = WaitForSingleObject((HANDLE)*DAT_007196a8,100), DVar1 == 0)) || (DVar1 == 0x80))
      {
        if (DAT_007196c0 < 1) {
          uVar3 = uVar3 | 8;
        }
        else {
          iVar4 = 0;
          if (0 < DAT_007196c0) {
            do {
              iVar5 = FUN_00617290(DAT_0071946c,*(undefined4 *)(DAT_007196bc + iVar4 * 4),1,1);
              if (iVar5 != 0) break;
              iVar4 = iVar4 + 1;
            } while (iVar4 < DAT_007196c0);
          }
          server_list_reset();
        }
        if (DAT_007196ac != 0) {
          ReleaseMutex((HANDLE)*DAT_007196a8);
        }
      }
      else {
        DAT_0071969c = DAT_0071969c | 0x10;
      }
    }
    if ((uVar3 & 8) != 0) {
      if (((DAT_007196ac == 0) ||
          (DVar1 = WaitForSingleObject((HANDLE)*DAT_007196a8,100), DVar1 == 0)) || (DVar1 == 0x80))
      {
        FUN_00616fc0(DAT_0071946c);
        server_list_reset();
        if (DAT_007196ac != 0) {
          ReleaseMutex((HANDLE)*DAT_007196a8);
        }
        if (DAT_006953f0 == '\0') {
          iVar5 = FUN_006171d0(DAT_0071946c,1,DAT_007227b8,DAT_007227b8 & 0xffff);
        }
        else {
          iVar5 = FUN_006171a0(DAT_0071946c,1,0,&DAT_00695424,10,0);
        }
      }
      else {
        DAT_0071969c = DAT_0071969c | 0x10;
      }
    }
    iVar4 = DAT_006953f4;
    if (((uVar3 & 0x20) != 0) && (DAT_006953f4 != -1)) {
      piVar2 = server_list_mutex_try_lock(100);
      if (piVar2 == (int *)0x0) {
        DAT_0071969c = DAT_0071969c | 0x20;
      }
      else {
        iVar5 = FUN_00617290(DAT_0071946c,*(undefined4 *)(*piVar2 + iVar4 * 4),1,1);
        if (DAT_007196ac != 0) {
          ReleaseMutex((HANDLE)*DAT_007196a8);
        }
      }
    }
    if ((DAT_007196a4 == 0) && (iVar5 != 0)) {
      DAT_007196a4 = iVar5;
    }
  }
  return;
}
#endif
