// network_local_hostent_get  (Ghidra: network_local_hostent_get, already named)
// address 0x441540, size 122 bytes
// name confidence: 0.55   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md summary ("resolves and returns the local
// machine's hostent structure, retrieving the hostname on a watchdog-timed worker thread
// first"); Ghidra already recovered the full signature. Shares network_hostname_ready
// (0x006f14cc) with network_hostname_thread_proc.c and the hostname buffer with it too.
// register convention: __cdecl, one recognized parameter (out_hostent).
// UNSURE: `hostent` itself is a Winsock structure, not a Blam type; kept as an opaque
// void * returned straight from gethostbyname, same treatment as
// src/networking/network_join_hostname_resolved_callback.c.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern char network_local_hostname_buffer[0x100]; // 0x006a4040
extern uint8_t network_hostname_ready;             // 0x006f14cc

extern void network_hostname_thread_proc(char *hostname_buffer); // 0x441510, this module

int network_local_hostent_get(void **out_hostent)
{
    void *thread_handle;
    uint32_t wait_result;
    uint32_t thread_id;

    network_hostname_ready = 0;
    thread_handle = CreateThread(0, 0x10400, (LPTHREAD_START_ROUTINE)network_hostname_thread_proc, // ends in ExitThread
                                  network_local_hostname_buffer, 0, &thread_id);
    if (thread_handle != 0) {
        wait_result = WaitForSingleObject(thread_handle, 10000);
        if (wait_result == 0x102) {
            TerminateThread(thread_handle, 0);
        }
        CloseHandle(thread_handle);
        if (network_hostname_ready != 0) {
            *out_hostent = gethostbyname(network_local_hostname_buffer);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x441540):

int __cdecl network_local_hostent_get(void **out_hostent)

{
  HANDLE hHandle;
  DWORD DVar1;
  hostent *phVar2;
  DWORD local_4;

  DAT_006f14cc = 0;
  hHandle = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,network_hostname_thread_proc,
                         &DAT_006a4040,0,&local_4);
  if (hHandle != (HANDLE)0x0) {
    DVar1 = WaitForSingleObject(hHandle,10000);
    if (DVar1 == 0x102) {
      TerminateThread(hHandle,0);
    }
    CloseHandle(hHandle);
    if (DAT_006f14cc != 0) {
      phVar2 = gethostbyname(&DAT_006a4040);
      *out_hostent = phVar2;
      return 1;
    }
  }
  return 0;
}
#endif
