// sv_rcon_password  (Ghidra: sv_rcon_password, already named)
// address 0x4e4b50, size 164 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; literal "rcon is DISABLED" / "Maximum rcon
// password length" strings; types/networking.h notes the server's rcon connection id lives near
// 0x0069fdfc, but the password string itself here is a separate small global at 0x0071c410.
// register convention: disassembly (objdump -d -M intel) reads `arguments` from [esp+0x8] after
// one prologue push (ecx, used only as stack-space filler) -- the same EAX/stack shape as
// sv_ban_penalty.c and friends.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: the exact size/owner of the 0x0071c410 password buffer beyond the 9-byte cap this
// function enforces (an 8-character password plus NUL); no struct is declared for it here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern char sv_rcon_password_value[9]; // 0x0071c410, 8 chars + forced NUL

extern void chimera__console_out(const char *format, ...); // 0x496b50

// Console command: gets or sets the server's rcon password (empty string disables rcon),
// enforcing an 8-character maximum.
void sv_rcon_password(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    if (argument_count == 0) {
    report:
        if (sv_rcon_password_value[0] == 0) {
            chimera__console_out("sv_rcon_password: '' (rcon is DISABLED)");
            return;
        }
        chimera__console_out("sv_rcon_password: '%s'", sv_rcon_password_value, strlen(sv_rcon_password_value));
        return;
    }
    if (argument_count == 1) {
        char *arg = (char *)arguments[0];
        if (strlen(arg) < 9) {
            strcpy(sv_rcon_password_value, arg);
            goto report;
        }
        chimera__console_out("Maximum rcon password length is %d characters", 8);
    }
    chimera__console_out("Incorrect usage. Type help sv_rcon_password for more information.");
}

#if 0
Original Ghidra decompilation (0x4e4b50), from tools/pack.py 0x4e4b50:

void sv_rcon_password(undefined4 *param_1)

{
  char cVar1;
  int in_EAX;
  char *pcVar2;
  char *pcVar3;

  if (in_EAX == 0) {
LAB_004e4b8a:
    pcVar3 = &DAT_0071c410;
    do {
      pcVar2 = pcVar3;
      pcVar3 = pcVar2 + 1;
    } while (*pcVar2 != '\0');
    if (pcVar2 + -0x71c410 == (char *)0x0) {
      chimera__console_out();
      return;
    }
    chimera__console_out("sv_rcon_password: \'%s\'",&DAT_0071c410,pcVar2 + -0x71c410);
    return;
  }
  if (in_EAX == 1) {
    pcVar3 = (char *)*param_1;
    pcVar2 = pcVar3;
    do {
      cVar1 = *pcVar2;
      pcVar2 = pcVar2 + 1;
    } while (cVar1 != '\0');
    if ((uint)((int)pcVar2 - (int)(pcVar3 + 1)) < 9) {
      pcVar2 = &DAT_0071c410;
      do {
        cVar1 = *pcVar3;
        pcVar3 = pcVar3 + 1;
        *pcVar2 = cVar1;
        pcVar2 = pcVar2 + 1;
      } while (cVar1 != '\0');
      goto LAB_004e4b8a;
    }
    chimera__console_out("Maximum rcon password length is %d characters",8);
  }
  chimera__console_out();
  return;
}
#endif
