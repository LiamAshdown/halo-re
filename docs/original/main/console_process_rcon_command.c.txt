// console_process_rcon_command  (Ghidra: FUN_004c69a0, unnamed)
// address 0x4c69a0, size 26 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: types/interface.h console_rcon_handle (0x006b2f1c, "-1 routes output to the win32
// console"). Disassembly (objdump -d -M intel, bin/halo.exe) shows `mov ds:0x6b2f1c,eax` before
// the call to console_process_command and `push 0x0` as its stack argument, with EDI left
// untouched (inherited from this function's own caller) -- i.e. this is a thin wrapper that
// routes one command's output to a remote handle instead of the local console.
// register convention: EAX -> rcon_handle, EDI -> command_line (passed straight through to
// console_process_command). // blam-cc: EAX -> rcon_handle, EDI -> command_line

#include "tags.h"
#include "memory.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int32_t console_rcon_handle; // 0x006b2f1c

extern char console_process_command(char *command_line, uint32_t context_flags); // this module, 0x4c6a80, blam-cc: EDI -> command_line, stack -> context_flags

// Temporarily routes console output to rcon_handle, runs command_line as an ordinary console
// command (with no extra context bits), then restores console_rcon_handle to -1 (local console).
void console_process_rcon_command(int32_t rcon_handle, char *command_line) // blam-cc: EAX -> rcon_handle, EDI -> command_line
{
    console_rcon_handle = rcon_handle;
    console_process_command(command_line, 0);
    console_rcon_handle = -1;
}

#if 0
Original Ghidra decompilation (0x4c69a0):

void FUN_004c69a0(void)

{
  console_process_command(0);
  DAT_006b2f1c = 0xffffffff;
  return;
}

Disassembly (0x4c69a0..0x4c69b9):

004c69a0:  push   0x0                    ; context_flags = 0
004c69a2:  mov    ds:0x6b2f1c,eax        ; console_rcon_handle = rcon_handle (EAX)
004c69a7:  call   0x4c6a80               ; console_process_command(EDI = command_line, 0)
004c69ac:  add    esp,0x4
004c69af:  mov    DWORD PTR ds:0x6b2f1c,0xffffffff  ; console_rcon_handle = -1
004c69b9:  ret
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
