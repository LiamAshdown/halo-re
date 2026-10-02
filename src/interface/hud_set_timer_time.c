// hud_set_timer_time  (Ghidra: FUN_004adbf0, renamed in the phase-4 review)
// address 0x4adbf0, size 101 bytes
// name confidence: 0.6 (chosen)   rewrite confidence: 0.9
// evidence: objdump 0x4adbf0..0x4adc54. (ECX * 60 + EAX) * 30 ticks: minutes in ECX, seconds in
// EAX; no direct callers (hs function table). Arms the countdown drawn by hud_timer_draw
// (0x4add10): timer_ticks, timer_start_time = game time, not paused, active, and clamps the
// timer anchor into 0..4.
// register convention: ECX minutes, EAX seconds.
//   // blam-cc: minutes -> ECX, seconds -> EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern hud_messaging_globals *hud_messaging;  // 0x006b3a40
extern game_time_globals *game_time;          // 0x006f1d6c

// blam-cc: minutes -> ECX, seconds -> EAX
// Starts the HUD countdown timer at minutes:seconds.
void hud_set_timer_time(int32_t minutes, int32_t seconds)
{
    int16_t anchor;

    hud_messaging->timer_ticks = (uint16_t)((minutes * 60 + seconds) * 30);
    hud_messaging->timer_paused = 0;
    hud_messaging->timer_active = 1;
    hud_messaging->timer_start_time = game_time->game_time;
    anchor = hud_messaging->timer_anchor;
    if (anchor < 0) {
        anchor = 0;
    } else if (anchor > 4) {
        anchor = 4;
    }
    hud_messaging->timer_anchor = anchor;
}

#if 0
Original Ghidra decompilation (0x4adbf0):

void FUN_004adbf0(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  short in_AX;
  short in_CX;

  iVar2 = DAT_006b3a40;
  *(short *)(DAT_006b3a40 + 0x47c) = (in_CX * 0x3c + in_AX) * 0x1e;
  iVar3 = DAT_006f1d6c;
  *(undefined1 *)(iVar2 + 0x486) = 0;
  *(undefined1 *)(iVar2 + 0x487) = 1;
  sVar1 = *(short *)(iVar2 + 0x484);
  *(undefined4 *)(iVar2 + 0x478) = *(undefined4 *)(iVar3 + 0xc);
  if (sVar1 < 0) {
    *(undefined2 *)(iVar2 + 0x484) = 0;
    return;
  }
  if (4 < sVar1) {
    *(undefined2 *)(iVar2 + 0x484) = 4;
    return;
  }
  *(short *)(iVar2 + 0x484) = sVar1;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
