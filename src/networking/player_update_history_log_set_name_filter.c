// player_update_history_log_set_name_filter  (Ghidra: FUN_004e5f80; renamed, no prior name)
// address 0x4e5f80, size 76 bytes
// name confidence: 0.4   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md: "Converts a player-name argument to a wide
// string and stores it as the active filter used to restrict per-player update-history debug
// logging."; player_update_history_log_printf_filtered.c (this batch, 0x4e5f20) reads the same
// buffer with wcscmp.
// register convention: disassembly (objdump -d -M intel) shows the string argument arrives in
// ESI (`mov eax,esi` is the very first instruction, used only to walk the string without
// consuming ESI itself).
//   // blam-cc: ESI -> name
// UNSURE: the destination is written back to front (NUL first, then each character from the end
// towards the start); reproduced here as a plain forward loop since the observable result is
// identical for a NUL-terminated, non-overlapping copy.

#include "tags.h"
#include "memory.h"
#include <string.h>

extern uint16_t local_player_name_filter[0x400]; // 0x0071c420, see player_update_history_log_printf_filtered.c

// Copies name (an 8-bit string, e.g. a console command argument) into the shared UTF-16 filter
// buffer used by player_update_history_log_printf_filtered, truncating to 0x3ff characters.
void player_update_history_log_set_name_filter(char *name) // blam-cc: ESI -> name
{
    int32_t length;
    int32_t i;

    length = (int32_t)strlen(name);
    if ((uint32_t)(length * 2 + 2) > 0x800) {
        length = 0x3ff;
    }
    if ((uint32_t)(length * 2 + 2) <= 0x800) {
        local_player_name_filter[length] = 0;
        for (i = length - 1; i >= 0; i--) {
            local_player_name_filter[i] = (uint16_t)(uint8_t)name[i];
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e5f80), from tools/pack.py 0x4e5f80:

void FUN_004e5f80(void)

{
  char cVar1;
  char *pcVar2;
  int iVar3;
  char *unaff_ESI;

  pcVar2 = unaff_ESI;
  do {
    cVar1 = *pcVar2;
    pcVar2 = pcVar2 + 1;
  } while (cVar1 != '\0');
  iVar3 = (int)pcVar2 - (int)(unaff_ESI + 1);
  if (0x800 < iVar3 * 2 + 2U) {
    iVar3 = 0x3ff;
  }
  if (iVar3 * 2 + 2U < 0x801) {
    *(undefined2 *)((iVar3 + -1) * 2 + 0x71c422) = 0;
    iVar3 = iVar3 + -1;
    while (-1 < iVar3) {
      *(ushort *)((iVar3 + -1) * 2 + 0x71c422) = (ushort)(byte)unaff_ESI[iVar3];
      iVar3 = iVar3 + -1;
    }
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) confirms ESI and pins the exact indices (worked
through concretely for L=0,1,2,3 to resolve Ghidras off-by-one-looking address arithmetic):
  4e5f80: mov eax,esi          ; walk the string from ESI without consuming it
  4e5fac: mov WORD PTR [eax*2+0x71c422],0x0   ; with eax == length-1 at this point, this writes
                                               ; player_update_log_filter_name[length] = 0
  4e5fc1: movzx cx,BYTE PTR [eax+esi*1+0x1]   ; name[eax+1]
  4e5fc7: mov WORD PTR [eax*2+0x71c422],cx    ; player_update_log_filter_name[eax+1] = name[eax+1]
  (the loop eax runs length-2 .. -1, so eax+1 runs length-1 .. 0 -- every index of name is
  covered exactly once)
#endif
