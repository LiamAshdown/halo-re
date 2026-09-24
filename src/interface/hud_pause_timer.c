// hud_pause_timer  (Ghidra: FUN_004adc60, renamed in the phase-4 review)
// address 0x4adc60, size 85 bytes
// name confidence: 0.55 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4adc60..0x4adcb4; no direct callers (hs function table, pause_hud_timer).
// Pausing (DL set) folds the elapsed time into timer_ticks, which then holds the remaining
// ticks (hud_timer_get_ticks returns it as is); resuming adds the same 16-bit difference back.
// Both use the low 16 bits of timer_start_time and game_time (word subtracts).
// register convention: DL paused.
//   // blam-cc: paused -> DL

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern game_time_globals *game_time;          // 0x006f1d6c

// blam-cc: paused -> DL
void hud_pause_timer(uint8_t paused)
{
    int16_t ticks = (int16_t)hud_messaging->timer_ticks;

    hud_messaging->timer_paused = paused;
    if (ticks <= 0) {
        return;
    }
    if (paused != 0) {
        hud_messaging->timer_ticks =
            (uint16_t)((uint16_t)((uint16_t)hud_messaging->timer_start_time - (uint16_t)game_time->game_time) + ticks);
    } else {
        hud_messaging->timer_ticks =
            (uint16_t)((uint16_t)((uint16_t)game_time->game_time - (uint16_t)hud_messaging->timer_start_time) + ticks);
    }
}

#if 0
Original Ghidra decompilation (0x4adc60):

void FUN_004adc60(void)

{
  short sVar1;
  int iVar2;
  char in_DL;

  iVar2 = DAT_006b3a40;
  sVar1 = *(short *)(DAT_006b3a40 + 0x47c);
  *(char *)(DAT_006b3a40 + 0x486) = in_DL;
  if (0 < sVar1) {
    if (in_DL != '\0') {
      *(short *)(iVar2 + 0x47c) =
           (*(short *)(iVar2 + 0x478) - *(short *)(DAT_006f1d6c + 0xc)) + sVar1;
      return;
    }
    *(short *)(iVar2 + 0x47c) = (*(short *)(DAT_006f1d6c + 0xc) - *(short *)(iVar2 + 0x478)) + sVar1
    ;
  }
  return;
}
#endif
