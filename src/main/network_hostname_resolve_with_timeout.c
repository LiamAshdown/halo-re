// network_hostname_resolve_with_timeout  (Ghidra: network_hostname_resolve_with_timeout, already named)
// address 0x4c8370, size 111 bytes
// name confidence: 0.55   rewrite confidence: 0.75
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites" table:
// "network_hostname_resolve_with_timeout 0x4c8370: ECX = host name (0x4c8410)". Confirmed
// directly in objdump -d -M intel bin/halo.exe at 0x4c8370..0x4c83de: `push ecx` at entry is a
// scratch slot for the CreateThread thread-id output, `push ecx` again just before the
// 0x4c8340 (network_hostname_resolve_thread_proc) push is the ORIGINAL ecx value handed to
// CreateThread as its thread parameter, i.e. the hostname pointer this function received in ECX.
// register convention: __fastcall-like, ECX -> hostname (in_ECX, unresolved register read).
// blam-cc: ECX -> hostname

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"

extern int32_t hostname_resolve_complete; // 0x00719b68, this module (network_hostname_resolve_thread_proc.c)
extern void *hostname_resolve_result;     // 0x00719b6c, this module (network_hostname_resolve_thread_proc.c)

extern uint32_t network_hostname_resolve_thread_proc(char *hostname); // 0x4c8340, this module

// blam-cc: ECX -> hostname
// Resolves hostname on a worker thread, waiting up to 10 seconds; kills the thread if it hasn't
// finished by then. Returns 1 if the resolve completed (hostname_resolve_result was filled in),
// 0 otherwise (also clearing hostname_resolve_result on the timeout/failure path).
char network_hostname_resolve_with_timeout(char *hostname)
{
    void *thread_handle;
    uint32_t thread_id;
    uint32_t wait_result;

    hostname_resolve_complete = 0;
    thread_handle = CreateThread(0, k_main_hostname_thread_stack_size,
                                  (LPTHREAD_START_ROUTINE)network_hostname_resolve_thread_proc, hostname, 0,
                                  &thread_id); // the routine ends in ExitThread (never returns)
    if (thread_handle != 0) {
        wait_result = WaitForSingleObject(thread_handle, k_main_hostname_resolve_timeout_ms);
        if (wait_result == 0x102) {
            TerminateThread(thread_handle, 0);
        }
        CloseHandle(thread_handle);
        if (hostname_resolve_complete != 0) {
            return 1;
        }
    }
    hostname_resolve_result = 0;
    return 0;
}

#if 0
Original Ghidra decompilation (0x4c8370):

undefined4 network_hostname_resolve_with_timeout(void)

{
  HANDLE hHandle;
  DWORD DVar1;
  LPVOID in_ECX;
  DWORD local_4;

  DAT_00719b68 = 0;
  hHandle = CreateThread((LPSECURITY_ATTRIBUTES)0x0,0x10400,network_hostname_resolve_thread_proc,
                         in_ECX,0,&local_4);
  if (hHandle != (HANDLE)0x0) {
    DVar1 = WaitForSingleObject(hHandle,10000);
    if (DVar1 == 0x102) {
      TerminateThread(hHandle,0);
    }
    CloseHandle(hHandle);
    if (DAT_00719b68 != 0) {
      return 1;
    }
  }
  _DAT_00719b6c = 0;
  return 0;
}
#endif
