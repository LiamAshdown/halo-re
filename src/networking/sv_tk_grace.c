// sv_tk_grace  (Ghidra: sv_tk_grace, already named)
// address 0x4e3cd0, size 104 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; the literal "sv_tk_grace: %ds" string; the
// division-by-30 magic-constant multiply matches the seconds<->ticks conversion (30 ticks/sec).
// register convention: disassembly (objdump -d -M intel) reads `arguments` from [esp+0x4] as
// the very first instruction after `test eax,eax` -- with zero prior pushes, this can only be a
// genuine incoming stack argument. See sv_ban_penalty.c for the shared shape across this batch's
// "get/set console variable" commands.
//   // blam-cc: EAX -> argument_count, stack -> arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t sv_tk_grace_ticks; // 0x0069956c
extern char sv_tk_grace_arg_buffer[]; // 0x0066d568, UNSURE: scratch buffer reused by 0x4e51c0

extern int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table); // this batch, 0x4e51c0
extern void chimera__console_out(const char *format, ...); // 0x496b50

// Console command: gets or sets the team-kill grace period, stored internally in ticks (30/sec)
// but reported/parsed in seconds (default unit 's' when a bare number is given).
void sv_tk_grace(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    if (argument_count != 0) {
        if (argument_count != 1) {
            chimera__console_out("Incorrect usage. Type help sv_tk_grace for more information.");
            return;
        }
        {
            int32_t seconds = parse_time_duration_string((char *)arguments[0], 's', (uint8_t *)sv_tk_grace_arg_buffer);
            if (seconds < 0) {
                chimera__console_out("Incorrect usage. Type help sv_tk_grace for more information.");
                return;
            }
            sv_tk_grace_ticks = seconds * 30;
        }
    }
    chimera__console_out("sv_tk_grace: %ds", sv_tk_grace_ticks / 30);
}

#if 0
Original Ghidra decompilation (0x4e3cd0), from tools/pack.py 0x4e3cd0:

void sv_tk_grace(void)

{
  int in_EAX;
  int iVar1;

  if (in_EAX != 0) {
    if ((in_EAX != 1) || (iVar1 = parse_time_duration_string(0x73,&DAT_0066d568), iVar1 < 0)) {
      chimera__console_out();
      return;
    }
    DAT_0069956c = iVar1 * 0x1e;
  }
  chimera__console_out("sv_tk_grace: %ds",DAT_0069956c / 0x1e);
  return;
}
#endif
