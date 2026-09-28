// sv_timelimit  (Ghidra: sv_timelimit, already named)
// address 0x4e49a0, size 278 bytes
// name confidence: 0.9   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md; literal "-1 = default"/"0 = infinite" strings.
// register convention: disassembly (objdump -d -M intel) reads `arguments` from [esp+0xc] after
// two prologue pushes (ebx, esi) -- same shape as sv_friendly_fire.c and sv_ban_penalty.c.
//   // blam-cc: EAX -> argument_count, stack -> arguments
// reconciled: R04 0x006f1d20 void * network_engine_callback_block -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdlib.h>

extern int32_t sv_timelimit_minutes; // 0x00699608
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)

extern void chimera__console_out(const char *format, ...); // 0x496b50

// Console command: gets or sets the game time limit in minutes (-1 default, 0 infinite, else
// 1..599), accepting "default"/"infinite" as synonyms for -1/0.
void sv_timelimit(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    uint8_t changed = 0;

    if (argument_count != 0) {
        if (argument_count != 1) {
            chimera__console_out("Incorrect usage. Type help sv_timelimit for more information.");
            return;
        }
        {
            char *arg = (char *)arguments[0];
            if (_stricmp(arg, "-1") == 0 || _stricmp(arg, "default") == 0) {
                sv_timelimit_minutes = -1;
                changed = 1;
            } else if (_stricmp(arg, "0") == 0 || _stricmp(arg, "infinite") == 0) {
                sv_timelimit_minutes = 0;
                changed = 1;
            } else {
                int32_t value = atol(arg);
                if (value < 1 || 599 < value) {
                    chimera__console_out("sv_timelimit:  invalid parameter %s", arg);
                    chimera__console_out("Incorrect usage. Type help sv_timelimit for more information.");
                    return;
                }
                changed = 1;
                sv_timelimit_minutes = value;
            }
        }
    }
    if (sv_timelimit_minutes == -1) {
        chimera__console_out("sv_timelimit: -1 = default");
    } else if (sv_timelimit_minutes != 0) {
        chimera__console_out("sv_timelimit: %d minutes", sv_timelimit_minutes);
        goto done;
    } else {
        chimera__console_out("sv_timelimit: 0 = infinite");
    }
done:
    if (changed && current_game_engine != 0) {
        chimera__console_out("   Game in progress...  Changes will apply to the next game.");
    }
}

#if 0
Original Ghidra decompilation (0x4e49a0), from tools/pack.py 0x4e49a0:

void sv_timelimit(undefined4 *param_1)

{
  bool bVar1;
  int in_EAX;
  int iVar2;
  long lVar3;
  char *pcVar4;

  bVar1 = false;
  if (in_EAX != 0) {
    if (in_EAX != 1) {
LAB_004e4a37:
      chimera__console_out();
      return;
    }
    pcVar4 = (char *)*param_1;
    iVar2 = __stricmp(pcVar4,"-1");
    if ((iVar2 == 0) || (iVar2 = __stricmp(pcVar4,"default"), iVar2 == 0)) {
      DAT_00699608 = -1;
      bVar1 = true;
    }
    else {
      iVar2 = __stricmp(pcVar4,"0");
      if ((iVar2 == 0) || (iVar2 = __stricmp(pcVar4,"infinite"), iVar2 == 0)) {
        DAT_00699608 = 0;
        bVar1 = true;
      }
      else {
        lVar3 = _atol(pcVar4);
        if ((lVar3 < 1) || (599 < lVar3)) {
          chimera__console_out("sv_timelimit:  invalid parameter %s",pcVar4);
          goto LAB_004e4a37;
        }
        bVar1 = true;
        DAT_00699608 = lVar3;
      }
    }
  }
  if (DAT_00699608 == -1) {
    pcVar4 = "sv_timelimit: -1 = default";
  }
  else {
    if (DAT_00699608 != 0) {
      chimera__console_out("sv_timelimit: %d minutes",DAT_00699608);
      goto LAB_004e4a95;
    }
    pcVar4 = "sv_timelimit: 0 = infinite";
  }
  chimera__console_out(pcVar4);
LAB_004e4a95:
  if ((bVar1) && (DAT_006f1d20 != 0)) {
    chimera__console_out();
    return;
  }
  return;
}
#endif
