// sv_tk_cooldown  (Ghidra: sv_tk_cooldown, already named)
// address 0x4e3d40, size 104 bytes
// name confidence: 0.9   rewrite confidence: 0.75
// evidence: out/phase4/networking_functions.md; the literal "sv_tk_cooldown: %ds" string;
// identical shape to sv_tk_grace.c (this batch) apart from the global it reads/writes.
// register convention: same as sv_tk_grace.c (EAX -> argument_count, stack -> arguments),
// confirmed independently by disassembly (objdump -d -M intel).
//   // blam-cc: EAX -> argument_count, stack -> arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t sv_tk_cooldown_ticks; // 0x00699570
extern char sv_tk_grace_arg_buffer[]; // 0x0066d568, UNSURE: same scratch buffer as sv_tk_grace.c

extern int32_t parse_time_duration_string(char *string, char default_unit, uint8_t *unit_table); // this batch, 0x4e51c0
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: gets or sets the team-kill punishment cooldown, stored internally in ticks
// (30/sec) but reported/parsed in seconds (default unit 's' when a bare number is given).
void sv_tk_cooldown(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    if (argument_count != 0) {
        if (argument_count != 1) {
            chimera__console_out((ColorARGB *)0, "Incorrect usage. Type help sv_tk_cooldown for more information.");
            return;
        }
        {
            int32_t seconds = parse_time_duration_string((char *)arguments[0], 's', (uint8_t *)sv_tk_grace_arg_buffer);
            if (seconds < 0) {
                chimera__console_out((ColorARGB *)0, "Incorrect usage. Type help sv_tk_cooldown for more information.");
                return;
            }
            sv_tk_cooldown_ticks = seconds * 30;
        }
    }
    chimera__console_out((ColorARGB *)0, "sv_tk_cooldown: %ds", sv_tk_cooldown_ticks / 30);
}

#if 0
Original Ghidra decompilation (0x4e3d40), from tools/pack.py 0x4e3d40:

void sv_tk_cooldown(void)

{
  int in_EAX;
  int iVar1;

  if (in_EAX != 0) {
    if ((in_EAX != 1) || (iVar1 = parse_time_duration_string(0x73,&DAT_0066d568), iVar1 < 0)) {
      chimera__console_out();
      return;
    }
    DAT_00699570 = iVar1 * 0x1e;
  }
  chimera__console_out("sv_tk_cooldown: %ds",DAT_00699570 / 0x1e);
  return;
}
#endif
