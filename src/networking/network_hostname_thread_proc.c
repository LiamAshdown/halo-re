// network_hostname_thread_proc  (Ghidra: network_hostname_thread_proc, already named)
// address 0x441510, size 34 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/networking_functions.md summary ("thread entry point that retrieves the
// local machine's hostname into a buffer, signals completion, and exits the thread"); Ghidra
// already recovered the full __stdcall signature and both Win32 calls by name.
// register convention: __stdcall, one recognized stack parameter (hostname_buffer).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern uint8_t network_hostname_ready; // 0x006f14cc

extern int32_t gethostname(char *name, int32_t buffer_length);
extern void ExitThread(uint32_t exit_code);

void network_hostname_thread_proc(char *hostname_buffer)
{
    gethostname(hostname_buffer, 0x100);
    network_hostname_ready = 1;
    // Subroutine does not return.
    ExitThread(0);
}

#if 0
Original Ghidra decompilation (0x441510):

void network_hostname_thread_proc(char *hostname_buffer)

{
  gethostname(hostname_buffer,0x100);
  DAT_006f14cc = 1;
                    /* WARNING: Subroutine does not return */
  ExitThread(0);
}
#endif
