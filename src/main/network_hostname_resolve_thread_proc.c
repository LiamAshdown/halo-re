// network_hostname_resolve_thread_proc  (Ghidra: network_hostname_resolve_thread_proc, already named)
// address 0x4c8340, size 34 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: out/phase4/main_functions.md summary; the module header comment in types/main.h
// documents 0x00719b6c/0x00719b68 as the resolved hostent pointer and completion flag this
// module's connect-by-hostname staging (0x4c83e0, 0x4c8370) waits on. Same treatment of
// hostent as an opaque pointer as src/networking/network_local_hostent_get.c.
// register convention: __stdcall, one recognized parameter (hostname), matching the
// CreateThread thread-proc signature used by the caller (0x4c8370).
// UNSURE: `hostent` itself is a Winsock structure, not a Blam type; kept as an opaque void *
// returned straight from gethostbyname, same treatment as network_local_hostent_get.c.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"

extern void *hostname_resolve_result;      // 0x00719b6c, this module; struct hostent * from gethostbyname
extern int32_t hostname_resolve_complete;  // 0x00719b68, this module; set once the lookup returns

extern void *__stdcall gethostbyname(const char *name);
extern void __stdcall ExitThread(uint32_t exit_code);

// blam-cc: hostname as the recognized parameter (__stdcall thread proc)
uint32_t network_hostname_resolve_thread_proc(char *hostname)
{
    hostname_resolve_result = gethostbyname(hostname);
    hostname_resolve_complete = 1;
    ExitThread(0); // does not return
}

#if 0
Original Ghidra decompilation (0x4c8340):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

DWORD network_hostname_resolve_thread_proc(char *hostname)

{
  _DAT_00719b6c = gethostbyname(hostname);
  DAT_00719b68 = 1;
                    /* WARNING: Subroutine does not return */
  ExitThread(0);
}
#endif
