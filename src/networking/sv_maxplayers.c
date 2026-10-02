// sv_maxplayers  (Ghidra: sv_maxplayers, already named)
// address 0x4e4ac0, size 130 bytes
// name confidence: 0.9   rewrite confidence: 0.7
// evidence: out/phase4/networking_functions.md; src/hs/sv_maxplayers_evaluate.c already forwards
// its HS argument straight to this function as (argument_count, arguments); types/networking.h
// network_server_globals (maximum_players is inside network_game_session, at server+0x1a5
// counting from the server base -- 0x008 session offset + 0x19d maximum_players -- matching the
// `*(char*)(server+0x1a5)` write here).
// register convention: disassembly (objdump -d -M intel) reads `arguments` from [esp+0x4] with
// zero prologue pushes before it -- the same EAX/stack shape as sv_ban_penalty.c and friends.
//   // blam-cc: EAX -> argument_count, stack -> arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdlib.h>

extern network_server_globals *network_server; // 0x0071c2d4
extern int32_t sv_maxplayers_value; // 0x00699584

extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: gets or sets the maximum number of players (1-16) for a hosted server.
void sv_maxplayers(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    if (argument_count == 0) {
    report:
        chimera__console_out((ColorARGB *)0, (char *)"sv_maxplayers: %d", sv_maxplayers_value);
        return;
    }
    if (argument_count == 1) {
        int32_t value = atol((char *)arguments[0]);
        if (0 < value && value < 0x11) {
            sv_maxplayers_value = value;
            if (network_server != 0) {
                network_server->session.maximum_players = (uint8_t)value;
            }
            if (value == 1) {
                chimera__console_out((ColorARGB *)0, (char *)"WARNING: sv_maxplayers set to 1, are you sure you want to do this?");
            }
            goto report;
        }
        chimera__console_out((ColorARGB *)0, (char *)"sv_maxplayers must be between 1 and %d", 0x10);
    }
    chimera__console_out((ColorARGB *)0, (char *)"Incorrect usage. Type help sv_maxplayers for more information.");
}

#if 0
Original Ghidra decompilation (0x4e4ac0), from tools/pack.py 0x4e4ac0:

void sv_maxplayers(undefined4 *param_1)

{
  int in_EAX;
  long lVar1;

  if (in_EAX == 0) {
LAB_004e4b0b:
    chimera__console_out("sv_maxplayers: %d",DAT_00699584);
    return;
  }
  if (in_EAX == 1) {
    lVar1 = _atol((char *)*param_1);
    if ((0 < lVar1) && (lVar1 < 0x11)) {
      DAT_00699584 = lVar1;
      if (DAT_0071c2d4 != 0) {
        *(char *)(DAT_0071c2d4 + 0x1a5) = (char)lVar1;
      }
      if (lVar1 == 1) {
        chimera__console_out("WARNING: sv_maxplayers set to 1, are you sure you want to do this?");
      }
      goto LAB_004e4b0b;
    }
    chimera__console_out("sv_maxplayers must be between 1 and %d",0x10);
  }
  chimera__console_out();
  return;
}
#endif
