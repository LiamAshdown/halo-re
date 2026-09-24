// sv_friendly_fire  (Ghidra: sv_friendly_fire, already named)
// address 0x4e4810, size 378 bytes
// name confidence: 0.9   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md; the literal "0 = default"/"1 = off"/
// "2 = shields"/"3 = on" strings and their string/number synonyms.
// register convention: disassembly (objdump -d -M intel) reads `arguments` from [esp+0xc] after
// two prologue pushes (ebx, esi) that do not touch this value -- the shared EAX/stack shape for
// this batch's "get/set console variable" commands (see sv_ban_penalty.c).
//   // blam-cc: EAX -> argument_count, stack -> arguments
// UNSURE: the "Game in progress... Changes will apply to the next game." follow-up message's
// exact condition (`bVar1 && network_engine_callback_block != 0`, this batch does not resolve
// what that block's non-NULL-ness signals beyond "a game is active").

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern int32_t sv_friendly_fire_mode; // 0x0071c40c
extern void *network_engine_callback_block; // 0x006f1d20 (types/networking.h: "the network game
    // engine callback block")

extern int32_t __stricmp(const char *a, const char *b); // CRT, case-insensitive strcmp
extern void chimera__console_out(const char *format, ...); // 0x496b50

// Console command: gets or sets the friendly-fire mode (0/default, 1/off, 2/shields, 3/on),
// accepting either the numeric or word form of each value.
void sv_friendly_fire(uint32_t argument_count, int32_t *arguments) // blam-cc: EAX -> argument_count, stack -> arguments
{
    uint8_t changed = 0;
    const char *label;

    if (argument_count != 0) {
        if (argument_count != 1) {
            chimera__console_out("Incorrect usage. Type help sv_friendly_fire for more information.");
            return;
        }
        {
            char *arg = (char *)arguments[0];
            if (__stricmp(arg, "0") == 0 || __stricmp(arg, "default") == 0) {
                sv_friendly_fire_mode = 0;
                changed = 1;
            } else if (__stricmp(arg, "1") == 0 || __stricmp(arg, "off") == 0) {
                sv_friendly_fire_mode = 1;
                changed = 1;
            } else if (__stricmp(arg, "2") == 0 || __stricmp(arg, "shields") == 0) {
                sv_friendly_fire_mode = 2;
                changed = 1;
            } else if (__stricmp(arg, "3") == 0 || __stricmp(arg, "on") == 0) {
                sv_friendly_fire_mode = 3;
                changed = 1;
            } else {
                chimera__console_out("sv_friendly_fire:  invalid parameter %s", arg);
                chimera__console_out("Incorrect usage. Type help sv_friendly_fire for more information.");
                return;
            }
        }
    }
    switch (sv_friendly_fire_mode) {
    case 1: label = "1 = off"; break;
    case 2: label = "2 = shields"; break;
    case 3: label = "3 = on"; break;
    default:
        sv_friendly_fire_mode = 0;
        /* fallthrough */
    case 0: label = "0 = default"; break;
    }
    chimera__console_out("sv_friendly_fire: %s", label);
    if (changed && network_engine_callback_block != 0) {
        chimera__console_out("   Game in progress...  Changes will apply to the next game.");
    }
}

#if 0
Original Ghidra decompilation (0x4e4810), from tools/pack.py 0x4e4810:

void sv_friendly_fire(undefined4 *param_1)

{
  bool bVar1;
  int in_EAX;
  int iVar2;
  char *pcVar3;

  bVar1 = false;
  if (in_EAX != 0) {
    if (in_EAX != 1) {
LAB_004e48de:
      chimera__console_out();
      return;
    }
    pcVar3 = (char *)*param_1;
    iVar2 = __stricmp(pcVar3,"0");
    if ((iVar2 == 0) || (iVar2 = __stricmp(pcVar3,"default"), iVar2 == 0)) {
      DAT_0071c40c = 0;
      bVar1 = true;
    }
    else {
      iVar2 = __stricmp(pcVar3,"1");
      if ((iVar2 == 0) ||
         (iVar2 = __stricmp(pcVar3,(char *)&PTR_s_iate_0066664c_0x23_006651fc), iVar2 == 0)) {
        DAT_0071c40c = 1;
        bVar1 = true;
      }
      else {
        iVar2 = __stricmp(pcVar3,"2");
        if ((iVar2 == 0) || (iVar2 = __stricmp(pcVar3,"shields"), iVar2 == 0)) {
          DAT_0071c40c = 2;
          bVar1 = true;
        }
        else {
          iVar2 = __stricmp(pcVar3,"3");
          if ((iVar2 != 0) && (iVar2 = __stricmp(pcVar3,"on"), iVar2 != 0)) {
            chimera__console_out("sv_friendly_fire:  invalid parameter %s",pcVar3);
            goto LAB_004e48de;
          }
          DAT_0071c40c = 3;
          bVar1 = true;
        }
      }
    }
  }
  switch(DAT_0071c40c) {
  case 1:
    pcVar3 = "1 = off";
    break;
  case 2:
    pcVar3 = "2 = shields";
    break;
  case 3:
    pcVar3 = "3 = on";
    break;
  default:
    DAT_0071c40c = 0;
  case 0:
    pcVar3 = "0 = default";
  }
  chimera__console_out("sv_friendly_fire: %s",pcVar3);
  if ((bVar1) && (DAT_006f1d20 != 0)) {
    chimera__console_out();
    return;
  }
  return;
}
#endif
