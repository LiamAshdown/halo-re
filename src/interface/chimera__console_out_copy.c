// chimera__console_out_copy  (Ghidra: chimera__console_out_copy, already named)
// address 0x496e90, size 242 bytes
// name confidence: 0.7   rewrite confidence: 0.55
// evidence: out/phase4/interface_functions.md summary; mirrors console output (in_EAX, the just
// printed console line) out to an attached rcon session and/or the win32 console window, after
// stripping the two formatting tokens the developer console uses internally.
// register convention: the printed line text in EAX (in_EAX, unresolved register read).
// blam-cc: EAX -> text
// UNSURE: 0x00718f80 is a plain reentrancy guard around chimera__rcon_out with no name recovered
// elsewhere in this module; kept as an anonymous extern.

#include "crt.h"
#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern int32_t console_rcon_handle;       // 0x006b2f1c, -1 routes output to the win32 console
extern uint8_t console_rcon_out_reentrant_guard; // 0x00718f80, UNSURE: no other reference found
extern uint8_t console_win32_attached;    // 0x006b2f18
extern char console_echo_prefix[];        // 0x00669140, matched by chimera__console_out
extern char DAT_0065fb2c[];               // 0x0065fb2c, UNSURE: exact literal not recovered
extern char DAT_0065fb14[];               // 0x0065fb14, UNSURE: single character, likely "\n"
extern char DAT_00669ae0[];               // 0x00669ae0, UNSURE: exact literal not recovered
extern void *console_output_handle;       // 0x006b2dd0, win32 console output handle

extern void chimera__rcon_out(int32_t rcon_handle);      // 0x4e50c0, networking module
extern void console_clear_bottom_line(int32_t clear_all); // 0x497010
extern void console_draw_input_line(void);                 // 0x4970a0
extern void string_replace_all_in_place(char *buffer, char *search, char *replacement); // 0x496df0

// blam-cc: EAX -> text
// Mirrors one printed console line: forwards it to an active rcon session (reentrancy-guarded),
// and, if a win32 console is attached, copies the line into a scratch buffer, strips the two
// internal formatting tokens, clears the console's bottom line and writes the result out
// followed by the developer-console line terminator, then redraws the input line.
void chimera__console_out_copy(char *text)
{
    char line[0x104];
    uint32_t chars_written;
    uint32_t length;

    if (console_rcon_handle != -1 && console_rcon_out_reentrant_guard == 0) {
        console_rcon_out_reentrant_guard = 1;
        chimera__rcon_out(console_rcon_handle);
        console_rcon_out_reentrant_guard = 0;
    }
    if (console_win32_attached != 0) {
        line[0] = '\0';
        strncpy(line, text, 0x100);
        string_replace_all_in_place(line, console_echo_prefix, DAT_0065fb2c);
        string_replace_all_in_place(line, DAT_00669ae0, DAT_0065fb14);
        length = strlen(line);
        console_clear_bottom_line(1);
        WriteConsoleA(console_output_handle, line, length, &chars_written, (void *)0);
        WriteConsoleA(console_output_handle, DAT_0065fb14, 1, &chars_written, (void *)0);
        console_draw_input_line();
    }
}

#if 0
Original Ghidra decompilation (0x496e90):

void chimera__console_out_copy(void)

{
  char cVar1;
  char *in_EAX;
  char *pcVar2;
  int iVar3;
  undefined4 *puVar4;
  DWORD local_114;
  char local_110;
  undefined4 local_10f [66];

  if ((DAT_006b2f1c != -1) && (DAT_00718f80 == '\0')) {
    DAT_00718f80 = 1;
    chimera__rcon_out(DAT_006b2f1c);
    DAT_00718f80 = '\0';
  }
  if (DAT_006b2f18 != '\0') {
    local_110 = '\0';
    puVar4 = local_10f;
    for (iVar3 = 0x40; iVar3 != 0; iVar3 = iVar3 + -1) {
      *puVar4 = 0;
      puVar4 = puVar4 + 1;
    }
    _strncpy(&local_110,in_EAX,0x100);
    FUN_00496df0(&DAT_00669140,&DAT_0065fb2c);
    FUN_00496df0(&DAT_00669ae0,&DAT_0065fb14);
    pcVar2 = &local_110;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    console_clear_bottom_line(1);
    WriteConsoleA(DAT_006b2dd0,&local_110,(int)pcVar2 - (int)local_10f,&local_114,(LPVOID)0x0);
    WriteConsoleA(DAT_006b2dd0,&DAT_0065fb14,1,&local_114,(LPVOID)0x0);
    console_draw_input_line();
  }
  return;
}
#endif
